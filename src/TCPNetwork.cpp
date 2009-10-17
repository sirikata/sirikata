#include "TCPNetwork.hpp"
using namespace Sirikata::Network;

namespace CBR {

TCPNetwork::TCPNetwork(Trace* trace, uint32 incomingBufferLength, uint32 incomingBandwidth, uint32 outgoingBandwidth) {
    memset(mReceiveBuffers,0,sizeof(TSQueue*)*MAX_RECEIVE_STREAMS);
    mIncomingBandwidth=incomingBandwidth;
    mOutgoingBandwidth=outgoingBandwidth;
    mIncomingBufferLength=incomingBufferLength;
    mTrace=trace;
    mPluginManager.load(Sirikata::DynamicLibrary::filename("tcpsst"));
    mIOService=IOServiceFactory::makeIOService();
    mListener=StreamListenerFactory::getSingleton().getDefaultConstructor()(mIOService);   
    mThread= new boost::thread(std::tr1::bind(&IOServiceFactory::runService,mIOService));    
    mCurNumStreams=0;
}
std::tr1::shared_ptr<TSQueue>getSharedQueue(const Address4&addy) {
    std::tr1::shared_ptr<TSQueue>retval;
    std::tr1::unordered_map<Address4,unsigned int>::iterator where=mReceiveBufferMapping.find(addy);
    if (mNewReceiveBuffers.probablyEmpty()==false||where==mReceiveBufferMapping.end()||!mReceiveBuffers[where->second]) {
        std::pair<Address4,unsigned int> ret;
        while (mNewReceiveBuffers.pop(ret)) {
            mReceiveBufferMapping.insert(ret);
        }
        where=mReceiveBufferMapping.find(addy);
    }
    if (where!=mReceiveBufferMapping.end()&&(retval=mReceiveBuffers[where->second])) {
        return retval;
    }
    return std::tr1::shared_ptr<TSQueue>();
}
void TCPNetwork::newStreamCallback(Stream*newStream, Stream::SetCallbacks&setCallbacks) {
    Address4 sourceAddress(newStream->getRemoteEndpoint());
    assert(mCurNumStreams<MAX_RECEIVE_STREAMS);
    if (mCurNumStreams<MAX_RECEIVE_STREAMS) {
        mNewReceiveBuffers.push(std::pair<Address4,unsigned int>(sourceAddress,mCurNumStreams));
        std::tr1::shared_ptr<TSQueue>newitem(new TSQueue(this,newStream));
        mReceiveBuffers[mCurNumStreams++]=newitem;
    }else{ 
        newStream->close();
        delete newStream();
    }
}
void TCPNetwork::listen(const Address4&as_server) {
    mListener->listen(Address("127.0.0.1",as_server.getPort()),
                      std::tr1::bind(&TCPNetwork::newStreamCallback,_1,_2));
}
void TCPNetwork::init(void *(*x)(void*data)) {
    x(NULL);
}
TCPNetwork::TCPNetwork() {
    mIOService->stopService();
    mThread->join();
    delete mThread;
    IOServiceFactory::destroyIOService(mIOService);
    mIOService=NULL;
}
void TCPNetwork::start() {
    
}



}
