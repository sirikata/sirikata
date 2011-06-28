
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../EmersonScript.hpp"
#include "../JSSerializer.hpp"
#include "../JSLogging.hpp"
#include "../JSObjects/JSVec3.hpp"
#include "../JSObjects/JSQuaternion.hpp"
#include "JSPositionListener.hpp"
#include "JSContextStruct.hpp"
#include "JSSuspendable.hpp"


namespace Sirikata {
namespace JS {


//this constructor is called when we ask the space to create a presence for us.
//we give the space the token presenceToken, which it ships back when connection 
//is completed.
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent, v8::Handle<v8::Function> connectedCallback,JSContextStruct* ctx, HostedObject::PresenceToken presenceToken)
 : JSPositionListener(nullProxyPtr),
   JSSuspendable(),
   mOnConnectedCallback(v8::Persistent<v8::Function>::New(connectedCallback)),
   mContID(ctx->getContextID()),
   isConnected(false),
   hasConnectedCallback(true),
   mPresenceToken(presenceToken),
   mSuspendedVelocity(Vector3f::nil()),
   mSuspendedOrientationVelocity(Quaternion::identity()),
   mContext(ctx)
{
    mContext->struct_registerSuspendable(this);
}

//use this constructor if we already have a presence that is connected to the
//space with spaceObjectRecference _sporef
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent, const SpaceObjectReference& _sporef, JSContextStruct* ctx,HostedObject::PresenceToken presenceToken)
 : JSPositionListener(parent->createProxyPtr(_sporef,nullProxyPtr)),
   JSSuspendable(),
   mContID(ctx->getContextID()),
   isConnected(true),
   hasConnectedCallback(false),
   mPresenceToken(presenceToken),
   mSuspendedVelocity(Vector3f::nil()),
   mSuspendedOrientationVelocity(Quaternion::identity()),
   mContext(ctx)
{
    mContext->struct_registerSuspendable(this);
}

//use this constructor when we are restoring a presence.
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent,PresStructRestoreParams& psrp,Vector3f center, HostedObject::PresenceToken presToken,JSContextStruct* jscont)
 : JSPositionListener(parent->createProxyPtr(*psrp.mSporef,nullProxyPtr)),
   JSSuspendable(),
   isConnected(false),
   hasConnectedCallback(false),
   mPresenceToken(presToken),
   mContext(NULL)
{
    jpp->mLocation = *psrp.mTmv3f;
    jpp->mOrientation = *psrp.mTmq;
    jpp->mMesh = *psrp.mMesh;
    jpp->mBounds = BoundingSphere3f( center  ,* psrp.mScale);
    jpp->mPhysics = "";
    mQuery       = *psrp.mQuery;

    mSuspendedVelocity = *psrp.mSuspendedVelocity;
    mSuspendedOrientationVelocity = *psrp.mSuspendedOrientationVelocity;

    if (psrp.mConnCallback != NULL)
    {
        hasConnectedCallback = true;
        mOnConnectedCallback = v8::Persistent<v8::Function>::New(*psrp.mConnCallback);
    }

    if (*psrp.mIsSuspended)
        suspend();

    if (*psrp.mIsCleared)
        clear();

    mContID = *psrp.mContID;
    if (mContID != jscont->getContextID())
        parent->registerFixupSuspendable(this,mContID);
    else
        mContext = jscont;
}


