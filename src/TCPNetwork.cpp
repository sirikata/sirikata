#include "TCPNetwork.hpp"
#include "sirikata/network/IOServiceFactory.hpp"
#include "sirikata/network/StreamFactory.hpp"
#include "sirikata/network/StreamListenerFactory.hpp"
#include "sirikata/network/StreamListener.hpp"
#include "boost/thread.hpp"
using namespace Sirikata::Network;
using namespace Sirikata;

namespace CBR {

TCPNetwork::TCPNetwork(Trace* trace, uint32 incomingBufferLength, uint32 incomingBandwidth, uint32 outgoingBandwidth) {
    mIncomingBandwidth=incomingBandwidth;
    mOutgoingBandwidth=outgoingBandwidth;
    mIncomingBufferLength=incomingBufferLength;
    mTrace=trace;
    mPluginManager.load(Sirikata::DynamicLibrary::filename("tcpsst"));
    mIOService=IOServiceFactory::makeIOService();
    mListener=StreamListenerFactory::getSingleton().getDefaultConstructor()(mIOService);   
    mThread= new boost::thread(std::tr1::bind(&IOServiceFactory::runService,mIOService));    

}


void TCPNetwork::newStreamCallback(Stream*newStream, Stream::SetCallbacks&setCallbacks) {
    Address4 sourceAddress(newStream->getRemoteEndpoint());
    dbl_ptr_queue newitem(new std::tr1::shared_ptr<TSQueue>(new TSQueue(this,newStream)));
    weak_dbl_ptr_queue weak_newitem(newitem);
    mNewReceiveBuffers.push(std::pair<Address4,dbl_ptr_queue>(sourceAddress,newitem));
    setCallbacks(
                 std::tr1::bind(&TCPNetwork::receivedConnectionCallback,this,weak_newitem,_1,_2),
                 std::tr1::bind(&TCPNetwork::bytesReceivedCallback,this,weak_newitem,_1),
                 &Stream::ignoreReadySend);

}


void TCPNetwork::sendStreamConnectionCallback(const Address4&addy, const Sirikata::Network::Stream::ConnectionStatus status, const std::string&reason){
    if (status!=Sirikata::Network::Stream::Connected) {
        mDisconnectedStreams.push(addy);
    }
}
void TCPNetwork::readySendCallback(const Address4 &addy) {
    //not sure what to do here since we don't have the push/pull style notifications
}


Address convertAddress4ToSirikata(const Address4&addy) {
    std::stringstream port;
    port << addy.getPort();
    std::stringstream hostname;
    uint32 mynum=addy.ip;
    unsigned char bleh[4];
    memcpy(bleh,&mynum,4);
    
    hostname << bleh[0]<<'.'<<bleh[1]<<'.'<<bleh[2]<<'.'<<bleh[3];
    
    return Address(hostname.str(),port.str());
}
bool TCPNetwork::canSend(const Address4&addy,uint32 size, bool reliable, bool ordered, int priority){
    std::tr1::unordered_map<Address4,Stream*>::iterator where;
    if ((where=mSendStreams.find(addy))==mSendStreams.end()) {
        return true;
    }
    return where->second->canSend(size);
}
bool TCPNetwork::send(const Address4&addy, const Chunk& data, bool reliable, bool ordered, int priority) {
    std::tr1::unordered_map<Address4,Stream*>::iterator where;
    if ((where=mSendStreams.find(addy))==mSendStreams.end()) {
        mSendStreams[addy]=StreamFactory::getSingleton().getDefaultConstructor()(mIOService);        
        where=mSendStreams.find(addy);

        Stream::ConnectionCallback connectionCallback(std::tr1::bind(&TCPNetwork::sendStreamConnectionCallback,this,addy,_1,_2));
        Stream::SubstreamCallback sscb(&Stream::ignoreSubstreamCallback);
        Stream::BytesReceivedCallback br(&Stream::ignoreBytesReceived);
        Stream::ReadySendCallback readySendCallback(std::tr1::bind(&TCPNetwork::readySendCallback,this,addy));
        
        where->second->connect(convertAddress4ToSirikata(addy),
                               sscb,
                               connectionCallback,
                               br,
                               readySendCallback);
    }
    return where->second->send(data,reliable&&ordered?ReliableOrdered:ReliableUnordered);
}
void TCPNetwork::listen(const Address4&as_server) {
    std::stringstream ss;
    ss<<as_server.getPort();
    mListener->listen(Address("127.0.0.1",ss.str()),
                      std::tr1::bind(&TCPNetwork::newStreamCallback,this,_1,_2));
}
void TCPNetwork::init(void *(*x)(void*data)) {
    x(NULL);
}
TCPNetwork::~TCPNetwork() {
    IOServiceFactory::stopService(mIOService);
    ((boost::thread*)mThread)->join();
    delete (boost::thread*)mThread;
    IOServiceFactory::destroyIOService(mIOService);
    mIOService=NULL;
}
void TCPNetwork::start() {
    
}

std::tr1::shared_ptr<TCPNetwork::TSQueue> TCPNetwork::getQueue(const Address4&addy){
  get_queue:
    std::tr1::unordered_map<Address4,dbl_ptr_queue>::iterator where=mReceiveBuffers.find(addy);
    if (where!=mReceiveBuffers.end()) {
        if (where->second) {
            std::tr1::shared_ptr<TSQueue> retval(*where->second);
            if (retval) {
                return retval;
            }else {
                mReceiveBuffers.erase(where);
                goto refresh_new_buffers;
            }
        }else {
            mReceiveBuffers.erase(where);
            goto refresh_new_buffers;
        }
    }else {
        goto refresh_new_buffers;
    }
  refresh_new_buffers://why use goto..cus I'm curious if people read my code :-) I promise this is the only goto I've written since I got here...just making sure you pay attention
    std::pair<Address4,dbl_ptr_queue> toPop;
    bool found=false;
    while (mNewReceiveBuffers.pop(toPop)) {
        mReceiveBuffers.insert(toPop);
        if (toPop.first==addy) found=true;
    }
    if (found) goto get_queue;
    return std::tr1::shared_ptr<TSQueue>();
}
void TCPNetwork::reportQueueInfo(const Time&)const {}
void TCPNetwork::service(const Time&){}
Chunk* TCPNetwork::front(const Address4&from, uint32 max_size) {
    std::tr1::shared_ptr<TSQueue> front(getQueue(from));
    if (front) {
        if (!front->front)  {
            Chunk **frontptr=&front->front;
            front->buffer.pop(*frontptr);
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
            front->buffer.pop(front->front);
            if (front->paused) {
                front->paused=false;
                front->stream->readyRead();
            }
        }
        if (front->front&&front->front->size()<=max_size) {
            Chunk * retval=front->front;
            front->front=NULL;
            return retval;;        
        }
    }
    return NULL;
}

bool TCPNetwork::bytesReceivedCallback(const weak_dbl_ptr_queue&weak_queue, Chunk&data){
    dbl_ptr_queue queue(weak_queue.lock());
    if (queue&&*queue){
        Chunk * tmp=new Chunk;
        tmp->swap(data);
        if (!(*queue)->buffer.push(tmp,false)) {
            (*queue)->paused=true;
            delete tmp;
            return false;
        }
    }
    return true;
}
void TCPNetwork::receivedConnectionCallback(const weak_dbl_ptr_queue&weak_queue, const Sirikata::Network::Stream::ConnectionStatus status, const std::string&reason){
    dbl_ptr_queue queue(weak_queue.lock());
    if (queue&&*queue){
        if (status!=Sirikata::Network::Stream::Connected) {
            *queue=std::tr1::shared_ptr<TSQueue>();
        }
    }
}
}
