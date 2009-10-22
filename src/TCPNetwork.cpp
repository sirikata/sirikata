#include "TCPNetwork.hpp"
#include "sirikata/network/IOServiceFactory.hpp"
#include "sirikata/network/IOService.hpp"
#include "sirikata/network/StreamFactory.hpp"
#include "sirikata/network/StreamListenerFactory.hpp"
#include "sirikata/network/StreamListener.hpp"
#include "boost/thread.hpp"
using namespace Sirikata::Network;
using namespace Sirikata;

namespace CBR {
void noop(){}
TCPNetwork::TCPNetwork(Trace* trace, uint32 incomingBufferLength, uint32 incomingBandwidth, uint32 outgoingBandwidth) {
    mIncomingBandwidth=incomingBandwidth;
    mOutgoingBandwidth=outgoingBandwidth;
    mIncomingBufferLength=incomingBufferLength;
    mTrace=trace;
    mPluginManager.load(Sirikata::DynamicLibrary::filename("tcpsst"));
    mIOService=IOServiceFactory::makeIOService();
    mIOService->post(Duration::seconds(60*60*24*365*68),&noop);
    mListener=StreamListenerFactory::getSingleton().getDefaultConstructor()(mIOService);
    mThread= new boost::thread(std::tr1::bind(&IOService::run,mIOService));
    //mThread= new boost::thread(&noop);//std::tr1::bind(&IOServiceFactory::runService,mIOService));    

}


void TCPNetwork::newStreamCallback(Stream*newStream, Stream::SetCallbacks&setCallbacks) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    Address4 sourceAddress(newStream->getRemoteEndpoint());
    dbl_ptr_queue newitem(new std::tr1::shared_ptr<TSQueue>(new TSQueue(this,newStream)));
    weak_dbl_ptr_queue weak_newitem(newitem);
    mNewReceiveBuffers.push(std::pair<Address4,dbl_ptr_queue>(sourceAddress,newitem));
    //std::cout<<" Got new stream from "<<newStream->getRemoteEndpoint().toString()<<'\n';
    setCallbacks(
                 std::tr1::bind(&TCPNetwork::receivedConnectionCallback,this,weak_newitem,_1,_2),
                 std::tr1::bind(&TCPNetwork::bytesReceivedCallback,this,weak_newitem,_1),
                 &Stream::ignoreReadySend);

}


void TCPNetwork::sendStreamConnectionCallback(const Address4&addy, const Sirikata::Network::Stream::ConnectionStatus status, const std::string&reason){
    //std::cout<<" Got connection callback "<<status<<" with "<<reason<<'\n';
    if (status!=Sirikata::Network::Stream::Connected) {
        mDisconnectedStreams.push(addy);
    }else {
        mNewConnectedStreams.push(addy);
    }
}
void TCPNetwork::readySendCallback(const Address4 &addy) {
    //not sure what to do here since we don't have the push/pull style notifications
}

bool TCPNetwork::canSend(const Address4&addy,uint32 size, bool reliable, bool ordered, int priority){
    //std::cout<<"Can send "<<convertAddress4ToSirikata(addy).toString()<<" "<<size<<"?";
    std::tr1::unordered_map<Address4,SendStream>::iterator where;
    if ((where=mSendStreams.find(addy))==mSendStreams.end()) {
        //std::cout<<"#t\n";
        return true;
    }
    if (where->second.connected==false) {
        processNewConnectionsOnMainThread();
    }
    if (where->second.connected&&where->second.stream->canSend(size)) {
        //std::cout<<"yes\n";
        return true;
    }
    //std::cout<<"#f\n";
    return false;
}
char toHex(unsigned char u) {
    if (u>=0&&u<=9) return '0'+u;
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
void TCPNetwork::processNewConnectionsOnMainThread(){
    Address4 newaddy;
    while (mNewConnectedStreams.pop(newaddy)) {
        std::tr1::unordered_map<Address4,SendStream>::iterator where=mSendStreams.find(newaddy);
        if (where!=mSendStreams.end()) {
            where->second.connected=true;
        }
    }

}
bool TCPNetwork::send(const Address4&addy, const Chunk& data, bool reliable, bool ordered, int priority) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    std::tr1::unordered_map<Address4,SendStream>::iterator where;
    if ((where=mSendStreams.find(addy))==mSendStreams.end()) {
        mSendStreams.insert(std::pair<Address4,SendStream>(addy,SendStream(StreamFactory::getSingleton().getDefaultConstructor()(mIOService))));
        where=mSendStreams.find(addy);

        Stream::ConnectionCallback connectionCallback(std::tr1::bind(&TCPNetwork::sendStreamConnectionCallback,this,addy,_1,_2));
        Stream::SubstreamCallback sscb(&Stream::ignoreSubstreamCallback);
        Stream::BytesReceivedCallback br(&Stream::ignoreBytesReceived);
        Stream::ReadySendCallback readySendCallback(std::tr1::bind(&TCPNetwork::readySendCallback,this,addy));
        //std::cout<< "Connecting to "<<convertAddress4ToSirikata(addy).toString()<<'\n';
        where->second.stream->connect(convertAddress4ToSirikata(addy),
                               sscb,
                               connectionCallback,
                               br,
                               readySendCallback);
    }
    //std::cout<<"Send existing "<<convertAddress4ToSirikata(addy).toString()<<" "<<data.size()<<"\n";
    if (where->second.connected==false) {
        processNewConnectionsOnMainThread();
    }
    if (where->second.connected && 
        where->second.stream->send(data,reliable&&ordered?ReliableOrdered:ReliableUnordered)) {
        //hexPrint("Sent",data);
        return true;
    }else {
        return false;
    }
}
void TCPNetwork::listen(const Address4&as_server) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    Address listenAddress(convertAddress4ToSirikata(as_server));
    //std::cout<< "Listening on "<<listenAddress.toString()<<'\n';
    mListener->listen(Address(listenAddress.getHostName(),listenAddress.getService()),
                      std::tr1::bind(&TCPNetwork::newStreamCallback,this,_1,_2));
}
void TCPNetwork::init(void *(*x)(void*data)) {
    x(NULL);
}
TCPNetwork::~TCPNetwork() {
    mIOService->stop();
    ((boost::thread*)mThread)->join();
    delete (boost::thread*)mThread;
    IOServiceFactory::destroyIOService(mIOService);
    mIOService=NULL;
}
void TCPNetwork::start() {

}
static Address4 zeroPort(Address4 addy) {
    addy.port=0;
    return addy;
}

