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

    void registerOnProxRemovedEventHandler(v8::Persistent<v8::Function>& cb)
    {
        mOnProxRemovedEventHandler = cb;
    }
    
    void registerOnProxAddedEventHandler(v8::Persistent<v8::Function>& cb)
    {
        mOnProxAddedEventHandler = cb;
    }
    
    
    JSObjectScript* jsObjScript;
    SpaceObjectReference* sporef;
    v8::Persistent<v8::Function> mOnProxRemovedEventHandler;
    v8::Persistent<v8::Function> mOnProxAddedEventHandler;
    
};


}//end namespace js
}//end namespace sirikata

#endif
