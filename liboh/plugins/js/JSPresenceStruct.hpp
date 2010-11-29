#ifndef _SIRIKATA_JS_PRESENCE_STRUCT_HPP_
#define _SIRIKATA_JS_PRESENCE_STRUCT_HPP_


#include "JSObjectScript.hpp"
#include <sirikata/oh/HostedObject.hpp>


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;

struct JSPresenceStruct
{
    JSPresenceStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef)
     : jsObjScript(parent), sporef(new SpaceObjectReference(_sporef))
    {}
    ~JSPresenceStruct() { delete sporef; }

    JSObjectScript* jsObjScript;
    SpaceObjectReference* sporef;
};


}//end namespace js
}//end namespace sirikata

#endif
