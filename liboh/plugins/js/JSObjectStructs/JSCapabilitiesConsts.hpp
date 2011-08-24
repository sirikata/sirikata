
#ifndef __JSCAPABILITIES_CONSTS_HPP__
#define __JSCAPABILITIES_CONSTS_HPP__

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata{
namespace JS{

/** Sister file in em, is in scripts/std/shim/capabilities.em.  Make change to
 * this file, means I should make change to that file.*/
struct Capabilities
{
    const static uint32 SEND_MESSAGE    = 1;
    const static uint32 RECEIVE_MESSAGE = 2;
    const static uint32 IMPORT          = 4;
    const static uint32 CREATE_PRESENCE = 8;
    const static uint32 CREATE_ENTITY   = 16;
    const static uint32 EVAL            = 32;
    const static uint32 PROX_CALLBACKS  = 64;
    const static uint32 PROX_QUERIES    = 128;
    const static uint32 CREATE_SANDBOX  = 256;
    const static uint32 GUI             = 512;
    const static uint32 HTTP            = 1024;
    const static uint32 MOVEMENT        = 2048;
    const static uint32 MESH            = 4096;
    
    static uint32 getFullCapabilities();

    //returns true if capabilitiesNum permits checkingCap to occur
    static bool givesCap(uint32 capabilitiesNum, uint32 checkingCap);

};


}//js
}//sirikata

#endif
