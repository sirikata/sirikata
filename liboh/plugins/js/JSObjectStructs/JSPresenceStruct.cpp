
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
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent, v8::Handle<v8::Function> connectedCallback, JSContextStruct* ctx, HostedObject::PresenceToken presenceToken, JSCtx* jsctx)
 // TODO(ewencp) this used to pass a default version of JSVisibleDataPtr instead
 // of NULL, is this ok?
 : JSPositionListener(parent, JSAggregateVisibleDataPtr(), jsctx),
   JSSuspendable(),
   mOnConnectedCallback(v8::Persistent<v8::Function>::New(connectedCallback)),
   mParent(parent),
   mContID(ctx->getContextID()),
   isConnected(false),
   hasConnectedCallback(true),
   mPresenceToken(presenceToken),
   mSuspendedVelocity(Vector3f::zero()),
   mSuspendedOrientationVelocity(Quaternion::identity()),
   mContext(ctx),
   mQuery()
{
    mContext->struct_registerSuspendable(this);
}

//use this constructor if we already have a presence that is connected to the
//space with spaceObjectRecference _sporef
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent, const SpaceObjectReference& _sporef, JSContextStruct* ctx,HostedObject::PresenceToken presenceToken, JSCtx* jsctx)
 : JSPositionListener(parent, parent->jsVisMan.getOrCreateVisible(_sporef),jsctx),
   JSSuspendable(),
   mParent(parent),
   mContID(ctx->getContextID()),
   isConnected(true),
   hasConnectedCallback(false),
   mPresenceToken(presenceToken),
   mSuspendedVelocity(Vector3f::zero()),
   mSuspendedOrientationVelocity(Quaternion::identity()),
   mContext(ctx),
   mQuery()
{
    // Normally we would need to initialize after calling
    // getOrCreateVisibleData, but in this case we will have already initialized
    // from the proxy data (in EmersonScript).

    mContext->struct_registerSuspendable(this);
}

//use this constructor when we are restoring a presence.
JSPresenceStruct::JSPresenceStruct(EmersonScript* parent, PresStructRestoreParams& psrp, Vector3f center, HostedObject::PresenceToken presToken,JSContextStruct* jscont, const TimedMotionVector3f& tmv, const TimedMotionQuaternion& tmq, JSCtx* jsctx)
 : JSPositionListener(parent, parent->jsVisMan.getOrCreateVisible(psrp.sporef), jsctx),
   JSSuspendable(),
   mParent(parent),
   isConnected(false),
   hasConnectedCallback(false),
   mPresenceToken(presToken),
   mContext(NULL)
{
    jpp->updateFrom(
        PresenceProperties(
            tmv, tmq, BoundingSphere3f(center, psrp.scale), Transfer::URI(psrp.mesh), psrp.physics
        )
    );

    mQuery = psrp.query;

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

    returner->Set(v8::String::New("query"), struct_getQuery());


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
    // We need to update our JSVisibleDataPtr since before the connect we didn't even
    // know what our SpaceObjectReference would be.
    jpp = mParent->jsVisMan.getOrCreateVisible(_sporef);

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


v8::Handle<v8::Value> JSPresenceStruct::setOrientationVelFunction(Quaternion newOrientationVel)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setOrientationVel");
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::MOVEMENT,setOrientationVel);

    Time tnow = mParent->getHostedTime();
    TimedMotionQuaternion orient = jpp->orientation();
    TimedMotionQuaternion new_orient(tnow, MotionQuaternion(orient.position(tnow), newOrientationVel) );
    mParent->setOrientation(getSporef(), new_orient);

    return v8::Undefined();
}
v8::Handle<v8::Value> JSPresenceStruct::struct_setVelocity(const Vector3f& newVel)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setVelocity");
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::MOVEMENT,setVelocity);

    Time tnow = mParent->getHostedTime();
    TimedMotionVector3f loc = jpp->location();
    TimedMotionVector3f new_loc(tnow, MotionVector3f(loc.position(tnow), newVel) );
    mParent->setLocation(getSporef(), new_loc);

    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::struct_setPosition(Vector3f newPos)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setPosition");
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::MOVEMENT,setPosition);


    Time tnow = mParent->getHostedTime();
    TimedMotionVector3f loc = jpp->location();
    TimedMotionVector3f new_loc(tnow, MotionVector3f(newPos, loc.velocity()) );
    mParent->setLocation(getSporef(), new_loc);

    return v8::Undefined();
}

