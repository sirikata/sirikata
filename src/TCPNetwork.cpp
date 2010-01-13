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

namespace CBR {

TCPNetwork::RemoteStream::RemoteStream(TCPNetwork* parent, Sirikata::Network::Stream*strm)
        : stream(strm),
          connected(false),
          shutting_down(false),
          front(NULL),
          buffer(Sirikata::SizedResourceMonitor(parent->mIncomingBufferLength)),
          paused(false)
{
}

TCPNetwork::RemoteStream::~RemoteStream() {
    delete stream;
}




TCPNetwork::TCPNetwork(SpaceContext* ctx, uint32 incomingBufferLength, uint32 incomingBandwidth, uint32 outgoingBandwidth)
 : Network(ctx),
   mIncomingBufferLength(incomingBufferLength),
   mIncomingBandwidth(incomingBandwidth),
   mOutgoingBandwidth(outgoingBandwidth),
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

void TCPNetwork::newStreamCallback(Stream* newStream, Stream::SetCallbacks&setCallbacks) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    if (newStream == NULL) {
        // Indicates underlying connection is being deleted.  Annoying, but we
        // just ignore it.
        return;
    }

    // A new stream was initiated by a remote endpoint.  We need to
    // set up our basic storage and then wait for the specification of
    // the remote endpoint ID before allowing use of the connection

    RemoteStreamPtr remote_stream( new RemoteStream(this, newStream) );
    RemoteStreamWPtr weak_remote_stream( remote_stream );

    Address4 source_address(newStream->getRemoteEndpoint());

    setCallbacks(
        std::tr1::bind(&TCPNetwork::connectionCallback, this, source_address, weak_remote_stream, _1, _2),
        std::tr1::bind(&TCPNetwork::bytesReceivedCallback, this, weak_remote_stream, _1),
        &Stream::ignoreReadySendCallback
    );

    mPendingStreams[source_address] = remote_stream;
}

void TCPNetwork::connectionCallback(const Address4& remote_addr, const RemoteStreamWPtr& rwstream, const Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason) {
    RemoteStreamPtr remote_stream(rwstream);

    if (status == Sirikata::Network::Stream::Disconnected ||
        status == Sirikata::Network::Stream::ConnectionFailed) {
        if (remote_stream) {
            remote_stream->connected = false;
            remote_stream->shutting_down = true;
        }
        mContext->mainStrand->post(
            std::tr1::bind(&TCPNetwork::markDisconnected, this, remote_addr)
        );
    }
    else if (status == Sirikata::Network::Stream::Connected) {
        // Note: We should only get Connected for connections we initiated.
        if (remote_stream) {
            // The first message needs to be our introduction
            sendServerIntro(remote_stream);
            // And after we're sure its sent, open things up for everybody else
            remote_stream->connected = true;
        }
    }
    else {
        SILOG(tcpnetwork,warning,"Unhandled send stream connection status: " << status << " -- " << reason);
    }
}

Sirikata::Network::Stream::ReceivedResponse TCPNetwork::bytesReceivedCallback(const RemoteStreamWPtr& rwstream, Chunk&data) {
    RemoteStreamPtr remote_stream(rwstream);

    // If we've lost all references to the RemoteStream just ignore
    if (!remote_stream)
        return Sirikata::Network::Stream::AcceptedData;

    // If the stream hasn't been marked as connected, this *should* be
    // the initial header
    if (remote_stream->connected == false) {
        // Remove from the list of pending connections
        Address4 source_addr = remote_stream->stream->getRemoteEndpoint();
        RemoteStreamMap::iterator it = mPendingStreams.find(source_addr);
        if (it == mPendingStreams.end()) {
            SILOG(tcpnetwork,error,"Address for connection not found in pending receive buffers"); // FIXME print addr
        }
        else {
            mPendingStreams.erase(it);
        }

        // Parse the header indicating the remote ID
        CBR::Protocol::Server::ServerIntro intro;
        bool parsed = parsePBJMessage(&intro, data);
        assert(parsed);

        Address4* remote_endpoint = mServerIDMap->lookupInternal( intro.id() );
        assert(remote_endpoint != NULL);

        remote_stream->endpoint = *remote_endpoint;
        remote_stream->connected = true;
        mContext->mainStrand->post(
            std::tr1::bind(&TCPNetwork::addNewStream,
                           this,
                           *remote_endpoint,
                           remote_stream
                           )
                                   );
    }
    else {
        // Normal case, we can just handle the message
        Chunk* tmp = new Chunk;
        tmp->swap(data);
        uint32 data_size = tmp->size();
        if (!remote_stream->buffer.push(tmp,false)) {
            remote_stream->paused = true;
            tmp->swap(data);
            delete tmp;
            return Sirikata::Network::Stream::PauseReceive;
        }
        else {
            // Check if this is the only thing on the queue implying it is the
            // now the front item and that we should send a notification
            if (remote_stream->buffer.getResourceMonitor().filledSize() == data_size)
                mReceiveListener->networkReceivedData( remote_stream->endpoint );
        }
    }

    return Sirikata::Network::Stream::AcceptedData;
}

