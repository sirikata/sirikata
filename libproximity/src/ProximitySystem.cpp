#include "proximity/Platform.hpp"
#include "Proximity_Sirikata.pbj.hpp"
#include "proximity/ProximitySystem.hpp"
#include "network/Stream.hpp"
//#include "proximity/ObjectSpaceBridgeProximitySystem.hpp"
//for testing only#include "proximity/BridgeProximitySystem.hpp"
namespace Sirikata { namespace Proximity {
ProximitySystem::~ProximitySystem(){}
void ProximitySystem::defaultProximityCallback(Network::Stream*strm, const ObjectReference&objectRef,const Sirikata::Protocol::IMessage&msg){
    assert(strm);
    Sirikata::Protocol::Message message;
    message.set_destination_object(objectRef.getObjectUUID());
    std::string data;
    message.SerializeToString(&data);
    strm->send(data.data(),data.size(),Network::ReliableOrdered);
}
void ProximitySystem::defaultNoAddressProximityCallback(Network::Stream*strm, const ObjectReference&objectRef,const Sirikata::Protocol::IMessage&msg){
    assert(strm);
    std::string data;
    msg.SerializeToString(&data);
    strm->send(data.data(),data.size(),Network::ReliableOrdered);
}
bool ProximitySystem::proximityRelevantMessageName(const String&msg){
    return msg=="ObjLoc"||msg=="ProxCall"||msg=="NewProxQuery"||msg=="DelProxQuery"||msg=="RetObj"||msg=="DelObj";
}
bool ProximitySystem::proximityRelevantMessage(const Sirikata::Protocol::IMessage&msg){
    int num_messages=msg.message_names_size();
    for (int i=0;i<num_messages;++i) {
        if (proximityRelevantMessageName(msg.message_names(i)))
            return true;
    }
    return false;
}



} }
