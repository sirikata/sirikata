#include "JSVisibleManager.hpp"
#include "JSObjectStructs/JSPositionListener.hpp"
#include "JSLogging.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "EmersonScript.hpp"

namespace Sirikata{
namespace JS{


JSVisibleManager::JSVisibleManager(EmersonScript* eScript,JSCtx* ctx)
 : emerScript(eScript),
   mCtx(ctx)
{
}

JSVisibleManager::~JSVisibleManager()
{
    clearVisibles();
}

void JSVisibleManager::clearVisibles() {
    // Stop tracking all known objects to clear out all listeners and state.
    while(!mTrackedObjects.empty()) {
        ProxyObjectPtr toFakeDestroy = *(mTrackedObjects.begin());
        iOnDestroyProxy(toFakeDestroy);
    }

    // Some proxies may not have gotten cleared out if there are still
    // references to it, e.g. from objects not yet collected by v8. In this
    // case, don't destroy them, but set them up so they won't try to invoke
    // methods in this object anymore, just getting cleaned up naturally when
    // there are no more references to them.
    for(SporefProxyMap::iterator pit = mProxies.begin(); pit != mProxies.end(); pit++) {
        // Clearing this blocks it from getting back to us.
        pit->second.lock()->disable();
    }
    mProxies.clear();
}

JSVisibleStruct* JSVisibleManager::createVisStruct(EmersonScript* parent, const SpaceObjectReference& whatsVisible, JSVisibleDataPtr addParams) {
    JSAggregateVisibleDataPtr toCreateFrom = getOrCreateVisible(whatsVisible);
    if (addParams) toCreateFrom->updateFrom(*addParams);
    return new JSVisibleStruct(parent, toCreateFrom,mCtx);
}

void JSVisibleManager::removeVisibleData(JSVisibleData* data) {
    SporefProxyMapIter proxIter = mProxies.find(data->id());
    assert(proxIter != mProxies.end());
    mProxies.erase(proxIter);
}


JSAggregateVisibleDataPtr JSVisibleManager::getOrCreateVisible(const SpaceObjectReference& whatsVisible) {
    SporefProxyMapIter proxIter = mProxies.find(whatsVisible);
    if (proxIter != mProxies.end())
        return proxIter->second.lock();

    JSAggregateVisibleDataPtr ret(new JSAggregateVisibleData(this, whatsVisible));
    mProxies.insert( SporefProxyMap::value_type(whatsVisible, ret) );
    return ret;
}

void JSVisibleManager::onCreateProxy(ProxyObjectPtr p)
{
    mCtx->objStrand->post(
        std::tr1::bind(&JSVisibleManager::iOnCreateProxy,this,
            p));
}

void JSVisibleManager::iOnCreateProxy(ProxyObjectPtr p)
{
    p->PositionProvider::addListener(this);
    p->MeshProvider::addListener(this);

    JSAggregateVisibleDataPtr data = getOrCreateVisible(p->getObjectReference());
    data->updateFrom(p);
    data->incref(data);
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
    p->PositionProvider::removeListener(this);
    p->MeshProvider::removeListener(this);

    JSAggregateVisibleDataPtr data = getOrCreateVisible(p->getObjectReference());
    data->updateFrom(p);
    data->decref();
    mTrackedObjects.erase(p);
}

void JSVisibleManager::updateLocation(ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef) {
    mCtx->objStrand->post(std::tr1::bind(&JSVisibleManager::iUpdatedProxy, this, proxy));
}

void JSVisibleManager::onSetMesh(ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef) {
    mCtx->objStrand->post(std::tr1::bind(&JSVisibleManager::iUpdatedProxy, this, proxy));
}

void JSVisibleManager::onSetScale(ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef) {
    mCtx->objStrand->post(std::tr1::bind(&JSVisibleManager::iUpdatedProxy, this, proxy));
}

void JSVisibleManager::onSetPhysics(ProxyObjectPtr proxy, const String& newphy,const SpaceObjectReference& sporef) {
    mCtx->objStrand->post(std::tr1::bind(&JSVisibleManager::iUpdatedProxy, this, proxy));
}

void JSVisibleManager::iUpdatedProxy(ProxyObjectPtr p) {
    JSAggregateVisibleDataPtr data = getOrCreateVisible(p->getObjectReference());
    data->updateFrom(p);
}

bool JSVisibleManager::isVisible(const SpaceObjectReference& sporef)
{
    // TODO(ewencp) This shouldn't be getOrCreate, it should just be get.
    JSAggregateVisibleDataPtr data = getOrCreateVisible(sporef);
    return data->visibleToPresence();
}

v8::Handle<v8::Value> JSVisibleManager::isVisibleV8(const SpaceObjectReference& sporef)
{
    return v8::Boolean::New(isVisible(sporef));
}

}//end namespace JS
}//end namespace Sirikata
