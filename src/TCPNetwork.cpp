#include "TCPNetwork.hpp"
#include "sirikata/network/IOServiceFactory.hpp"
#include "sirikata/network/IOService.hpp"
#include "sirikata/network/StreamFactory.hpp"
#include "sirikata/network/StreamListenerFactory.hpp"
#include "sirikata/network/StreamListener.hpp"
#include "Options.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"

using namespace Sirikata::Network;
using namespace Sirikata;

#define TCPNET_LOG(level,msg) SILOG(tcpnetwork,level,"[TCPNET] " << msg)

namespace CBR {

TCPNetwork::RemoteStream::RemoteStream(TCPNetwork* parent, Sirikata::Network::Stream*strm)
        : stream(strm),
          network_endpoint(),
          logical_endpoint(NullServerID),
          connected(false),
          shutting_down(false),
          front(NULL),
          paused(false)
{
}

TCPNetwork::RemoteStream::~RemoteStream() {
    delete stream;
}




TCPNetwork::TCPNetwork(SpaceContext* ctx)
 : Network(ctx),
   mSendListener(NULL),
   mReceiveListener(NULL)
{
    mStreamPlugin = GetOption("spacestreamlib")->as<String>();
    mPluginManager.load(Sirikata::DynamicLibrary::filename(mStreamPlugin));

    mListenOptions = StreamListenerFactory::getSingleton().getOptionParser(mStreamPlugin)(GetOption("spacestreamoptions")->as<String>());
    mSendOptions = StreamFactory::getSingleton().getOptionParser(mStreamPlugin)(GetOption("spacestreamoptions")->as<String>());

    mIOService = IOServiceFactory::makeIOService();
    mIOWork = new IOWork(mIOService, "TCPNetwork Work");
    mThread = new Thread(std::tr1::bind(&IOService::run,mIOService));

    mListener = StreamListenerFactory::getSingleton().getConstructor(mStreamPlugin)(mIOService,mListenOptions);
}

TCPNetwork::~TCPNetwork() {
    delete mListener;

    // Cancel close timers
    for(TimerSet::iterator it = mClosingStreamTimers.begin(); it != mClosingStreamTimers.end(); it++) {
        Sirikata::Network::IOTimerPtr timer = *it;
        timer->cancel();
    }
    mClosingStreamTimers.clear();

    // Clear out front streams to get rid of references to them
    mFrontStreams.clear();

    // Close all connections
    for(RemoteStreamMap::iterator it = mClosingStreams.begin(); it != mClosingStreams.end(); it++) {
        RemoteStreamPtr conn = it->second;
        conn->stream->close();
    }
    mClosingStreams.clear();

    for(RemoteStreamMap::iterator it = mRemoteStreams.begin(); it != mRemoteStreams.end(); it++) {
        RemoteStreamPtr conn = it->second;
        conn->stream->close();
    }
    mRemoteStreams.clear();


    delete mIOWork;
    mIOWork = NULL;

    mIOService->stop(); // FIXME forceful shutdown...

    mThread->join();
    delete mThread;

    IOServiceFactory::destroyIOService(mIOService);
    mIOService = NULL;
}



// IO Thread Methods

void TCPNetwork::newStreamCallback(Sirikata::Network::Stream* newStream,
				   Sirikata::Network::Stream::SetCallbacks& setCallbacks)
{
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    if (newStream == NULL) {
        // Indicates underlying connection is being deleted.  Annoying, but we
        // just ignore it.
        return;
    }

    TCPNET_LOG(info,"New stream accepted.");
    // A new stream was initiated by a remote endpoint.  We need to
    // set up our basic storage and then wait for the specification of
    // the remote endpoint ID before allowing use of the connection

    RemoteStreamPtr remote_stream( new RemoteStream(this, newStream) );
    RemoteStreamWPtr weak_remote_stream( remote_stream );

    Address4 source_address(newStream->getRemoteEndpoint());
    remote_stream->network_endpoint = source_address;

    setCallbacks(
        std::tr1::bind(&TCPNetwork::connectionCallback, this, weak_remote_stream, _1, _2),
        std::tr1::bind(&TCPNetwork::bytesReceivedCallback, this, weak_remote_stream, _1),
        std::tr1::bind(&TCPNetwork::readySendCallback, this, weak_remote_stream)
    );

    mPendingStreams[source_address] = remote_stream;
}

void TCPNetwork::connectionCallback(const RemoteStreamWPtr& rwstream, const Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason) {
    RemoteStreamPtr remote_stream(rwstream);

    if (!remote_stream)
        TCPNET_LOG(warning,"Got connection callback on stream that is no longer referenced.");

    if (status == Sirikata::Network::Stream::Disconnected ||
        status == Sirikata::Network::Stream::ConnectionFailed) {
        TCPNET_LOG(info,"Got disconnection event from " << remote_stream->logical_endpoint);

        remote_stream->connected = false;
        remote_stream->shutting_down = true;
        mContext->mainStrand->post(
            std::tr1::bind(&TCPNetwork::markDisconnected, this, remote_stream)
                                   );
    }
    else if (status == Sirikata::Network::Stream::Connected) {
        // Note: We should only get Connected for connections we initiated.
        TCPNET_LOG(info,"Sending intro and marking stream as connected to " << remote_stream->logical_endpoint);
        // The first message needs to be our introduction
        sendServerIntro(remote_stream);
        // And after we're sure its sent, open things up for everybody else
        remote_stream->connected = true;
        // And notify listeners.  We *must* do this here because the initial
        // send may have failed due to not having a connection so the normal
        // readySend callback will not occur since we couldn't register for
        // it yet.
        notifyListenersOfNewStream(remote_stream->logical_endpoint);
    }
    else {
        TCPNET_LOG(error,"Unhandled send stream connection status: " << status << " -- " << reason);
    }
}

Sirikata::Network::Stream::ReceivedResponse TCPNetwork::bytesReceivedCallback(const RemoteStreamWPtr& rwstream, Chunk&data) {
    RemoteStreamPtr remote_stream(rwstream);

    // If we've lost all references to the RemoteStream just ignore
    if (!remote_stream) {
        TCPNET_LOG(error,"Ignoring received data; invalid weak reference to remote stream.");
        return Sirikata::Network::Stream::AcceptedData;
    }

    // If the stream hasn't been marked as connected, this *should* be
    // the initial header
    if (remote_stream->connected == false) {
        // The stream might already be closing.  If so, just ignore this data.
        if (remote_stream->shutting_down)
            return Sirikata::Network::Stream::AcceptedData;

        // Remove from the list of pending connections
        TCPNET_LOG(info,"Parsing endpoint information for incomplete remote-initiated remote stream.");
        Address4 source_addr = remote_stream->network_endpoint;
        RemoteNetStreamMap::iterator it = mPendingStreams.find(source_addr);
        if (it == mPendingStreams.end()) {
            TCPNET_LOG(error,"Address for connection not found in pending receive buffers"); // FIXME print addr
        }
        else {
            mPendingStreams.erase(it);
        }

        // Parse the header indicating the remote ID
        CBR::Protocol::Server::ServerIntro intro;
        bool parsed = parsePBJMessage(&intro, data);
        assert(parsed);

        TCPNET_LOG(info,"Parsed remote endpoint information from " << intro.id());
        remote_stream->logical_endpoint = intro.id();
        remote_stream->connected = true;
        mContext->mainStrand->post(
            std::tr1::bind(&TCPNetwork::addNewStream,
                           this,
                           remote_stream
                           )
                                   );
        notifyListenersOfNewStream(intro.id());
    }
    else {
        TCPNET_LOG(insane,"Handling regular received data from " << remote_stream->logical_endpoint << ".");
        // Normal case, we can just handle the message
        if (remote_stream->front != NULL) {
            // Space is occupied, pause and ignore
            TCPNET_LOG(insane,"Pausing receive from " << remote_stream->logical_endpoint << ".");
            remote_stream->paused = true;
            return Sirikata::Network::Stream::PauseReceive;
        }
        else {
            // Otherwise, give it its own chunk and push it up
            TCPNET_LOG(insane,"Passing data up to next layer from " << remote_stream->logical_endpoint << ".");
            Chunk* tmp = new Chunk;
            tmp->swap(data);
            remote_stream->front = tmp;
            // Note that the notification happens on the IO thread
            mReceiveListener->networkReceivedData( remote_stream->logical_endpoint );
        }
    }

    return Sirikata::Network::Stream::AcceptedData;
}

void TCPNetwork::readySendCallback(const RemoteStreamWPtr& rwstream) {
    RemoteStreamPtr remote_stream(rwstream);
    if (!remote_stream)
        return;

    assert(remote_stream->logical_endpoint != NullServerID);

    mSendListener->networkReadyToSend(remote_stream->logical_endpoint);
}

void TCPNetwork::notifyListenersOfNewStream(const ServerID& remote) {
    mSendListener->networkReadyToSend(remote);
    mReceiveListener->networkReceivedConnection(remote);
}

// Main Thread/Strand Methods

namespace {

char toHex(unsigned char u) {
    if (u<=9) return '0'+u;
    return 'A'+(u-10);
}

void hexPrint(const char *name, const Chunk&data) {
    std::string str;
    str.resize(data.size()*2);
    for (size_t i=0;i<data.size();++i) {
        str[i*2]=toHex(data[i]%16);
        str[i*2+1]=toHex(data[i]/16);
    }
    std::cout<< name<<' '<<str<<'\n';
}

} // namespace

void TCPNetwork::openConnection(const ServerID& dest) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    TCPNET_LOG(info,"Initiating new connection to " << dest);

