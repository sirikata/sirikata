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

void noop(){}


TCPNetwork::SendStream::SendStream(Sirikata::Network::Stream* stream)
 : stream(stream),
   connected(false)
{
}


TCPNetwork::TSQueue::TSQueue(TCPNetwork* parent, Sirikata::Network::Stream*strm)
 : stream(strm),
   front(NULL),
   buffer(Sirikata::SizedResourceMonitor(parent->mIncomingBufferLength)),
   inserted(false),
   paused(false)
{
}




TCPNetwork::TCPNetwork(SpaceContext* ctx, uint32 incomingBufferLength, uint32 incomingBandwidth, uint32 outgoingBandwidth)
 : Network(ctx),
   mIncomingBufferLength(incomingBufferLength),
   mIncomingBandwidth(incomingBandwidth),
   mOutgoingBandwidth(outgoingBandwidth)
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

    Address4 sourceAddress(newStream->getRemoteEndpoint());
    dbl_ptr_queue newitem(new std::tr1::shared_ptr<TSQueue>(new TSQueue(this,newStream)));
    weak_dbl_ptr_queue weak_newitem(newitem);

    setCallbacks(
        std::tr1::bind(&TCPNetwork::receivedConnectionCallback,this,weak_newitem,_1,_2),
        std::tr1::bind(&TCPNetwork::bytesReceivedCallback,this,weak_newitem,_1),
        &Stream::ignoreReadySendCallback
    );

    mPendingReceiveBuffers[sourceAddress] = newitem;
}


void TCPNetwork::sendStreamConnectionCallback(const Address4& addr, const Sirikata::Network::Stream::ConnectionStatus status, const std::string&reason){
    if (status == Sirikata::Network::Stream::Disconnected) {
        mContext->mainStrand->post(
            std::tr1::bind(&TCPNetwork::markSendDisconnected, this, addr)
        );
    }
    else if (status == Sirikata::Network::Stream::Connected) {
        mContext->mainStrand->post(
            std::tr1::bind(&TCPNetwork::markSendConnected, this, addr)
        );
    }
    else if (status == Sirikata::Network::Stream::ConnectionFailed) {
        mContext->mainStrand->post(
            std::tr1::bind(&TCPNetwork::markSendDisconnected, this, addr)
        );
    }
    else {
        SILOG(tcpnetwork,warning,"Unhandled send stream connection status: " << status << " -- " << reason);
    }
}

void TCPNetwork::readySendCallback(const Address4 &addr) {
    //not sure what to do here since we don't have the push/pull style notifications
}

Sirikata::Network::Stream::ReceivedResponse TCPNetwork::bytesReceivedCallback(const weak_dbl_ptr_queue& weak_queue, Chunk&data){
    dbl_ptr_queue queue(weak_queue.lock());
    if (queue&&*queue){
        if ((*queue)->inserted == false) {
            // Remove from the list of pending connections
            Address4 source_addr = (*queue)->stream->getRemoteEndpoint();
            ReceiveMap::iterator it = mPendingReceiveBuffers.find(source_addr);
            if (it == mPendingReceiveBuffers.end()) {
                SILOG(tcpnetwork,error,"Address for connection not found in pending receive buffers"); // FIXME print addr
            }
            else {
                mPendingReceiveBuffers.erase(it);
            }

            // Special case, we're receiving the header. This tells use who the remote side is.
            CBR::Protocol::Server::ServerIntro intro;
            bool parsed = parsePBJMessage(&intro, data);
            assert(parsed);

            Address4* remote_endpoint = mServerIDMap->lookupInternal( intro.id() );
            assert(remote_endpoint != NULL);

            mContext->mainStrand->post(
                std::tr1::bind(&TCPNetwork::addNewStream,
                    this,
                    *remote_endpoint,
                    queue
                )
            );

            (*queue)->inserted = true;
        }
        else {
            // Normal case, we can just handle the message
            Chunk* tmp = new Chunk;
            tmp->swap(data);
            if (!(*queue)->buffer.push(tmp,false)) {
                (*queue)->paused=true;
                tmp->swap(data);
                delete tmp;
                return Sirikata::Network::Stream::PauseReceive;
            }
        }
    }

    return Sirikata::Network::Stream::AcceptedData;
}

