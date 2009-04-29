#include <proximity/Platform.hpp>
#include "util/ObjectReference.hpp"
#include "proximity/ProximityConnection.hpp"
#include "proximity/ProximitySystem.hpp"
#include "network/TCPStream.hpp"

namespace Sirikata { namespace Proximity {
namespace {
void connectionCallback(ProximityConnection* con, Network::Stream::ConnectionStatus status, const std::string&reason) {
    if (status!=Network::Stream::Connected)
        con->streamDisconnected();
}
void readProximityMessage(std::tr1::weak_ptr<Network::Stream> mLock,
                          ProximitySystem * system,
                          const ObjectReference &object,
                          const Network::Chunk&chunk) {
    std::tr1::shared_ptr<Network::Stream> lok=mLock.lock();//make sure this proximity connection will not disappear;
    if (lok) {
        Protocol::Message msg;
        const void *chunk_data=NULL;
        if (chunk.size()) {
            chunk_data=&chunk[0];
            msg.ParsePartialFromArray(chunk_data,chunk.size());
        }
        system->processOpaqueProximityMessage(&object,msg,chunk_data,chunk.size());
        
    }
}
}

ProximityConnection::ProximityConnection(const Network::Address&addy, ProximitySystem*prox, Network::IOService&io):mParent(prox),mConnectionStream(new Network::TCPStream(io)) {
        mConnectionStream->connect(addy,
                                   &Network::Stream::ignoreSubstreamCallback,
                                   std::tr1::bind(&connectionCallback,this,_1,_2),
                                   &Network::Stream::ignoreBytesReceived);
    
}
void ProximityConnection::streamDisconnected() {
    SILOG(proximity,error,"Lost connection with proximity manager");
}
void ProximityConnection::send(const ObjectReference&obc,
                               const Protocol::IMessage&msg,
                               const void *optionalSerializedMessage,
                               const size_t optionalSerializedMessageSize) {
    ObjectStreamMap::iterator where=mObjectStreams.find(obc);
    if (where==mObjectStreams.end()) {
        SILOG(proximity,error,"Cannot locate object with OR "<<obc<<" in the proximity connection map");
    }else {
        if (optionalSerializedMessageSize){
            where->second->send(optionalSerializedMessage,optionalSerializedMessageSize,Network::ReliableOrdered);
        }else{
            std::string data;
            msg.SerializeToString(&data);
            where->second->send(data.data(),data.size(),Network::ReliableOrdered);
        }
    }
}


ProximityConnection::~ProximityConnection() {
    for (ObjectStreamMap::iterator i=mObjectStreams.begin(),ie=mObjectStreams.end();i!=ie;++i) {
        delete i->second;
    }
    std::tr1::weak_ptr<Network::Stream> lok(mConnectionStream);
    mConnectionStream=std::tr1::shared_ptr<Network::Stream>();
    while (lok.lock()) {
        //sleep? wait for callback to complete
    }
    mObjectStreams.clear();
}
void ProximityConnection::constructObjectStream(const ObjectReference&obc) {
    mObjectStreams[obc]
        = mConnectionStream->clone(&Network::Stream::ignoreConnectionStatus,
                                   std::tr1::bind(&readProximityMessage,std::tr1::weak_ptr<Network::Stream>(mConnectionStream), mParent, obc,_1));
}
void ProximityConnection::deleteObjectStream(const ObjectReference&obc) {
    ObjectStreamMap::iterator where=mObjectStreams.find(obc);
    if (where==mObjectStreams.end()) {
        SILOG(proximity,error,"Cannot locate object with OR "<<obc<<" in the proximity connection map");
    }else {
        mObjectStreams.erase(where);
    }
}
} }