    RemoteStreamPtr remote(
        new RemoteStream(
            this, StreamFactory::getSingleton().getConstructor(mStreamPlugin)(mIOService,mSendOptions)
                         )
                           );
    RemoteStreamWPtr weak_remote(remote);

    // Insert before calling connect to ensure data is in place when event
    // occurs.  Note that remote->connected is still false here so we won't
    // prematurely use the connection for sending
    Address4* addr = mServerIDMap->lookupInternal(dest);
    assert(addr != NULL);
    remote->network_endpoint = *addr;
    remote->logical_endpoint = dest;
    mRemoteStreams.insert(std::pair<ServerID,RemoteStreamPtr>(dest, remote));

    Sirikata::Network::Stream::SubstreamCallback sscb(&Sirikata::Network::Stream::ignoreSubstreamCallback);
    Sirikata::Network::Stream::ConnectionCallback connCallback(std::tr1::bind(&TCPNetwork::connectionCallback, this, weak_remote, _1, _2));
    Sirikata::Network::Stream::ReceivedCallback br(std::tr1::bind(&TCPNetwork::bytesReceivedCallback, this, weak_remote, _1));
    Sirikata::Network::Stream::ReadySendCallback readySendCallback(std::tr1::bind(&TCPNetwork::readySendCallback, this, weak_remote));
    remote->stream->connect(convertAddress4ToSirikata(*addr),
                            sscb,
                            connCallback,
                            br,
                            readySendCallback);