void TCPNetwork::readySendCallback(const Address4 &addr) {
    //not sure what to do here since we don't have the push/pull style notifications
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


void TCPNetwork::markDisconnected(const Address4& addr) {
    // When we get a disconnection signal, there's nothing we can even do
    // anymore -- the other side has closed the connection for some reason and
    // isn't going to accept any more data.  The connection has already been
    // marked as disconnected and shutting down, we just have to do the removal
    // from the main thread.

    RemoteStreamMap::iterator it = mRemoteStreams.find(addr);
    // We may not even have the connection anymore, either because we explicitly
    // removed it or the disconnection event came for a closing stream.
    if (it == mRemoteStreams.end())
        return;

    RemoteStreamPtr remote_stream = it->second;
    assert(remote_stream &&
           !remote_stream->connected &&
           remote_stream->shutting_down);

    mRemoteStreams.erase(addr);
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

void TCPNetwork::addNewStream(const Address4& remote_endpoint, RemoteStreamPtr remote_stream) {
    RemoteStreamMap::iterator it = mRemoteStreams.find(remote_endpoint);

    // If there's already a stream in the map, this incoming stream conflicts
    // with one we started in the outgoing direction.  We need to cancel one and
    // save the other in our map.
    if (it != mRemoteStreams.end()) {
        SILOG(tcpnetwork,insane,"Resolving multiple conflicting connections.");

        RemoteStreamPtr new_remote_stream = remote_stream;
        RemoteStreamPtr existing_remote_stream = it->second;

        // The choice of which connection to maintain is arbitrary, but needs to
        // be symmetric.
        RemoteStreamPtr stream_to_save;
        RemoteStreamPtr stream_to_close;
        if (mListenAddress < remote_endpoint) {
            stream_to_save = existing_remote_stream;
            stream_to_close = new_remote_stream;
        }
        else {
            stream_to_save = new_remote_stream;
            stream_to_close = existing_remote_stream;
        }

        mRemoteStreams[remote_endpoint] = stream_to_save;
        // We should never have 2 closing streams at the same time or else the
        // remote end is misbehaving
        assert(mClosingStreams.find(remote_endpoint) == mClosingStreams.end());
        stream_to_close->shutting_down = true;
        mClosingStreams[remote_endpoint] = stream_to_close;

        Sirikata::Network::IOTimerPtr timer = Sirikata::Network::IOTimer::create(mIOService);
        timer->wait(
            Duration::seconds(5),
            std::tr1::bind(&TCPNetwork::handleClosingStreamTimeout, this, timer, remote_endpoint)
                   );

        mClosingStreamTimers.insert(timer);

        return;
    }

    // Otherwise this is the only connection for this endpoint we know about,
    // just store it
    mRemoteStreams[remote_endpoint] = remote_stream;
}

void TCPNetwork::handleClosingStreamTimeout(Sirikata::Network::IOTimerPtr timer, const Address4& remote_endpoint) {
    SILOG(tcpnetwork,insane,"Closing stream due to timeout.");

    mClosingStreamTimers.erase(timer);


    RemoteStreamMap::iterator it = mClosingStreams.find(remote_endpoint);
    if (it == mClosingStreams.end()) return;

    RemoteStreamPtr remote_stream = it->second;
    mClosingStreams.erase(it);
    remote_stream->connected = false;
    remote_stream->stream->close();
}

TCPNetwork::RemoteStreamPtr TCPNetwork::getReceiveQueue(const Address4& addr) {
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
        // Previous checks should still be valid, just return it
        return front_it->second;
    }

    RemoteStreamMap::iterator closing_it = mClosingStreams.find(addr);
    if (closing_it != mClosingStreams.end()) {
        RemoteStreamPtr result = closing_it->second;
        if (result &&
            result->buffer.getResourceMonitor().filledSize() > 0 &&
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
            result->buffer.getResourceMonitor().filledSize() > 0 &&
            (result->connected || result->shutting_down)
            ) {
            mFrontStreams[addr] = result;
            return result;
        }
    }

    return RemoteStreamPtr();
}

void TCPNetwork::clearReceiveQueue(const Address4& addr) {
    mFrontStreams.erase(addr);
}



bool TCPNetwork::canSend(const Address4& addr, uint32 size) {
    RemoteStreamMap::iterator where = mRemoteStreams.find(addr);

    // If we don't have a connection yet, we'll use this as a hint to make the
    // connection, but return false since a call to send() would fail
    if (where == mRemoteStreams.end()) {
        SILOG(tcpnetwork,insane,"Initiating new connection in canSend()");

        RemoteStreamPtr remote(
            new RemoteStream(
                this, StreamFactory::getSingleton().getConstructor(mStreamPlugin)(mIOService,mSendOptions)
                             )
                               );
        RemoteStreamWPtr weak_remote(remote);

        // Insert before calling connect to ensure data is in place when event
        // occurs.  Note that remote->connected is still false here so we won't
        // prematurely use the connection for sending
        remote->endpoint = addr;
        mRemoteStreams.insert(std::pair<Address4,RemoteStreamPtr>(addr, remote));
        where = mRemoteStreams.find(addr);

        Stream::SubstreamCallback sscb(&Stream::ignoreSubstreamCallback);
        Stream::ConnectionCallback connCallback(std::tr1::bind(&TCPNetwork::connectionCallback, this, addr, weak_remote, _1, _2));
        Stream::ReceivedCallback br(&Stream::ignoreReceivedCallback);
        Stream::ReadySendCallback readySendCallback(std::tr1::bind(&TCPNetwork::readySendCallback, this, addr));
        remote->stream->connect(convertAddress4ToSirikata(addr),
                                sscb,
                                connCallback,
                                br,
                                readySendCallback);

        // Note: Initial connection header is sent upon successful connection

        return false;
    }

    RemoteStreamPtr remote(where->second);
    if (remote->connected &&
        !remote->shutting_down &&
        remote->stream->canSend(size))
        return true;

    return false;
}

bool TCPNetwork::send(const Address4&addr, const Chunk& data) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    RemoteStreamMap::iterator where = mRemoteStreams.find(addr);

    if (where == mRemoteStreams.end())
        return false;

    RemoteStreamPtr remote(where->second);
    bool success = (remote->connected &&
                    !remote->shutting_down &&
                    remote->stream->send(data,ReliableOrdered));
    return success;
}