void TCPNetwork::receivedConnectionCallback(const weak_dbl_ptr_queue&weak_queue, const Sirikata::Network::Stream::ConnectionStatus status, const std::string&reason){
    dbl_ptr_queue queue(weak_queue.lock());
    if (queue&&*queue){
        if (status != Sirikata::Network::Stream::Connected) {
            *queue=std::tr1::shared_ptr<TSQueue>();
        }
    }
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


void TCPNetwork::markSendConnected(const Address4& addr) {
    TCPStreamMap::iterator where = mSendStreams.find(addr);
    if (where == mSendStreams.end())
        return;

    // The first message needs to be our introduction
    CBR::Protocol::Server::ServerIntro intro;
    intro.set_id(mContext->id());
    std::string serializedIntro;
    serializePBJMessage(&serializedIntro, intro);

    where->second.stream->send(
        MemoryReference(serializedIntro),
        ReliableOrdered
    );

    // And after we're sure its sent, open things up for everybody else
    where->second.connected = true;
}

void TCPNetwork::markSendDisconnected(const Address4& addr) {
    // FIXME what should we do about this
}

void TCPNetwork::addNewStream(const Address4& remote_endpoint, dbl_ptr_queue source_queue) {
    // Make sure the dbl_ptr is still valid
    if (!source_queue || !(*source_queue)) {
        return;
    }

    assert(mReceiveBuffers.find(remote_endpoint) == mReceiveBuffers.end());
    mReceiveBuffers[remote_endpoint] = source_queue;
}

TCPNetwork::ptr_queue TCPNetwork::getQueue(const Address4& addr){
    std::tr1::unordered_map<Address4,dbl_ptr_queue>::iterator where = mReceiveBuffers.find(addr);
    if (where == mReceiveBuffers.end())
        return std::tr1::shared_ptr<TSQueue>();

    if (!where->second) {
        mReceiveBuffers.erase(where);
        return std::tr1::shared_ptr<TSQueue>();
    }

    std::tr1::shared_ptr<TSQueue> retval(*where->second);
    if (!retval) {
        mReceiveBuffers.erase(where);
        return std::tr1::shared_ptr<TSQueue>();
    }

    return retval;
}


bool TCPNetwork::canSend(const Address4& addr,uint32 size, bool reliable, bool ordered, int priority) {
    TCPStreamMap::iterator where = mSendStreams.find(addr);
    if (where == mSendStreams.end())
        return true;

    if (where->second.connected && where->second.stream->canSend(size))
        return true;

    return false;
}

bool TCPNetwork::send(const Address4&addr, const Chunk& data, bool reliable, bool ordered, int priority) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    TCPStreamMap::iterator where = mSendStreams.find(addr);
    if (where == mSendStreams.end()) {
        mSendStreams.insert(std::pair<Address4,SendStream>(
                addr,
                SendStream(StreamFactory::getSingleton().getConstructor(mStreamPlugin)(mIOService,mSendOptions))
            ));
        where = mSendStreams.find(addr);

        Stream::ConnectionCallback connectionCallback(std::tr1::bind(&TCPNetwork::sendStreamConnectionCallback,this,addr,_1,_2));
        Stream::SubstreamCallback sscb(&Stream::ignoreSubstreamCallback);
        Stream::ReceivedCallback br(&Stream::ignoreReceivedCallback);
        Stream::ReadySendCallback readySendCallback(std::tr1::bind(&TCPNetwork::readySendCallback,this,addr));
        where->second.stream->connect(convertAddress4ToSirikata(addr),
                               sscb,
                               connectionCallback,
                               br,
                               readySendCallback);
    }

    if (where->second.connected &&
        //where->second.stream->send(data,reliable&&ordered?ReliableOrdered:ReliableUnordered)) {
        where->second.stream->send(data,ReliableOrdered)) {
        return true;
    }else {
        return false;
    }
}

void TCPNetwork::listen(const Address4&as_server) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    mListenAddress = as_server;
    Address listenAddress(convertAddress4ToSirikata(as_server));
    mListener->listen(Address(listenAddress.getHostName(),listenAddress.getService()),
                      std::tr1::bind(&TCPNetwork::newStreamCallback,this,_1,_2));
}

void TCPNetwork::init(void *(*x)(void*data)) {
    x(NULL);
}

void TCPNetwork::begin() {
}

void TCPNetwork::reportQueueInfo() const {
}

Chunk* TCPNetwork::front(const Address4&from, uint32 max_size) {
    std::tr1::shared_ptr<TSQueue> front(getQueue(from));
    if (front) {
        if (!front->front)  {
            Chunk **frontptr=&front->front;
            bool popped=front->buffer.pop(*frontptr);
            if (front->paused) {
                front->paused=false;
                front->stream->readyRead();
            }
        }
        if (front->front&&front->front->size()<=max_size) {
            return front->front;
        }
    }

    return NULL;
}

Chunk* TCPNetwork::receiveOne(const Address4&from, uint32 max_size) {
    std::tr1::shared_ptr<TSQueue> front(getQueue(from));
    if (front) {
        if (!front->front) {
            bool popped=front->buffer.pop(front->front);
            if (front->paused) {
                front->paused=false;
                front->stream->readyRead();
            }
        }
        if (front->front&&front->front->size()<=max_size) {
            Chunk * retval=front->front;
            front->front=NULL;
            return retval;
        }
    }
    return NULL;
}

} // namespace Sirikata