v8::Handle<v8::Value> JSPresenceStruct::setOrientationFunction(Quaternion newOrientation)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setOrientation");
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::MOVEMENT,setPosition);

    Time tnow = mParent->getHostedTime();
    TimedMotionQuaternion orient = jpp->orientation();
    TimedMotionQuaternion new_orient(tnow, MotionQuaternion(newOrientation, orient.velocity()) );
    mParent->setOrientation(getSporef(), new_orient);

    return v8::Undefined();
}


v8::Handle<v8::Value> JSPresenceStruct::setVisualScaleFunction(float new_scale)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setScale");
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::MESH,setScale);

    mParent->setBounds(getSporef(), BoundingSphere3f(jpp->bounds().center(), new_scale));

    return v8::Undefined();
}

v8::Handle<v8::Value>JSPresenceStruct::setVisualFunction(String urilocation)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setMesh");
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::MESH,setMesh);
    mParent->setVisual(getSporef(), urilocation);
    return v8::Undefined();
}



String JSPresenceStruct::getQuery()
{
    if (isConnected) {
        assert(jpp);
        const SpaceObjectReference& pres_id = jpp->id();
        return mParent->getQuery(pres_id);
    }
    return mQuery;
}

v8::Handle<v8::Value> JSPresenceStruct::struct_getQuery()
{
    INLINE_CHECK_IS_CONNECTED_ERROR("getQueryAngle");
    String q = getQuery();
    return v8::String::New(q.c_str(), q.size());
}

v8::Handle<v8::Value> JSPresenceStruct::setQueryFunction(const String& new_qa)
{
    INLINE_CHECK_IS_CONNECTED_ERROR("setQuery");
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::PROX_QUERIES,setQuery);
    mParent->setQueryFunction(getSporef(), new_qa);
    mQuery = new_qa;
    return v8::Undefined();
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
    // We might be able to do this in JSPositionListener if we don't have any
    // overridden virtual methods?
    Liveness::letDie();

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





//returns this presence as a visible object.
v8::Local<v8::Object>  JSPresenceStruct::toVisible()
{
    return mParent->presToVis(this,mContext);
}


/*
  GIANT FIXME on this entire function.  Likely garbage collection is incorrect.
  Doing very little through jsobjscript directly, etc.
 */
v8::Handle<v8::Value> JSPresenceStruct::runSimulation(String simname)
{
    v8::HandleScope scope;
    INLINE_CHECK_CAPABILITY_ERROR(Capabilities::GUI, runSimulation);

    JSInvokableObject::JSInvokableObjectInt* invokableObj = mParent->runSimulation(getSporef(),simname);
    if (invokableObj == NULL)
        return scope.Close(v8::Undefined());

    v8::Local<v8::Object> tmpObj = mParent->JSObjectScript::mCtx->mInvokableObjectTemplate->NewInstance();
    tmpObj->SetInternalField(JSSIMOBJECT_JSOBJSCRIPT_FIELD,External::New(mParent));
    tmpObj->SetInternalField(JSSIMOBJECT_SIMULATION_FIELD,External::New(invokableObj));
    tmpObj->SetInternalField(TYPEID_FIELD, External::New(new String(JSSIMOBJECT_TYPEID_STRING)));
    return scope.Close(tmpObj);
}




void JSPresenceStruct::addAssociatedContext(JSContextStruct* toAdd)
{
    associatedContexts.push_back(toAdd);
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
