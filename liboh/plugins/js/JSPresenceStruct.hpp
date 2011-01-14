#ifndef _SIRIKATA_JS_PRESENCE_STRUCT_HPP_
#define _SIRIKATA_JS_PRESENCE_STRUCT_HPP_

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;

struct JSPresenceStruct
{
    
    JSPresenceStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef)
        : jsObjScript(parent),
        sporef(new SpaceObjectReference(_sporef))
        {}
    
    ~JSPresenceStruct() { delete sporef; }

    void registerOnProxAddedEventHandler(v8::Persistent<v8::Function>& cb)
    {
        mOnProxAddedEventHandler = cb;
    }

    
    void registerOnProxRemovedEventHandler(v8::Persistent<v8::Function>& cb)
    {
    mOnProxRemovedEventHandler = cb;
    }


    //data
    JSObjectScript* jsObjScript;
    SpaceObjectReference* sporef; //sporef associated with this presence.
    v8::Persistent<v8::Function> mOnProxRemovedEventHandler;
    v8::Persistent<v8::Function> mOnProxAddedEventHandler;
    
};


}//end namespace js
}//end namespace sirikata

#endif