void TCPNetwork::listen(const Address4& as_server, ReceiveListener* receive_listener) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    mReceiveListener = receive_listener;

    mListenAddress = as_server;
    Address listenAddress(convertAddress4ToSirikata(as_server));
    mListener->listen(
        Address(listenAddress.getHostName(),listenAddress.getService()),
        std::tr1::bind(&TCPNetwork::newStreamCallback,this,_1,_2)
                      );
}


Chunk* TCPNetwork::front(const Address4& from, uint32 max_size) {
    RemoteStreamPtr stream(getReceiveQueue(from));

    if (!stream)
        return NULL;

    // If we need to pull into the front buffer, do so
    if (!stream->front)  {
        Chunk **frontptr = &stream->front;
        bool popped = stream->buffer.pop(*frontptr);
        if (stream->paused) {
            stream->paused = false;
            stream->stream->readyRead();
        }
    }

    // Only return the front item if it doesn't violate size requirements
    if (stream->front &&
        stream->front->size() <= max_size) {
        return stream->front;
    }

    return NULL;
}

Chunk* TCPNetwork::receiveOne(const Address4&from, uint32 max_size) {
    // Use front() to get the next one.  We have a slight duplication of work in
    // calling getReceiveQueue twice...
    Chunk* result = front(from, max_size);

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