    // Note: Initial connection header is sent upon successful connection
}

void TCPNetwork::markDisconnected(const RemoteStreamWPtr& wstream) {
    // When we get a disconnection signal, there's nothing we can even do
    // anymore -- the other side has closed the connection for some reason and
    // isn't going to accept any more data.  The connection has already been
    // marked as disconnected and shutting down, we just have to do the removal
    // from the main thread.

    RemoteStreamPtr remote_stream(wstream);
    if (!remote_stream)
        return;

    RemoteStreamMap::iterator it = mRemoteStreams.find(remote_stream->logical_endpoint);
    // We may not even have the connection anymore, either because we explicitly
    // removed it or the disconnection event came for a closing stream.
    if (it == mRemoteStreams.end())
        return;

    // The stored one may not be the one we're looking for
    RemoteStreamPtr stored_remote_stream = it->second;
    if (stored_remote_stream != remote_stream)
        return;

    assert(remote_stream &&
           !remote_stream->connected &&
           remote_stream->shutting_down);

    mRemoteStreams.erase(it);
}


void TCPNetwork::sendServerIntro(const RemoteStreamPtr& out_stream) {
    CBR::Protocol::Server::ServerIntro intro;
    intro.set_id(mContext->id());
    std::string serializedIntro;
    serializePBJMessage(&serializedIntro, intro);

    out_stream->stream->send(
        MemoryReference(serializedIntro),
        ReliableOrdered
    );
}

