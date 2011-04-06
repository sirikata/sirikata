
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "../JSLogging.hpp"
#include "../JSObjects/JSVec3.hpp"
#include "JSPositionListener.hpp"
#include "JSContextStruct.hpp"
#include "JSSuspendable.hpp"

namespace Sirikata {
namespace JS {


//this constructor is called when the presence associated
JSPresenceStruct::JSPresenceStruct(JSObjectScript* parent, v8::Handle<v8::Function> connectedCallback,JSContextStruct* ctx, HostedObject::PresenceToken presenceToken)
 : JSPositionListener(parent),
   JSSuspendable(),
   mOnConnectedCallback(v8::Persistent<v8::Function>::New(connectedCallback)),
   isConnected(false),
   hasConnectedCallback(true),
   mPresenceToken(presenceToken),
   mContext(ctx)
{
    mContext->struct_registerSuspendable(this);
}


JSPresenceStruct::JSPresenceStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef, JSContextStruct* ctx,HostedObject::PresenceToken presenceToken)
 : JSPositionListener(parent),
   JSSuspendable(),
   isConnected(true),
   hasConnectedCallback(false),
   mPresenceToken(presenceToken),
   mContext(ctx)
{
    mContext->struct_registerSuspendable(this);
    JSPositionListener::setListenTo(&_sporef, &_sporef);
    JSPositionListener::registerAsPosListener();
}



v8::Handle<v8::Value> JSPresenceStruct::getIsConnectedV8()
{
    v8::HandleScope handle_scope;
    return v8::Boolean::New(isConnected);
}


v8::Handle<v8::Value> JSPresenceStruct::suspend()
{
    if (getIsSuspended())
        return getIsSuspendedV8();

    mSuspendedVelocity = getVelocity();
    mSuspendedOrientationVelocity = getOrientationVelocity();

    Vector3f newVel (0,0,0);
    struct_setVelocity(newVel);
    Quaternion newOrientVel(0,0,0,1);
    setOrientationVelFunction(newOrientVel);


    return JSSuspendable::suspend();
}
v8::Handle<v8::Value> JSPresenceStruct::resume()
{
    if (! getIsSuspended())
        return JSSuspendable::resume();

    struct_setVelocity(mSuspendedVelocity);
    setOrientationVelFunction(mSuspendedOrientationVelocity);
    return JSSuspendable::resume();
}


v8::Handle<v8::Value>JSPresenceStruct::requestDisconnect()
{
    jsObjScript->requestDisconnect(this);
    return v8::Undefined();
}


//note will still receive a call to disconnect from JSObjectScript.cpp, so don't
//do too much clean up here.
v8::Handle<v8::Value> JSPresenceStruct::clear()
{
    requestDisconnect();
    if (isConnected)
        deregisterAsPosListener();

    isConnected = false;
    clearPreviousConnectedCB();
    return JSSuspendable::clear();
}


bool JSPresenceStruct::getIsConnected()
{
    return isConnected;
}

HostedObject::PresenceToken JSPresenceStruct::getPresenceToken()
{
    return mPresenceToken;
}

void JSPresenceStruct::connect(const SpaceObjectReference& _sporef)
{
    v8::HandleScope handle_scope;

    if (getIsConnected())
    {
        JSLOG(error, "Error when calling connect on presence.  The presence was already connected.");
        deregisterAsPosListener();
    }

    isConnected = true;
    JSPositionListener::setListenTo(&_sporef,NULL);
    JSPositionListener::registerAsPosListener();


    callConnectedCallback();
}


void JSPresenceStruct::callConnectedCallback()
{
    if (hasConnectedCallback)
        jsObjScript->handlePresCallback(mOnConnectedCallback,mContext,this);

    if (mContext != NULL)
        mContext->checkContextConnectCallback(this);

}


void JSPresenceStruct::clearPreviousConnectedCB()
{
    if (hasConnectedCallback)
        mOnConnectedCallback.Dispose();

    hasConnectedCallback = false;
}

v8::Handle<v8::Value> JSPresenceStruct::setOrientationFunction(Quaternion newOrientation)
{
    jsObjScript->setOrientationFunction(sporefToListenTo,newOrientation);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setOrientationVelFunction(Quaternion newOrientationVel)
{
    jsObjScript->setOrientationVelFunction(sporefToListenTo,newOrientationVel);
    return v8::Undefined();
}



v8::Handle<v8::Value> JSPresenceStruct::setQueryAngleFunction(SolidAngle new_qa)
{
    jsObjScript->setQueryAngleFunction(sporefToListenTo, new_qa);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setVisualScaleFunction(float new_scale)
{
    jsObjScript->setVisualScaleFunction(sporefToListenTo, new_scale);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::struct_setPosition(Vector3f newPos)
{
    jsObjScript->setPositionFunction(sporefToListenTo, newPos);
    return v8::Undefined();
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

}

void JSPresenceStruct::disconnect()
{
    if (getIsCleared())
        return;

    if (! getIsConnected())
        JSLOG(error, "Error when calling disconnect on presence.  The presence wasn't already connected.");

    isConnected = false;


    if (mContext != NULL)
        mContext->checkContextDisconnectCallback(this);

}

v8::Handle<v8::Value>JSPresenceStruct::setVisualFunction(String urilocation)
{
    jsObjScript->setVisualFunction(sporefToListenTo, urilocation);
    return v8::Undefined();
}

v8::Handle<v8::Value>JSPresenceStruct::getVisualFunction()
{
    return jsObjScript->getVisualFunction(sporefToListenTo);
}

//returns this presence as a visible object.
v8::Persistent<v8::Object>  JSPresenceStruct::toVisible()
{
    return jsObjScript->presToVis(this,mContext);
}


/*
  GIANT FIXME on this entire function.  Likely garbage collection is incorrect.
  Doing very little through jsobjscript directly, etc.
 */
v8::Handle<v8::Value>JSPresenceStruct::runSimulation(String simname)
{
    v8::HandleScope scope;
    JSInvokableObject::JSInvokableObjectInt* invokableObj = jsObjScript->runSimulation(*sporefToListenTo,simname);

    v8::Local<v8::Object> tmpObj = jsObjScript->manager()->mInvokableObjectTemplate->NewInstance();
    tmpObj->SetInternalField(JSSIMOBJECT_JSOBJSCRIPT_FIELD,External::New(jsObjScript));
    tmpObj->SetInternalField(JSSIMOBJECT_SIMULATION_FIELD,External::New(invokableObj));
    tmpObj->SetInternalField(TYPEID_FIELD, External::New(new String(JSSIMOBJECT_TYPEID_STRING)));
    return tmpObj;
}


v8::Handle<v8::Value> JSPresenceStruct::struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport,bool canCreatePres, bool canCreateEnt, bool canEval)
{
    JSContextStruct* dummy;
    return jsObjScript->createContext(this,canMessage,sendEveryone,recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval,dummy);
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

v8::Handle<v8::Value> JSPresenceStruct::struct_setVelocity(const Vector3f& newVel)
{
    if (!getIsConnected())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling setVelocity on presence.  The presence is not connected to any space, and therefore has no velocity to set.")));


    jsObjScript->setVelocityFunction(sporefToListenTo,newVel);
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