std::tr1::shared_ptr<TCPNetwork::TSQueue> TCPNetwork::getQueue(const Address4&addy){
  get_queue:
    std::tr1::unordered_map<Address4,dbl_ptr_queue>::iterator where=mReceiveBuffers.find(zeroPort(addy));
    if (where!=mReceiveBuffers.end()) {
        //std::cout<<"Found queue";
        if (where->second) {
            std::tr1::shared_ptr<TSQueue> retval(*where->second);
            if (retval) {
                //std::cout<<" active\n";
                return retval;
            }else {
                //std::cout<<" inactive\n";
                mReceiveBuffers.erase(where);
                goto refresh_new_buffers;
            }
        }else {
            //std::cout<<" vinactive\n";
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
        //std::cout<<"Pop found "<<convertAddress4ToSirikata(toPop.first).toString()<<'\n';
        toPop.first=zeroPort(toPop.first);
        mReceiveBuffers.insert(toPop);
        if (toPop.first==zeroPort(addy)) {
            //std::cout<<"Pop xxx\n";
            found=true;
        }
    }
    if (found) goto get_queue;
    return std::tr1::shared_ptr<TSQueue>();
}
void TCPNetwork::reportQueueInfo(const Time&)const {}
void TCPNetwork::service(const Time&){
//    mIOService->poll();
}
Chunk* TCPNetwork::front(const Address4&from, uint32 max_size) {
    std::tr1::shared_ptr<TSQueue> front(getQueue(from));
    if (front) {
        //std::cout<<"RetQ xfound\n";
        if (!front->front)  {

            Chunk **frontptr=&front->front;
            bool popped=front->buffer.pop(*frontptr);
            //std::cout<<"RetQ xnof"<<popped<<"\n";
            if (front->paused) {
                front->paused=false;
                //std::cout<<convertAddress4ToSirikata(from).toString()<<" ready read\n";
                front->stream->readyRead();
            }
        }
        if (front->front&&front->front->size()<=max_size) {
            //std::cout<<"Returning "<<front->front->size()<<" bytes from "<<convertAddress4ToSirikata(from).toString()<<'\n';
            return front->front;
        }
    }
    return NULL;
}

Chunk* TCPNetwork::receiveOne(const Address4&from, uint32 max_size) {
    std::tr1::shared_ptr<TSQueue> front(getQueue(from));
    if (front) {
        //std::cout<<"RetQ found\n";
        if (!front->front) {

            bool popped=front->buffer.pop(front->front);
            //std::cout<<"RetQ nof"<<popped<<"\n";
            if (front->paused) {
                front->paused=false;
                //std::cout<<convertAddress4ToSirikata(from).toString()<<" ready read\n";
                front->stream->readyRead();
            }
        }
        if (front->front&&front->front->size()<=max_size) {
            Chunk * retval=front->front;
            front->front=NULL;
            //std::cout<<"Returning "<<retval->size()<<" bytes from "<<convertAddress4ToSirikata(from).toString()<<'\n';
            //hexPrint("Rcvd",*retval);
            return retval;
        }
    }
    return NULL;
}

bool TCPNetwork::bytesReceivedCallback(const weak_dbl_ptr_queue&weak_queue, Chunk&data){
    dbl_ptr_queue queue(weak_queue.lock());
    if (queue&&*queue){
        
        Chunk * tmp=new Chunk;
        tmp->swap(data);
        //std::cout<<"Pushing "<<tmp->size()<<"to "<<(*queue)->stream->getRemoteEndpoint().toString()<<'\n';
        if (!(*queue)->buffer.push(tmp,false)) {
            (*queue)->paused=true;
            //std::cout<<"Push fail of "<<data.size()<< "to "<<(*queue)->stream->getRemoteEndpoint().toString()<<'\n';
            tmp->swap(data);
            delete tmp;
            return false;
        }
    }
    return true;
}
void TCPNetwork::receivedConnectionCallback(const weak_dbl_ptr_queue&weak_queue, const Sirikata::Network::Stream::ConnectionStatus status, const std::string&reason){
    //std::cout<<" Got recv cnx callback "<<status<<" with "<<reason<<'\n';
    dbl_ptr_queue queue(weak_queue.lock());
    if (queue&&*queue){
        if (status!=Sirikata::Network::Stream::Connected) {
            *queue=std::tr1::shared_ptr<TSQueue>();
        }
    }
}
}
