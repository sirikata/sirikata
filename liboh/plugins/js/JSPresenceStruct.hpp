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


    static JSPresenceStruct* decodePresenceStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage)
    {
        v8::HandleScope handle_scope;  //for garbage collection.
    
        if (! toDecode->IsObject())
        {
            errorMessage += "Error in decode of JSPresneceStruct.  Should have received an object to decode.";
            return NULL;
        }
        
        v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();
        
        //now check internal field count
        if (toDecodeObject->InternalFieldCount() != PRESENCE_FIELD_COUNT)
        {
            errorMessage += "Error in decode of JSPresneceStruct.  Object given does not have adequate number of internal fields for decode.";
            return NULL;
        }
        
        //now actually try to decode each.
        //decode the jsVisibleStruct field
        v8::Local<v8::External> wrapJSVisibleObj;
        wrapJSPresStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(PRESENCE_FIELD_PRESENCE));
        void* ptr = wrapJSPresStructObj->Value();
        
        JSPresenceStruct* returner;
        returner = static_cast<JSPresenceStruct*>(ptr);
        if (returner == NULL)
            errorMessage += "Error in decode of JSPresneceStruct.  Internal field of object given cannot be casted to a JSPresenceStruct.";

        return returner;

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
