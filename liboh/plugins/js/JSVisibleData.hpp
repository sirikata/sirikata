// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_JS_VISIBLE_DATA_HPP_
#define _SIRIKATA_JS_VISIBLE_DATA_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/PresenceProperties.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>

namespace Sirikata{
namespace JS{

class JSVisibleStruct;

class JSVisibleData;
typedef std::tr1::weak_ptr<JSVisibleData>   JSVisibleDataWPtr;
typedef std::tr1::shared_ptr<JSVisibleData> JSVisibleDataPtr;

/** Tracks the lifetime of JSVisibleData. */
class JSVisibleDataListener {
public:
    virtual ~JSVisibleDataListener() {}

    virtual void removeVisibleData(JSVisibleData* data) = 0;
};

/** JSVisibleData is the interface for accessing data about visibles. It is
 *  only an interface because we need to support regular operation (against
 *  ProxyObjects) and special cases (like during restoration, when we've stored
 *  a copy of values but don't have access to a real ProxyObject.
 *
 *  You actually can't assume much about what's available and valid in here
 *  because visibles can come from so many places: if they originate from
 *  proximity results (they are backed by ProxyObjects) they'll hold all
 *  information, but they can also be restored (not associated with a particular
 *  parent presence), can be deserialized from a message from another host (in
 *  which case we'd just get an identifier) or just be the origin of a message
 *  (again, meaning only the ID will be valid).
 */
class JSVisibleData : public virtual IPresencePropertiesRead {
public:
    JSVisibleData(JSVisibleDataListener* parent)
     : mParent(parent)
    {}
    ~JSVisibleData();

    /** ID of this object in the space. */
    virtual const SpaceObjectReference& id() = 0;
    /** ID of the observing presence of this object. */
    virtual const SpaceObjectReference& observer() = 0;

    // "Disables" this data, making it invalid. Only used when we know
    // its safe to destroy everything because the script is being
    // killed. Also ensures we are able to clean up since references
    // to proxies, which hold references to the parent object, are
    // cleared by this.
    virtual void disable();
protected:
    // Clear the data for this visible (i.e. for the ID given) from the parent script.
    void clearFromParent();

    JSVisibleDataListener* mParent;
};

class JSRestoredVisibleData;
typedef std::tr1::shared_ptr<JSRestoredVisibleData> JSRestoredVisibleDataPtr;
/** JSVisibleData that uses static data. */
class JSRestoredVisibleData : public JSVisibleData, public PresenceProperties {
public:
    // Regular constructors representing real data (although possibly from
    // restored data, i.e. not valid according to the space).
    JSRestoredVisibleData(JSVisibleDataListener* parent, const SpaceObjectReference& from, JSVisibleDataPtr fromData = JSVisibleDataPtr());
    JSRestoredVisibleData(JSVisibleDataListener* parent, const SpaceObjectReference& from, const IPresencePropertiesRead& orig);

    virtual ~JSRestoredVisibleData();

    virtual const SpaceObjectReference& id();
    virtual const SpaceObjectReference& observer();

    // IPresencePropertiesRead Interface is implemented by PresenceProperties

    void updateFrom(const IPresencePropertiesRead& orig);
private:
    SpaceObjectReference sporefToListenTo;

    JSRestoredVisibleData();
};


/** JSVisibleData that works from a ProxyObject. */
class JSProxyVisibleData : public JSVisibleData {
public:
    JSProxyVisibleData(JSVisibleDataListener* parent, ProxyObjectPtr from);
    virtual ~JSProxyVisibleData();

    virtual const SpaceObjectReference& id();
    virtual const SpaceObjectReference& observer();

    virtual void disable();

    // IPresencePropertiesRead Interface
    virtual TimedMotionVector3f location() const { return proxy->location(); }
    virtual TimedMotionQuaternion orientation() const { return proxy->orientation(); }
    virtual BoundingSphere3f bounds() const { return proxy->bounds(); }
    virtual Transfer::URI mesh() const { return proxy->mesh(); }
    virtual String physics() const { return proxy->physics(); }
    virtual bool isAggregate() const { return proxy->isAggregate(); }
    virtual ObjectReference parent() const { return proxy->parentAggregate(); }

private:
    JSProxyVisibleData();

    ProxyObjectPtr proxy;
};



class JSAggregateVisibleData;
typedef std::tr1::shared_ptr<JSAggregateVisibleData> JSAggregateVisibleDataPtr;
typedef std::tr1::weak_ptr<JSAggregateVisibleData> JSAggregateVisibleDataWPtr;
/** JSVisibleData that aggregates multiple other JSVisibleDatas, presenting the
 *  best information available at the time.
 */
class JSAggregateVisibleData :
        public JSVisibleData,
        public JSVisibleDataListener,
        Noncopyable
{
public:
    // Construct aggregate, starting with an ID (e.g. when we've received a
    // message) and possibly some restored data (e.g. when we're restoring).
    JSAggregateVisibleData(JSVisibleDataListener* parent, const SpaceObjectReference& vis);
    virtual ~JSAggregateVisibleData();

    virtual const SpaceObjectReference& id();
    virtual const SpaceObjectReference& observer();

    virtual void disable();

    // Indicates whether this presence is visible to a presence or not, i.e. if
    // any ProxyObject still exists for it.
    bool visibleToPresence() const;

    // IPresencePropertiesRead Interface
    virtual TimedMotionVector3f location() const;
    virtual TimedMotionQuaternion orientation() const;
    virtual BoundingSphere3f bounds() const;
    virtual Transfer::URI mesh() const;
    virtual String physics() const;
    virtual bool isAggregate() const;
    virtual ObjectReference parent() const;

    // JSVisibleDataListener
    virtual void removeVisibleData(JSVisibleData* data);

    void updateFrom(ProxyObjectPtr proxy);
    void updateFrom(const IPresencePropertiesRead& props);

    // Called for *ProxyObject* references.
    void incref(JSVisibleDataPtr self);
    void decref();

private:
    JSAggregateVisibleData();

    JSVisibleDataPtr getBestChild() const;

    // Number of references to this JSVisibleData *by ProxyObjectPtrs*
    // (not by v8 objects). We use a slightly confusing setup to
    // manage the lifetime of the object. The refcount forces this
    // object to maintain a shared_ptr to itself, guaranteeing it'll
    // stay alive. For each v8 visible, there is also a shared_ptr.
    int32 refcount;
    JSVisibleDataPtr selfPtr;

    // Within an aggregate, all IDs should be the same. We index by the owning
    // presence of each individual Visible.
    // TODO(ewencp) These have to be strong references since nothing else will
    // hold on to them, but then they will just continue to aggregate unless we
    // have some way of turning them weak + maintaining the latest updated child
    // as a strong ref.
    typedef std::map<SpaceObjectReference, JSVisibleDataPtr> ChildMap;
    ChildMap mChildren;
    SpaceObjectReference mBest;

    typedef boost::mutex Mutex;
    Mutex childMutex;


};

} // namespace JS
} // namespace Sirikata

#endif // _SIRIKATA_JS_VISIBLE_DATA_HPP_
