
#include "JSCapabilitiesConsts.hpp"
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/Platform.hpp>
#include "JSPresenceStruct.hpp"
#include "../JSLogging.hpp"

namespace Sirikata{
namespace JS{

Capabilities::CapNum Capabilities::getFullCapabilities()
{
    return SEND_MESSAGE|RECEIVE_MESSAGE|IMPORT|CREATE_PRESENCE|CREATE_ENTITY|EVAL|PROX_CALLBACKS|PROX_QUERIES|CREATE_SANDBOX|GUI|HTTP|MOVEMENT|MESH;
}



bool Capabilities::presenceSpecificCap(Caps checkingCap)
{
    if ((checkingCap == SEND_MESSAGE)   || (checkingCap == RECEIVE_MESSAGE) ||
        (checkingCap == PROX_CALLBACKS) || (checkingCap == PROX_QUERIES)    ||
        (checkingCap == CREATE_SANDBOX) || (checkingCap == MOVEMENT)        ||
        (checkingCap == MESH))
    {
        return true;
    }

    return false;
}


bool Capabilities::givesCap(CapNum capabilitiesNum, Caps checkingCap,
    JSPresenceStruct* ctxPres, JSPresenceStruct* onPres)
{
    if (! presenceSpecificCap(checkingCap))
        return (capabilitiesNum & checkingCap);

    //means its a presence-specific capability: should only return false if
    //trying to perform operation on presence passed through to sandbox and
    //if capability is not granted to that sandbox.

    if (onPres == NULL)
        JSLOG(error, "Error in checking whether given a capability.  "  <<\
            "Checking a presence-specific capability, but provided no " <<\
            "presence to check against.");
    
    //ctxPres can be null when executing from root context
    if ((ctxPres == NULL) || (onPres == NULL))
        return true;
        
    if (ctxPres == onPres)
    {
        //check if given capability
        return (capabilitiesNum & checkingCap);
    }

    return true;
}



} //end namespace js
} //end namespace sirikata.
