
#ifndef __JSCAPABILITIES_CONSTS_HPP__
#define __JSCAPABILITIES_CONSTS_HPP__

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata{
namespace JS{

class JSPresenceStruct;

/** Sister file in em, is in scripts/std/shim/capabilities.em.  Make change to
 * this file, means I should make change to that file.*/
struct Capabilities
{
    typedef int CapNum;
    
    enum Caps{
        SEND_MESSAGE    = 1,
        RECEIVE_MESSAGE = 2,
        IMPORT          = 4,
        CREATE_PRESENCE = 8,
        CREATE_ENTITY   = 16,
        EVAL            = 32,
        PROX_CALLBACKS  = 64,
        PROX_QUERIES    = 128,
        CREATE_SANDBOX  = 256,
        GUI             = 512,
        HTTP            = 1024,
        MOVEMENT        = 2048,
        MESH            = 4096
    };
    
    static CapNum getFullCapabilities();



    /**
       @param {int} capabilitiesNum Bitwise-or of all listed capabilities
       @param {Caps} checkingCap checking whether the capabilitiesNum permits
       this capability (optionally) on the presesences below
       @param {JSPresenceStruct} ctxPres Some capabilities are restricted to
       particular presences (eg. PROX_CALLBACKS).  For this set, check whether
       ctxPres is same as onPres before checking whether have capability to
       perform action.  If they aren't the same, means sandbox is trying to
       perform an action on one of the presences it created, and we can just
       permit the operation.
     */
    static bool givesCap(CapNum capabilitiesNum, Caps checkingCap,
        JSPresenceStruct* ctxPres=NULL, JSPresenceStruct* onPres=NULL);

private:
    
    //there are several presence-specific capabilities.  Ie, you can perform all of
    //these actions on presences you create yourself, but cannot perform them on
    //the presence that was passed through to your sandbox.  If checkingCap is
    //one of these capabilities, returns true.  Otherwise, return false.
    static bool presenceSpecificCap(Caps checkingCap);
    
};


}//js
}//sirikata

#endif