v8::Handle<v8::Value> JSPresenceStruct::getAllData()
{
    v8::HandleScope handle_scope;
    v8::Handle<v8::Value> returnerVal = JSPositionListener::struct_getAllData();
    if (!returnerVal->IsObject())
    {
        //means there was an exception in underlying position listener.  pass it
        //on by returning here.
        return returnerVal;
    }

    v8::Handle<v8::Object> returner = returnerVal->ToObject();
    

    uint32 contID = mContext->getContextID();
    returner->Set(v8::String::New("isConnected"), v8::Boolean::New(isConnected));
    returner->Set(v8::String::New("contextId"), v8::Integer::NewFromUnsigned(contID));
    bool isclear   = getIsCleared();
    returner->Set(v8::String::New("isCleared"), v8::Boolean::New(isclear));


    bool issusp = getIsSuspended();
    returner->Set(v8::String::New("isSuspended"), v8::Boolean::New(issusp));

    if (!v8::Context::InContext())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in get all data of presences truct.  not currently in a v8 context.")));

    v8::Handle<v8::Context>curContext = v8::Context::GetCurrent();
    returner->Set(v8::String::New("suspendedOrientationVelocity"),CreateJSResult(curContext,mSuspendedOrientationVelocity));
    returner->Set(v8::String::New("suspendedVelocity"),CreateJSResult(curContext,mSuspendedVelocity));

    returner->Set(v8::String::New("solidAngleQuery"),struct_getQueryAngle());

    

    //onConnected
    if (mOnConnectedCallback.IsEmpty())
        returner->Set(v8::String::New("connectCallback"), v8::Null());
    else
        returner -> Set(v8::String::New("connectCallback"),    mOnConnectedCallback);


    return handle_scope.Close(returner);
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




//note will still receive a call to disconnect from JSObjectScript.cpp, so don't
//do too much clean up here.
//when doing disconnecting call his
v8::Handle<v8::Value> JSPresenceStruct::clear()
{
    if (getIsCleared())
        return JSSuspendable::clear();

    v8::HandleScope handle_scope;
    v8::Handle<v8::Value> returner = JSSuspendable::clear();

    for (ContextVector::iterator iter = associatedContexts.begin(); iter != associatedContexts.end(); ++iter)
        (*iter)->clear();
    associatedContexts.clear();

    clearPreviousConnectedCB();

    isConnected = false;

    //do not ask emerson script to delete presence here.  the context will get
    //this presence deleted.
    
    mContext->struct_deregisterSuspendable(this);
    return handle_scope.Close(returner);
}


v8::Handle<v8::Value> JSPresenceStruct::disconnect()
{
    clearPreviousConnectedCB();
    isConnected = false;

    jpp->emerScript->requestDisconnect(this);
    return v8::Boolean::New(true);
}


bool JSPresenceStruct::getIsConnected()
{
    return isConnected;
}

HostedObject::PresenceToken JSPresenceStruct::getPresenceToken()
{
    return mPresenceToken;
}

void JSPresenceStruct::connect(const SpaceObjectReference& _sporef, JSProxyPtr newJPP)
{
    //set the jpp of jspositionlistener to be a non-empty value now that we
    //actually have a sporef to associate with.
    setSharedProxyDataPtr(   newJPP);
    
    v8::HandleScope handle_scope;

    if (getIsConnected())
        JSLOG(error, "Error when calling connect on presence.  The presence was already connected.");

    isConnected = true;
    callConnectedCallback();
}


void JSPresenceStruct::callConnectedCallback()
{
    if (hasConnectedCallback)
        jpp->emerScript->handlePresCallback(mOnConnectedCallback,mContext,this);


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
    jpp->emerScript->setOrientationFunction(getSporef(),newOrientation);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setOrientationVelFunction(Quaternion newOrientationVel)
{
    jpp->emerScript->setOrientationVelFunction(getSporef(),newOrientationVel);
    return v8::Undefined();
}

SolidAngle JSPresenceStruct::getQueryAngle()
{
    return jpp->emerScript->getQueryAngle(getSporef());
}

v8::Handle<v8::Value> JSPresenceStruct::struct_getQueryAngle()
{
    return v8::Number::New(getQueryAngle().asFloat());
}

v8::Handle<v8::Value> JSPresenceStruct::setQueryAngleFunction(SolidAngle new_qa)
{
    jpp->emerScript->setQueryAngleFunction(getSporef(), new_qa);
    mQuery = new_qa;
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setVisualScaleFunction(float new_scale)
{
    jpp->emerScript->setVisualScaleFunction(getSporef(), new_scale);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::struct_setPosition(Vector3f newPos)
{
    jpp->emerScript->setPositionFunction(getSporef(), newPos);
    return v8::Undefined();
}


v8::Handle<v8::Value> JSPresenceStruct::getPhysicsFunction() {
    return jpp->emerScript->getPhysicsFunction(getSporef());
}
v8::Handle<v8::Value> JSPresenceStruct::setPhysicsFunction(const String& phy) {
    jpp->emerScript->setPhysicsFunction(getSporef(), phy);
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
    clear();
}

//called from jsobjectscript.
void JSPresenceStruct::disconnectCalledFromObjScript()
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
    jpp->emerScript->setVisualFunction(getSporef(), urilocation);
    return v8::Undefined();
}


//returns this presence as a visible object.
v8::Persistent<v8::Object>  JSPresenceStruct::toVisible()
{
    return jpp->emerScript->presToVis(this,mContext);
}


/*
  GIANT FIXME on this entire function.  Likely garbage collection is incorrect.
  Doing very little through jsobjscript directly, etc.
 */
v8::Handle<v8::Value>JSPresenceStruct::runSimulation(String simname)
{
    v8::HandleScope scope;
    JSInvokableObject::JSInvokableObjectInt* invokableObj = jpp->emerScript->runSimulation(getSporef(),simname);

    v8::Local<v8::Object> tmpObj = jpp->emerScript->manager()->mInvokableObjectTemplate->NewInstance();
    tmpObj->SetInternalField(JSSIMOBJECT_JSOBJSCRIPT_FIELD,External::New(jpp->emerScript));
    tmpObj->SetInternalField(JSSIMOBJECT_SIMULATION_FIELD,External::New(invokableObj));
    tmpObj->SetInternalField(TYPEID_FIELD, External::New(new String(JSSIMOBJECT_TYPEID_STRING)));
    return tmpObj;
}


v8::Handle<v8::Value> JSPresenceStruct::struct_createContext(SpaceObjectReference canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport,bool canCreatePres, bool canCreateEnt, bool canEval)
{
    JSContextStruct* dummy;
    return jpp->emerScript->createContext(this,canMessage,sendEveryone,recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval,dummy);
}


void JSPresenceStruct::addAssociatedContext(JSContextStruct* toAdd)
{
    associatedContexts.push_back(toAdd);
}


v8::Handle<v8::Value> JSPresenceStruct::struct_setVelocity(const Vector3f& newVel)
{
    if (!getIsConnected())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling setVelocity on presence.  The presence is not connected to any space, and therefore has no velocity to set.")));


    jpp->emerScript->setVelocityFunction(getSporef(),newVel);
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
