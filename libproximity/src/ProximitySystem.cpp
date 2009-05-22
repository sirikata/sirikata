#include "proximity/Platform.hpp"
#include "util/ObjectReference.hpp"
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


const char *const* ProximitySystem::relevantUnfilteredMessages()const{
    static const char* retval[]={"ObjLoc","RetObj","DelObj"};
    return &retval[0];
}
size_t ProximitySystem::relevantUnfilteredMessagesSize()const{
    return 3;
}
const char *const* ProximitySystem::relevantFilteredMessages()const{
    static const char* retval[]={"NewProxQuery","DelProxQuery"};
    return &retval[0];
}
size_t ProximitySystem::relevantFilteredMessagesSize()const{
    return 2;
}
/*
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
*/


} }
