// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "JSVisibleData.hpp"

namespace Sirikata {
namespace JS {


JSVisibleData::~JSVisibleData()
{
}

void JSVisibleData::clearFromParent() {
    // We're guaranteed by shared_ptrs and our reference counting that
    // this destructor is only called once there are no more
    // ProxyObjectPtrs and no v8 visibles referring to this data,
    // allowing us to clear this out of the JSVisibleManager.
    if (mParent) mParent->removeVisibleData(this);
}

void JSVisibleData::disable() {
    mParent = NULL;
}



// JSProxyVisibleData
JSProxyVisibleData::JSProxyVisibleData(JSVisibleDataListener* parent, ProxyObjectPtr from)
 : JSVisibleData(parent),
   proxy(from)
{
    proxy->PositionProvider::addListener(this);
    proxy->MeshProvider::addListener(this);
}

JSProxyVisibleData::~JSProxyVisibleData() {
    if (proxy) {
        proxy->PositionProvider::removeListener(this);
        proxy->MeshProvider::removeListener(this);
    }
    clearFromParent();
}

const SpaceObjectReference& JSProxyVisibleData::id() {
    return proxy->getObjectReference();
}

const SpaceObjectReference& JSProxyVisibleData::observer() {
    return proxy->getOwnerPresenceID();
}

void JSProxyVisibleData::disable() {
    proxy->PositionProvider::removeListener(this);
    proxy->MeshProvider::removeListener(this);
    proxy.reset();
    JSVisibleData::disable();
}

// PositionListener Interface
void JSProxyVisibleData::updateLocation(ProxyObjectPtr obj,
    const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient,
    const AggregateBoundingInfo& newBounds, const SpaceObjectReference& sporef)
{
    // FIXME this interface doesn't let us tell exactly what's updated, so we
    // have to send notifications for everything
    notify(&JSVisibleDataEventListener::visiblePositionChanged, this);
    notify(&JSVisibleDataEventListener::visibleVelocityChanged, this);
    notify(&JSVisibleDataEventListener::visibleOrientationChanged, this);
    notify(&JSVisibleDataEventListener::visibleOrientationVelChanged, this);
    // scale handled by MeshListener interface
}
// MeshListener Interface
void JSProxyVisibleData::onSetMesh(ProxyObjectPtr proxy, Transfer::URI const& newMesh, const SpaceObjectReference& sporef) {
    notify(&JSVisibleDataEventListener::visibleMeshChanged, this);
}
void JSProxyVisibleData::onSetScale(ProxyObjectPtr proxy, float32 newScale,const SpaceObjectReference& sporef ) {
    notify(&JSVisibleDataEventListener::visibleScaleChanged, this);
}
void JSProxyVisibleData::onSetPhysics(ProxyObjectPtr proxy, const String& phy,const SpaceObjectReference& sporef ) {
    notify(&JSVisibleDataEventListener::visiblePhysicsChanged, this);
}
void JSProxyVisibleData::onSetIsAggregate(ProxyObjectPtr proxy, bool isAggregate, const SpaceObjectReference& sporef ) {
    // Not exposed to VisibleDataEventListeners
}



// JSRestoredVisibleData
JSRestoredVisibleData::JSRestoredVisibleData(JSVisibleDataListener* parent, const SpaceObjectReference& from, JSVisibleDataPtr fromData)
 : JSVisibleData(parent),
   sporefToListenTo(from)
{
    if (fromData) {
        assert(from == fromData->id());
        updateFrom(*fromData);
    }
}

JSRestoredVisibleData::JSRestoredVisibleData(JSVisibleDataListener* parent, const SpaceObjectReference& from, const IPresencePropertiesRead& orig)
 : JSVisibleData(parent),
   sporefToListenTo(from)
{
    updateFrom(orig);
}

JSRestoredVisibleData::~JSRestoredVisibleData() {
    clearFromParent();
}

const SpaceObjectReference& JSRestoredVisibleData::id() {
    return sporefToListenTo;
}

const SpaceObjectReference& JSRestoredVisibleData::observer() {
    // No observer...
    static SpaceObjectReference n = SpaceObjectReference::null();
    return n;
}

void JSRestoredVisibleData::updateFrom(const IPresencePropertiesRead& orig) {
    setLocation(orig.location());
    setOrientation(orig.orientation());
    setBounds(orig.bounds());
    setMesh(orig.mesh());
    setPhysics(orig.physics());

}


// JSAggregateVisibleData
JSAggregateVisibleData::JSAggregateVisibleData(JSVisibleDataListener* parent, const SpaceObjectReference& vis)
 : JSVisibleData(parent),
   refcount(0),
   selfPtr(),
   mBest(SpaceObjectReference::null())
{
    // Note NULL so we don't get notifications since they'd only happen on
    // destruction since we hold strong refs currently.
    // We use null index to indicate no associated presence
    mChildren[SpaceObjectReference::null()] =
        JSVisibleDataPtr(new JSRestoredVisibleData(NULL, vis) );
}

JSAggregateVisibleData::~JSAggregateVisibleData() {
    clearFromParent();
}

const SpaceObjectReference& JSAggregateVisibleData::id() {
    return getBestChild()->id();
}

const SpaceObjectReference& JSAggregateVisibleData::observer() {
    // Can't specify one observer...
    static SpaceObjectReference n = SpaceObjectReference::null();
    return n;
}

// IPresencePropertiesRead Interface
TimedMotionVector3f JSAggregateVisibleData::location() const{
    return getBestChild()->location();
}

TimedMotionQuaternion JSAggregateVisibleData::orientation() const{
    return getBestChild()->orientation();
}

AggregateBoundingInfo JSAggregateVisibleData::bounds() const{
    return getBestChild()->bounds();
}

Transfer::URI JSAggregateVisibleData::mesh() const{
    return getBestChild()->mesh();
}

String JSAggregateVisibleData::physics() const{
    return getBestChild()->physics();
}

bool JSAggregateVisibleData::isAggregate() const{
    return getBestChild()->isAggregate();
}

ObjectReference JSAggregateVisibleData::parent() const{
    return getBestChild()->parent();
}

void JSAggregateVisibleData::removeVisibleData(JSVisibleData* data) {
    Mutex::scoped_lock locker (childMutex);
    mChildren.erase(data->observer());
}

void JSAggregateVisibleData::updateFrom(ProxyObjectPtr proxy) {
    Mutex::scoped_lock locker (childMutex);
    if (mChildren.find(proxy->getObjectReference()) == mChildren.end()) {
        // Note that we currently pass in NULL so we don't get
        // notifications. We'd only get them upon clearing our children list in
        // the destructor anyway.
        JSVisibleDataPtr jsvisdata(new JSProxyVisibleData(NULL, proxy));
        jsvisdata->addListener(this); // JSVisibleDataEventListener
        mChildren[proxy->getObjectReference()] = jsvisdata;
    }
    mBest = proxy->getObjectReference();
}

void JSAggregateVisibleData::updateFrom(const IPresencePropertiesRead& props) {
    Mutex::scoped_lock locker (childMutex);
    if (mChildren.find(SpaceObjectReference::null()) != mChildren.end()) {
        std::tr1::dynamic_pointer_cast<JSRestoredVisibleData>(mChildren[SpaceObjectReference::null()])->updateFrom(props);
    }
    // We don't update mBest because either it's already this, or we have a
    // Proxy, which is preferable.
}

JSVisibleDataPtr JSAggregateVisibleData::getBestChild() const{
    Mutex::scoped_lock locker (const_cast<Mutex&>(childMutex));
    assert(!mChildren.empty());
    assert(mChildren.find(mBest) != mChildren.end());
    return mChildren.find(mBest)->second;
}

void JSAggregateVisibleData::incref(JSVisibleDataPtr self) {
    selfPtr = self;
    refcount++;
}

void JSAggregateVisibleData::decref() {
    assert(refcount > 0);
    refcount--;
    if (refcount == 0) selfPtr.reset();
}

bool JSAggregateVisibleData::visibleToPresence() const {
    return refcount > 0;
}

void JSAggregateVisibleData::disable() {
    Mutex::scoped_lock locker (childMutex);
    mChildren.clear();
    JSVisibleData::disable();
}

void JSAggregateVisibleData::visiblePositionChanged(JSVisibleData* data) {
    notify(&JSVisibleDataEventListener::visiblePositionChanged, this);
}

void JSAggregateVisibleData::visibleVelocityChanged(JSVisibleData* data) {
    notify(&JSVisibleDataEventListener::visibleVelocityChanged, this);
}

void JSAggregateVisibleData::visibleOrientationChanged(JSVisibleData* data) {
    notify(&JSVisibleDataEventListener::visibleOrientationChanged, this);
}

void JSAggregateVisibleData::visibleOrientationVelChanged(JSVisibleData* data) {
    notify(&JSVisibleDataEventListener::visibleOrientationVelChanged, this);
}

void JSAggregateVisibleData::visibleScaleChanged(JSVisibleData* data) {
    notify(&JSVisibleDataEventListener::visibleScaleChanged, this);
}

void JSAggregateVisibleData::visibleMeshChanged(JSVisibleData* data) {
    notify(&JSVisibleDataEventListener::visibleMeshChanged, this);
}

void JSAggregateVisibleData::visiblePhysicsChanged(JSVisibleData* data) {
    notify(&JSVisibleDataEventListener::visiblePhysicsChanged, this);
}

} // namespace JS
} // namespace Sirikata