void TCPNetwork::addNewStream(RemoteStreamPtr remote_stream) {
    RemoteStreamMap::iterator it = mRemoteStreams.find(remote_stream->logical_endpoint);

    // If there's already a stream in the map, this incoming stream conflicts
    // with one we started in the outgoing direction.  We need to cancel one and
    // save the other in our map.
    if (it != mRemoteStreams.end()) {
        TCPNET_LOG(info,"Resolving multiple conflicting connections to " << remote_stream->logical_endpoint);

        RemoteStreamPtr new_remote_stream = remote_stream;
        RemoteStreamPtr existing_remote_stream = it->second;

        // The choice of which connection to maintain is arbitrary, but needs to
        // be symmetric.
        RemoteStreamPtr stream_to_save;
        RemoteStreamPtr stream_to_close;
        if (mContext->id() < remote_stream->logical_endpoint) {
            stream_to_save = existing_remote_stream;
            stream_to_close = new_remote_stream;
        }
        else {
            stream_to_save = new_remote_stream;
            stream_to_close = existing_remote_stream;
        }

        mRemoteStreams[stream_to_save->logical_endpoint] = stream_to_save;
        // We should never have 2 closing streams at the same time or else the
        // remote end is misbehaving
        if (mClosingStreams.find(stream_to_close->logical_endpoint) != mClosingStreams.end()) {
            TCPNET_LOG(fatal,"Multiple closing streams for single remote server, server " << stream_to_close->logical_endpoint);
        }
        assert(mClosingStreams.find(stream_to_close->logical_endpoint) == mClosingStreams.end());
        stream_to_close->shutting_down = true;
        mClosingStreams[stream_to_close->logical_endpoint] = stream_to_close;

        Sirikata::Network::IOTimerPtr timer = Sirikata::Network::IOTimer::create(mIOService);
        timer->wait(
            Duration::seconds(5),
            std::tr1::bind(&TCPNetwork::handleClosingStreamTimeout, this, timer, RemoteStreamWPtr(stream_to_close))
                   );

        mClosingStreamTimers.insert(timer);

        return;
    }

    // Otherwise this is the only connection for this endpoint we know about,
    // just store it
    mRemoteStreams[remote_stream->logical_endpoint] = remote_stream;
}

void TCPNetwork::handleClosingStreamTimeout(Sirikata::Network::IOTimerPtr timer, RemoteStreamWPtr& wstream) {
    mClosingStreamTimers.erase(timer);

    RemoteStreamPtr rstream(wstream);
    if (!rstream) {
        TCPNET_LOG(error,"Got close stream timeout for stream that is no longer in closing stream set.");
        return;
    }

    TCPNET_LOG(info,"Closing stream due to timeout, server " << rstream->logical_endpoint);

    rstream->connected = false;
    rstream->stream->close();

    RemoteStreamMap::iterator it = mClosingStreams.find(rstream->logical_endpoint);
    if (it == mClosingStreams.end()) return;
    mClosingStreams.erase(it);
}

