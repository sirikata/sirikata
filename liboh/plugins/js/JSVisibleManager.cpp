#include "JSVisibleManager.hpp"
#include "JSObjectStructs/JSPositionListener.hpp"
#include "JSLogging.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "EmersonScript.hpp"

namespace Sirikata{
namespace JS{

JSProxyData::JSProxyData()
 : emerScript(NULL),
   refcount(0),
   selfPtr()
{
}

JSProxyData::JSProxyData(EmersonScript* eScript, const SpaceObjectReference& from)
 : emerScript(eScript),
   sporefToListenTo(from),
   refcount(0),
   selfPtr()
{
}

JSProxyData::JSProxyData(EmersonScript* eScript, ProxyObjectPtr from)
 : emerScript(eScript),
   sporefToListenTo(from->getObjectReference()),
   refcount(0),
   selfPtr()
{
    updateFrom(from);
}

JSProxyData::JSProxyData(EmersonScript* eScript, const SpaceObjectReference& from, JSProxyPtr fromData)
 : emerScript(eScript),
   sporefToListenTo(from),
   mLocation(fromData->mLocation),
   mOrientation(fromData->mOrientation),
   mBounds(fromData->mBounds),
   mMesh(fromData->mMesh),
   mPhysics(fromData->mPhysics),
   refcount(0),
   selfPtr()
{
}

JSProxyData::~JSProxyData()
{
    // We're guaranteed by shared_ptrs and our reference counting that
    // this destructor is only called once there are no more
    // ProxyObjectPtrs and no v8 visibles referring to this data,
    // allowing us to clear this out of the JSVisibleManager.
    if (emerScript)
        emerScript->removeProxyData(sporefToListenTo);
}

void JSProxyData::incref(JSProxyPtr self) {
    selfPtr = self;
    refcount++;
}

void JSProxyData::decref() {
    assert(refcount > 0);
    refcount--;
    if (refcount == 0) selfPtr.reset();
}

void JSProxyData::updateFrom(ProxyObjectPtr from) {
    assert(sporefToListenTo  == from->getObjectReference());

    mLocation = from->location();
    mOrientation = from->orientation();
    mBounds = from->bounds();
    mMesh = from->mesh().toString();
    mPhysics = from->physics();
}

bool JSProxyData::visibleToPresence() const {
    return refcount > 0;
}



JSVisibleManager::JSVisibleManager(EmersonScript* eScript,JSCtx* ctx)
 : emerScript(eScript),
   mCtx(ctx)
{
}

JSVisibleManager::~JSVisibleManager()
{
    // Stop tracking all known objects to clear out all listeners and state.
    while(!mTrackedObjects.empty()) {
        ProxyObjectPtr toFakeDestroy = *(mTrackedObjects.begin());
        onDestroyProxy(toFakeDestroy);
    }

    // Some proxies may not have gotten cleared out if there are still
    // references to it, e.g. from objects not yet collected by v8. In this
    // case, don't destroy them, but set them up so they won't try to invoke
    // methods in this object anymore, just getting cleaned up naturally when
    // there are no more references to them.
    for(SporefProxyMap::iterator pit = mProxies.begin(); pit != mProxies.end(); pit++) {
        // Clearing this blocks it from getting back to us.
        pit->second.lock()->emerScript = NULL;
    }
    mProxies.clear();
}


JSVisibleStruct* JSVisibleManager::createVisStruct(const SpaceObjectReference& whatsVisible, JSProxyPtr addParams) {
    JSProxyPtr toCreateFrom = getOrCreateProxyPtr(whatsVisible, addParams);
    return new JSVisibleStruct(toCreateFrom,mCtx);
}

JSProxyPtr JSVisibleManager::getOrCreateProxyPtr(const SpaceObjectReference& whatsVisible, JSProxyPtr addParams) {
    // If we have proxy data already, ignore the additional parameters
    // (since they only come from previously stored objects) and
    // return the existing data.
    SporefProxyMapIter proxIter = mProxies.find(whatsVisible);
    if (proxIter != mProxies.end()) {
        JSProxyPtr ret = proxIter->second.lock();
        // Based on our memory management, it shouldn't be possible for it to
        // remain as a weak pointer in our data structure but have no other
        // references.
        assert(ret);
        return ret;
    }

    JSProxyPtr returner;
    if (addParams) {
        assert(addParams->sporefToListenTo == whatsVisible);
        returner = JSProxyPtr(new JSProxyData(emerScript, whatsVisible, addParams));
    }
    else {
        returner = JSProxyPtr(new JSProxyData(emerScript, whatsVisible));
    }
    mProxies[whatsVisible] = JSProxyWPtr(returner);
    return returner;
}

JSProxyPtr JSVisibleManager::getOrCreateProxyPtr(ProxyObjectPtr proxy) {
    // If we have data already, update it (new proxy should have
    // up-to-date info).
    SporefProxyMapIter proxIter = mProxies.find(proxy->getObjectReference());
    if (proxIter != mProxies.end()) {
        JSProxyPtr ret = proxIter->second.lock();
        ret->updateFrom(proxy);
        return ret;
    }

    JSProxyPtr returner (new JSProxyData(emerScript, proxy));
    mProxies[proxy->getObjectReference()] = JSProxyWPtr(returner);
    return returner;
}

void JSVisibleManager::removeProxyData(const SpaceObjectReference& sporef) {
    SporefProxyMapIter proxIter = mProxies.find(sporef);
    assert(proxIter != mProxies.end());
    mProxies.erase(proxIter);
}



void JSVisibleManager::onCreateProxy(ProxyObjectPtr p)
{
    mCtx->objStrand->post(
        std::tr1::bind(&JSVisibleManager::iOnCreateProxy,this,
            p));
}

void JSVisibleManager::iOnCreateProxy(ProxyObjectPtr p)
{
    JSProxyPtr data = getOrCreateProxyPtr(p);
    data->incref(data);
    p->PositionProvider::addListener(this);
    p->MeshProvider::addListener(this);
    mTrackedObjects.insert(p);
}

void JSVisibleManager::onDestroyProxy(ProxyObjectPtr p)
{
    mCtx->objStrand->post(
        std::tr1::bind(&JSVisibleManager::iOnDestroyProxy,this,
            p));
}

void JSVisibleManager::iOnDestroyProxy(ProxyObjectPtr p)
{
    JSProxyPtr data = getOrCreateProxyPtr(p);
    data->decref();

    p->PositionProvider::removeListener(this);
    p->MeshProvider::removeListener(this);

    mTrackedObjects.erase(p);
}

void JSVisibleManager::updateLocation (
    ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation,
    const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,
    const SpaceObjectReference& sporef)
{
    mCtx->objStrand->post(
        std::tr1::bind(&JSVisibleManager::iUpdateLocation,this,
            proxy,newLocation,newOrient,newBounds,sporef));
}


void JSVisibleManager::iUpdateLocation (
    ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation,
    const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,
    const SpaceObjectReference& sporef)
{
    JSProxyPtr data = getOrCreateProxyPtr(sporef, JSProxyPtr());
    data->mLocation = newLocation;
    data->mOrientation = newOrient;
    data->mBounds = newBounds;
}


void JSVisibleManager::onSetMesh (
    ProxyObjectPtr proxy, Transfer::URI const& newMesh,
    const SpaceObjectReference& sporef)
{
    mCtx->objStrand->post(
        std::tr1::bind(&JSVisibleManager::iOnSetMesh,this,
            proxy,newMesh,sporef));
}

void JSVisibleManager::iOnSetMesh (
    ProxyObjectPtr proxy, Transfer::URI const& newMesh,
    const SpaceObjectReference& sporef)
{
    JSProxyPtr data = getOrCreateProxyPtr(sporef, JSProxyPtr());
    data->mMesh = newMesh.toString();
}

void JSVisibleManager::onSetScale (
    ProxyObjectPtr proxy, float32 newScale, const SpaceObjectReference& sporef)
{
    mCtx->objStrand->post(
        std::tr1::bind(&JSVisibleManager::iOnSetScale,this,
            proxy,newScale,sporef));
}

void JSVisibleManager::iOnSetScale(
    ProxyObjectPtr proxy, float32 newScale, const SpaceObjectReference& sporef)
{
    JSProxyPtr data = getOrCreateProxyPtr(sporef, JSProxyPtr());
    data->mBounds = BoundingSphere3f(data->mBounds.center(), newScale);
}

void JSVisibleManager::onSetPhysics (
    ProxyObjectPtr proxy, const String& newphy,
    const SpaceObjectReference& sporef)
{
    mCtx->objStrand->post(
        std::tr1::bind(&JSVisibleManager::iOnSetPhysics,this,
            proxy,newphy,sporef));
}

void JSVisibleManager::iOnSetPhysics (
    ProxyObjectPtr proxy, const String& newphy,
    const SpaceObjectReference& sporef)
{
    JSProxyPtr data = getOrCreateProxyPtr(sporef, JSProxyPtr());
    data->mPhysics = newphy;
}


bool JSVisibleManager::isVisible(const SpaceObjectReference& sporef)
{
    JSProxyPtr data = getOrCreateProxyPtr(sporef, JSProxyPtr());
    return data->visibleToPresence();
}

v8::Handle<v8::Value> JSVisibleManager::isVisibleV8(const SpaceObjectReference& sporef)
{
    return v8::Boolean::New(isVisible(sporef));
}

}//end namespace JS
}//end namespace Sirikata
