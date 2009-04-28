#include "proximity/Platform.hpp"
#include "proximity/ProximitySystem.hpp"
//#include "proximity/ObjectSpaceBridgeProximitySystem.hpp"
//for testing only#include "proximity/BridgeProximitySystem.hpp"
namespace Sirikata { namespace Proximity {
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
