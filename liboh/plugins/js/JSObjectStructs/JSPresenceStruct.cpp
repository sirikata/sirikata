
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "../JSLogging.hpp"

namespace Sirikata {
namespace JS {


//this constructor is called when the presence associated
JSPresenceStruct::JSPresenceStruct(JSObjectScript* parent, v8::Handle<v8::Function> connectedCallback, int presenceToken)
 :  JSWatchable(),
    mOnConnectedCallback(v8::Persistent<v8::Function>::New(connectedCallback)),
    jsObjScript(parent),
    sporef(NULL),
    isConnected(false),
    hasConnectedCallback(true),
    mPresenceToken(presenceToken)
{
}

JSPresenceStruct::JSPresenceStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef, int presenceToken)
 : JSWatchable(),
   jsObjScript(parent),
   sporef(new SpaceObjectReference(_sporef)),
   isConnected(true),
   hasConnectedCallback(false),
   mPresenceToken(presenceToken)
{
    setFlag();
}


v8::Handle<v8::Value> JSPresenceStruct::getIsConnectedV8()
{
    v8::HandleScope handle_scope;
    return v8::Boolean::New(isConnected);
}


bool JSPresenceStruct::getIsConnected()
{
    return isConnected;
}

int JSPresenceStruct::getPresenceToken()
{
    return mPresenceToken;
}

void JSPresenceStruct::connect(const SpaceObjectReference& _sporef)
{
    v8::HandleScope handle_scope;

    if (getIsConnected())
        JSLOG(error, "Error when calling connect on presence.  The presence was already connected.");

    setFlag();
    isConnected = true;
    sporef = new SpaceObjectReference(_sporef);

    if (hasConnectedCallback)
        jsObjScript->handleTimeoutContext(mOnConnectedCallback,NULL);
}

void JSPresenceStruct::clearPreviousConnectedCB()
{
    if (hasConnectedCallback)
        mOnConnectedCallback.Dispose();

    hasConnectedCallback = false;
}

v8::Handle<v8::Value> JSPresenceStruct::setOrientationFunction(Quaternion newOrientation)
{
    jsObjScript->setOrientationFunction(sporef,newOrientation);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setOrientationVelFunction(Quaternion newOrientationVel)
{
    jsObjScript->setOrientationVelFunction(sporef,newOrientationVel);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::getOrientationVelFunction()
{
    return jsObjScript->getOrientationVelFunction(sporef);
}

v8::Handle<v8::Value> JSPresenceStruct::getVisualScaleFunction()
{
    return jsObjScript->getVisualScaleFunction(sporef);
}

    
v8::Handle<v8::Value> JSPresenceStruct::setQueryAngleFunction(SolidAngle new_qa)
{
    jsObjScript->setQueryAngleFunction(sporef, new_qa);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setVisualScaleFunction(float new_scale)
{
    jsObjScript->setVisualScaleFunction(sporef, new_scale);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setPositionFunction(Vector3f newPos)
{
    setFlag();
    jsObjScript->setPositionFunction(sporef, newPos);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::getOrientationFunction()
{
    return jsObjScript->getOrientationFunction(sporef);
}

v8::Handle<v8::Value>JSPresenceStruct::getVelocityFunction()
{
    return jsObjScript->getVelocityFunction(sporef);
}

v8::Handle<v8::Value> JSPresenceStruct::distance(Vector3d* distTo)
{
    if (!getIsConnected())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling distance on presence.  The presence is not connected to any space, and therefore does not have any position.")));

    setFlag();
    
    return jsObjScript->getDistanceFunction(sporef,distTo);
}



v8::Handle<v8::Value> JSPresenceStruct::setConnectedCB(v8::Handle<v8::Function> newCB)
{
    v8::HandleScope handle_scope;
    clearPreviousConnectedCB();
    hasConnectedCallback = true;
    mOnConnectedCallback = v8::Persistent<v8::Function>::New(newCB);
    return v8::Boolean::New(true);
}

JSPresenceStruct::~JSPresenceStruct()
{
    clearPreviousConnectedCB();
    
    for (ContextVector::iterator iter = associatedContexts.begin(); iter != associatedContexts.end(); ++iter)
        (*iter)->presenceDied();

    if (sporef != NULL)
        delete sporef;
}

void JSPresenceStruct::disconnect()
{
    if (! getIsConnected())
        JSLOG(error, "Error when calling disconnect on presence.  The presence wasn't already connected.");
        
    if (sporef != NULL)
        delete sporef;

    sporef = NULL;
    isConnected = false;
}

v8::Handle<v8::Value>JSPresenceStruct::setVisualFunction(String urilocation)
{
    jsObjScript->setVisualFunction(sporef, urilocation);
    return v8::Undefined();
}

v8::Handle<v8::Value>JSPresenceStruct::getVisualFunction()
{
    return jsObjScript->getVisualFunction(sporef);    
}



/*
  GIANT FIXME on this entire function.  Likely garbage collection is incorrect.
  Doing very little through jsobjscript directly, etc.
 */
v8::Handle<v8::Value>JSPresenceStruct::runSimulation(String simname)
{
    v8::HandleScope scope;
    JSInvokableObject::JSInvokableObjectInt* invokableObj = jsObjScript->runSimulation(*sporef,simname);

    v8::Local<v8::Object> tmpObj = jsObjScript->manager()->mInvokableObjectTemplate->NewInstance();
    tmpObj->SetInternalField(JSSIMOBJECT_JSOBJSCRIPT_FIELD,External::New(jsObjScript));
    tmpObj->SetInternalField(JSSIMOBJECT_SIMULATION_FIELD,External::New(invokableObj));
    tmpObj->SetInternalField(TYPEID_FIELD, External::New(new String(JSSIMOBJECT_TYPEID_STRING)));
    return tmpObj;
}
    

v8::Handle<v8::Value> JSPresenceStruct::struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries)
{
    return jsObjScript->createContext(this,canMessage,sendEveryone,recvEveryone,proxQueries);
}


void JSPresenceStruct::addAssociatedContext(JSContextStruct* toAdd)
{
    associatedContexts.push_back(toAdd);
}

v8::Handle<v8::Value> JSPresenceStruct::registerOnProxRemovedEventHandler(v8::Handle<v8::Function> cb)
{
    v8::HandleScope handle_scope;
    if (!mOnProxRemovedEventHandler.IsEmpty())
        mOnProxRemovedEventHandler.Dispose();

    mOnProxRemovedEventHandler = v8::Persistent<v8::Function>::New(cb);

    return v8::Boolean::New(true);
}
    
v8::Handle<v8::Value> JSPresenceStruct::registerOnProxAddedEventHandler(v8::Handle<v8::Function> cb)
{
    v8::HandleScope handle_scope;
    if (!mOnProxAddedEventHandler.IsEmpty())
        mOnProxAddedEventHandler.Dispose();

    mOnProxAddedEventHandler = v8::Persistent<v8::Function>::New(cb);

    return v8::Boolean::New(true);
}


v8::Handle<v8::Value> JSPresenceStruct::struct_getPosition()
{
    if (!getIsConnected())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling getPosition on presence.  The presence is not connected to any space, and therefore does not have any position.")));

    setFlag();
        
    return jsObjScript->getPositionFunction(sporef);
}


v8::Handle<v8::Value> JSPresenceStruct::struct_setVelocity(const Vector3f& newVel)
{
    if (!getIsConnected())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling setVelocity on presence.  The presence is not connected to any space, and therefore has no velocity to set.")));

    
    jsObjScript->setVelocityFunction(sporef,newVel);
    return v8::Undefined();
}


v8::Handle<v8::Value> JSPresenceStruct::struct_broadcastVisible(v8::Handle<v8::Object> toBroadcast)
{
    v8::HandleScope handle_scope;
    if (!getIsConnected())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling broadcastVisible on presence.  The presence is not connected to any space, and therefore has no one to broadcast to.")));

    
    v8::Local<v8::Object> obj = v8::Local<v8::Object>::New(toBroadcast);
    jsObjScript->broadcastVisible(sporef,JSSerializer::serializeObject(obj));
    return v8::Undefined();
}

JSPresenceStruct* JSPresenceStruct::decodePresenceStruct(v8::Handle<v8::Value> toDecode ,String& errorMessage)
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