TCPNetwork::RemoteStreamPtr TCPNetwork::getReceiveQueue(const ServerID& addr) {
    // Consider
    //  1) an already marked front stream
    //  2) closing streams
    //  3) active streams.
    // In order to return a stream it must
    //  a) exist
    //  b) have data in its receive queue
    //  c) be either connected or shutting_down

    RemoteStreamMap::iterator front_it = mFrontStreams.find(addr);
    if (front_it != mFrontStreams.end()) {
        // At least for closing streams, results may no longer be valid, need to
        // double check.
        RemoteStreamPtr result = front_it->second;
        if (result &&
            result->front != NULL &&
            (result->connected || result->shutting_down)
            ) {
            return result;
        }
        else {
            // It got invalidated, get it out of the front stream map and
            // continue search.
            clearReceiveQueue(addr);
        }
    }

    RemoteStreamMap::iterator closing_it = mClosingStreams.find(addr);
    if (closing_it != mClosingStreams.end()) {
        RemoteStreamPtr result = closing_it->second;
        if (result &&
            result->front != NULL &&
            (result->connected || result->shutting_down)
            ) {
            mFrontStreams[addr] = result;
            return result;
        }
    }

    RemoteStreamMap::iterator active_it = mRemoteStreams.find(addr);
    if (active_it != mRemoteStreams.end()) {
        RemoteStreamPtr result = active_it->second;
        if (result &&
            result->front != NULL &&
            (result->connected || result->shutting_down)
            ) {
            mFrontStreams[addr] = result;
            return result;
        }
    }

    return RemoteStreamPtr();
}

void TCPNetwork::clearReceiveQueue(const ServerID& from) {
    mFrontStreams.erase(from);
}


void TCPNetwork::setSendListener(SendListener* sl) {
    mSendListener = sl;
}

bool TCPNetwork::canSend(const ServerID& addr, uint32 size) {
    RemoteStreamMap::iterator where = mRemoteStreams.find(addr);

    // If we don't have a connection yet, we'll use this as a hint to make the
    // connection, but return false since a call to send() would fail
    if (where == mRemoteStreams.end()) {
        openConnection(addr);
        return false;
    }

    RemoteStreamPtr remote(where->second);
    if (remote->connected &&
        !remote->shutting_down &&
        remote->stream->canSend(size))
        return true;

    return false;
}

bool TCPNetwork::send(const ServerID& addr, const Chunk& data) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    RemoteStreamMap::iterator where = mRemoteStreams.find(addr);

    if (where == mRemoteStreams.end()) {
        openConnection(addr);
        return false;
    }

    RemoteStreamPtr remote(where->second);
    bool success = (remote->connected &&
                    !remote->shutting_down &&
                    remote->stream->send(data,ReliableOrdered));
    if (!success)
        remote->stream->requestReadySendCallback();
    return success;
}


void TCPNetwork::listen(const ServerID& as_server, ReceiveListener* receive_listener) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    mReceiveListener = receive_listener;

    TCPNET_LOG(info,"Listening for remote space servers.");

    Address4* listen_addr = mServerIDMap->lookupInternal(as_server);
    assert(listen_addr != NULL);
    mListenAddress = *listen_addr;

    Address listenAddress(convertAddress4ToSirikata(mListenAddress));
    mListener->listen(
        Address(listenAddress.getHostName(),listenAddress.getService()),
        std::tr1::bind(&TCPNetwork::newStreamCallback,this,_1,_2)
                      );
}


Chunk* TCPNetwork::front(const ServerID& from) {
    RemoteStreamPtr stream(getReceiveQueue(from));

    if (!stream || !stream->front)
        return NULL;

    Chunk* result = stream->front;
    return result;
}

Chunk* TCPNetwork::receiveOne(const ServerID& from) {
    // Use front() to get the next one.  We have a slight duplication of work in
    // calling getReceiveQueue twice...
    Chunk* result = front(from);

    // If front fails we can bail out now
    if (result == NULL)
        return NULL;

    // Otherwise, just do cleanup before returning the front item
    RemoteStreamPtr stream(getReceiveQueue(from));
    assert(stream);
    assert(stream->front == result);

    // Unmark this queue as the front queue
    clearReceiveQueue(from);
    // Was already popped by front, just zero out the front ptr
    stream->front = NULL;
    // Unpause receiving if necessary
    if (stream->paused) {
        stream->paused = false;
        stream->stream->readyRead();
    }

    return result;
}

} // namespace Sirikata
