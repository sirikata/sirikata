
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
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent, v8::Handle<v8::Function> connectedCallback, JSContextStruct* ctx, HostedObject::PresenceToken presenceToken)
 : JSPositionListener(JSProxyPtr(new JSProxyData())),
   JSSuspendable(),
   mParent(parent),
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
 : JSPositionListener(parent->getOrCreateProxyPtr(_sporef)),
   JSSuspendable(),
   mParent(parent),
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
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent,PresStructRestoreParams& psrp,Vector3f center, HostedObject::PresenceToken presToken,JSContextStruct* jscont, const TimedMotionVector3f& tmv, const TimedMotionQuaternion& tmq)
 : JSPositionListener(parent->getOrCreateProxyPtr(psrp.sporef)),
   JSSuspendable(),
   mParent(parent),
   isConnected(false),
   hasConnectedCallback(false),
   mPresenceToken(presToken),
   mContext(NULL)
{
    jpp->mLocation    = tmv;
    jpp->mOrientation = tmq;
    jpp->mMesh    = psrp.mesh;
    jpp->mBounds  = BoundingSphere3f( center  ,psrp.scale);
    jpp->mPhysics = psrp.physics;

    mQuery        = psrp.query;

    if (!psrp.suspendedVelocity.isNull())
        mSuspendedVelocity = psrp.suspendedVelocity.getValue();

    if (!psrp.suspendedOrientationVelocity.isNull())
        mSuspendedOrientationVelocity = psrp.suspendedOrientationVelocity.getValue();

    if (!psrp.connCallback.isNull())
    {
        hasConnectedCallback = true;
        mOnConnectedCallback = v8::Persistent<v8::Function>::New(psrp.connCallback.getValue());
    }

    if (psrp.isSuspended)
        suspend();

    if (psrp.isCleared)
        clear();


    //if null, should just create presence in current context.
    //if mContID != jscont->getContextID, means that we were restoring a
    //presence, and should be restoring a presence that was in a different
    //sandbox than the one we're in.
    if ((! psrp.contID.isNull()) && (psrp.contID.getValue() != jscont->getContextID()))
    {
        mContID = psrp.contID.getValue();
        parent->registerFixupSuspendable(this,mContID);
        JSLOG(fatal,"Restoring a presence with multiple sandboxes doesn't work right now. You won't be able to receive messages properly...");
    }
    else {
        mContID = jscont->getContextID();
        mContext = jscont;
        // FIXME this same call needs to go in the if-block above when it sets
        // mContext properly.
        mContext->struct_registerSuspendable(this);
    }

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
    returner->Set(v8::String::New("queryCount"),struct_getQueryCount());



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

    if (getIsConnected())
        disconnect();

    //do not ask emerson script to delete presence here.  the context will get
    //this presence deleted.

    mContext->struct_deregisterSuspendable(this);
    return handle_scope.Close(returner);
}


v8::Handle<v8::Value> JSPresenceStruct::disconnect()
{
    clearPreviousConnectedCB();
    // Don't set isConnected = false here, callbacks will set it when it's
    // actually occurred.
    mParent->requestDisconnect(this);
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

void JSPresenceStruct::connect(const SpaceObjectReference& _sporef)
{
    // We need to update our JSProxyPtr since before the connect we didn't even
    // know what our SpaceObjectReference would be.
    jpp = mParent->getOrCreateProxyPtr(_sporef);

    v8::HandleScope handle_scope;

    if (getIsConnected())
        JSLOG(error, "Error when calling connect on presence.  The presence was already connected.");

    isConnected = true;
    callConnectedCallback();
}


void JSPresenceStruct::callConnectedCallback()
{
    if (hasConnectedCallback)
        mParent->handlePresCallback(mOnConnectedCallback,mContext,this);


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
    INLINE_CHECK_IS_CONNECTED_ERROR("setOrientation");
    mParent->setOrientationFunction(getSporef(),newOrientation);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setOrientationVelFunction(Quaternion newOrientationVel)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setOrientationVel");
    mParent->setOrientationVelFunction(getSporef(),newOrientationVel);
    return v8::Undefined();
}



//FIXME: should store query angle locally instead of requiring a request through
//EmersonScript to hosted object.
SolidAngle JSPresenceStruct::getQueryAngle()
{
    return mParent->getQueryAngle(getSporef());
}

v8::Handle<v8::Value> JSPresenceStruct::struct_getQueryAngle()
{
    INLINE_CHECK_IS_CONNECTED_ERROR("getQueryAngle");
    return v8::Number::New(getQueryAngle().asFloat());
}

v8::Handle<v8::Value> JSPresenceStruct::setQueryAngleFunction(SolidAngle new_qa)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setQueryAngle");
    mParent->setQueryAngleFunction(getSporef(), new_qa);
    mQuery = new_qa;
    return v8::Undefined();
}

