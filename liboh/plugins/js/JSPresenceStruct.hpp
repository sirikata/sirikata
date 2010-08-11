#ifndef _SIRIKATA_JS_PRESENCE_STRUCT_HPP_
#define _SIRIKATA_JS_PRESENCE_STRUCT_HPP_


#include "JSObjectScript.hpp"
#include <sirikata/oh/HostedObject.hpp>


namespace Sirikata {
namespace JS {

    class JSObjectScript;

struct JSPresenceStruct
{
    
    //I'm repsonsible for sID memory.
    SpaceID*                       sID;

    
    //likely that the jsObjScript won't be owned by JSPresence (don't delete it myself).
    JSObjectScript*        jsObjScript;
};


}//end namespace js
}//end namespace sirikata

#endif
