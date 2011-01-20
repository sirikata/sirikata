
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../JSObjectScript.hpp"

namespace Sirikata {
namespace JS {

JSPresenceStruct::JSPresenceStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef)
        : jsObjScript(parent),
        sporef(new SpaceObjectReference(_sporef))
        {}

JSPresenceStruct::~JSPresenceStruct()
{
    for (ContextVector::iterator iter = associatedContexts.begin(); iter != associatedContexts.end(); ++iter)
        (*iter)->presenceDied();
    
    delete sporef;
}


v8::Handle<v8::Value> JSPresenceStruct::script_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries)
{
    return jsObjScript->createContext(this,canMessage,sendEveryone,recvEveryone,proxQueries);
}


void JSPresenceStruct::addAssociatedContext(JSContextStruct* toAdd)
{
    associatedContexts.push_back(toAdd);
}

void JSPresenceStruct::registerOnProxRemovedEventHandler(v8::Persistent<v8::Function>& cb)
{
    mOnProxAddedEventHandler = cb;
}
    
void JSPresenceStruct::registerOnProxAddedEventHandler(v8::Persistent<v8::Function>& cb)
{
    mOnProxRemovedEventHandler = cb;
}


v8::Handle<v8::Value> JSPresenceStruct::script_getPosition()
{
    return jsObjScript->getPositionFunction(sporef);
}


v8::Handle<v8::Value> JSPresenceStruct::script_setVelocity(const Vector3f& newVel)
{
    jsObjScript->setVelocityFunction(sporef,newVel);
    return v8::Undefined();
}

JSPresenceStruct* decodePresenceStruct(v8::Handle<v8::Value> toDecode ,String& errorMessage)
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
    v8::Local<v8::External> wrapJSPresStructObj;
    wrapJSPresStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(PRESENCE_FIELD_PRESENCE));
    void* ptr = wrapJSPresStructObj->Value();
    
    JSPresenceStruct* returner;
    returner = static_cast<JSPresenceStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of JSPresneceStruct.  Internal field of object given cannot be casted to a JSPresenceStruct.";

    return returner;

}


} //namespace JS
} //namespace Sirikata