//FIXME: should store query count locally instead of requiring a request through
//EmersonScript to hosted object.
uint32 JSPresenceStruct::getQueryCount()
{
    return mParent->getQueryCount(getSporef());
}

v8::Handle<v8::Value> JSPresenceStruct::struct_getQueryCount()
{
    INLINE_CHECK_IS_CONNECTED_ERROR("getQueryCount");
    return v8::Integer::New(getQueryCount());
}

v8::Handle<v8::Value> JSPresenceStruct::setQueryCount(uint32 new_qc)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setQueryCount");
    mParent->setQueryCount(getSporef(), new_qc);
    return v8::Undefined();
}


v8::Handle<v8::Value> JSPresenceStruct::setVisualScaleFunction(float new_scale)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setScale");
    mParent->setVisualScaleFunction(getSporef(), new_scale);
    return v8::Undefined();
}


v8::Handle<v8::Value> JSPresenceStruct::struct_setPosition(Vector3f newPos)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setPosition");
    mParent->setPositionFunction(getSporef(), newPos);
    return v8::Undefined();
}


v8::Handle<v8::Value> JSPresenceStruct::getPhysicsFunction() {
    return mParent->getPhysicsFunction(getSporef());
}
v8::Handle<v8::Value> JSPresenceStruct::setPhysicsFunction(const String& phy) {
    INLINE_CHECK_IS_CONNECTED_ERROR("setPhysics");
    mParent->setPhysicsFunction(getSporef(), phy);
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
void JSPresenceStruct::markDisconnected()
{
    bool wasConnected = getIsConnected();
    // We *have* to set this, even if we're already cleared, to get
    // all the disconnection paths working properly.
    isConnected = false;

    if (!getIsCleared() && !wasConnected) {
        JSLOG(error, "Error when calling disconnect on presence.  The presence wasn't already connected.");
        return;
    }
}

void JSPresenceStruct::handleDisconnectedCallback() {
    if (getIsCleared()) return;

    if (mContext != NULL)
        mContext->checkContextDisconnectCallback(this);
}




v8::Handle<v8::Value>JSPresenceStruct::setVisualFunction(String urilocation)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setMesh");
    mParent->setVisualFunction(getSporef(), urilocation);
    return v8::Undefined();
}


//returns this presence as a visible object.
v8::Persistent<v8::Object>  JSPresenceStruct::toVisible()
{
    return mParent->presToVis(this,mContext);
}


/*
  GIANT FIXME on this entire function.  Likely garbage collection is incorrect.
  Doing very little through jsobjscript directly, etc.
 */
v8::Handle<v8::Value>JSPresenceStruct::runSimulation(String simname)
{
    v8::HandleScope scope;
    JSInvokableObject::JSInvokableObjectInt* invokableObj = mParent->runSimulation(getSporef(),simname);

    v8::Local<v8::Object> tmpObj = mParent->manager()->mInvokableObjectTemplate->NewInstance();
    tmpObj->SetInternalField(JSSIMOBJECT_JSOBJSCRIPT_FIELD,External::New(mParent));
    tmpObj->SetInternalField(JSSIMOBJECT_SIMULATION_FIELD,External::New(invokableObj));
    tmpObj->SetInternalField(TYPEID_FIELD, External::New(new String(JSSIMOBJECT_TYPEID_STRING)));
    return scope.Close(tmpObj);
}




void JSPresenceStruct::addAssociatedContext(JSContextStruct* toAdd)
{
    associatedContexts.push_back(toAdd);
}


v8::Handle<v8::Value> JSPresenceStruct::struct_setVelocity(const Vector3f& newVel)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setVelocity");
    mParent->setVelocityFunction(getSporef(),newVel);
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
