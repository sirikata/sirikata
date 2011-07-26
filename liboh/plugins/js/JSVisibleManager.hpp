#ifndef __JS_VISIBLE_MANAGER_HPP__
#define __JS_VISIBLE_MANAGER_HPP__


#include <map>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include <sirikata/proxyobject/PositionListener.hpp>
#include <sirikata/proxyobject/MeshListener.hpp>
#include "JSUtil.hpp"

namespace Sirikata{
namespace JS{

class EmersonScript;
class JSVisibleStruct;

class JSProxyData;
typedef std::tr1::weak_ptr<JSProxyData>   JSProxyWPtr;
typedef std::tr1::shared_ptr<JSProxyData> JSProxyPtr;

struct JSProxyData {
public:
    // Default constructor, indicates empty information for when we need a
    // placeholder but don't even have a real identifier from the space yet.
    JSProxyData();
    // Regular constructors representing real data (although possibly from
    // restored data, i.e. not valid according to the space).
    JSProxyData(EmersonScript* eScript, const SpaceObjectReference& from);
    JSProxyData(EmersonScript* eScript, ProxyObjectPtr from);
    JSProxyData(EmersonScript* eScript, const SpaceObjectReference& from, JSProxyPtr fromData);
    ~JSProxyData();

    // Called for *ProxyObject* references.
    void incref(JSProxyPtr self);
    void decref();

    void updateFrom(ProxyObjectPtr from);

    // Indicates whether this presence is visible to a presence or not, i.e. if
    // any ProxyObject still exists for it.
    bool visibleToPresence() const;


    EmersonScript*                  emerScript;
    SpaceObjectReference      sporefToListenTo;
    TimedMotionVector3f              mLocation;
    TimedMotionQuaternion         mOrientation;
    BoundingSphere3f                   mBounds;
    String                               mMesh;
    String                            mPhysics;

private:
    // Number of references to this JSProxyData *by ProxyObjectPtrs*
    // (not by v8 objects). We use a slightly confusing setup to
    // manage the lifetime of the object. The refcount forces this
    // object to maintain a shared_ptr to itself, guaranteeing it'll
    // stay alive. For each v8 visible, there is also a shared_ptr.
    int32 refcount;
    JSProxyPtr selfPtr;
};


/** JSVisibleManager manages the data underlying visibles and presences in
 *  Emerson. For each visible, there isn't currently one canonical visible
 *  object in Emerson. Instead, we can create many of them on demand as we need
 *  them, pointing at the single, internal JSProxyData. JSProxyData is saved in
 *  the JSVisibleManager as a weak pointer so it can be cleaned up automatically
 *  when v8 cleans up all visibles pointing to the data. Visibles and presences
 *  maintain shared pointers to keep them alive while those objects need them,
 *  and their C++ data structures are cleaned up automatically when v8 collects
 *  the JavaScript objects.
 *
 *  JSProxyDatas represent the current known state for a
 *  visible. Information could be coming from multiple ProxyObjects,
 *  or even none at all, e.g. if we found out about the visible via a
 *  message. JSProxyData is created due to new ProxyObjects or due to
 *  createVisStruct calls (in that case containing empty
 *  information). Either of two things keep the JSProxyData alive:
 *  living ProxyObjectPtrs (through reference count + shared_ptr) and
 *  existing v8 visible/presence objects (through shared_ptr to JSProxyData).
 *  This setup allows us to trivially manage listening to
 *  ProxyObjects, always update the JSProxyData as we get updates from
 *  ProxyObjects, and makes dealing with v8 garbage collection easy.
 */
class JSVisibleManager : public ProxyCreationListener,
                         public PositionListener,
                         public MeshListener
{
public:
    JSVisibleManager(EmersonScript* eScript);
    virtual ~JSVisibleManager();

    /**
       Creates a new visible struct with sporef whatsVisible.  First checks if
       already have visible with sporef whatsVisible in mProxies.  If do,
       creates new visible struct from weak pointer stored there.  If do not,
       then checks if hosted object has any proxy objects with given sporef, if
       not, then tries to load from addParams.  If addParams is null, then just
       creates a blank proxy data object with whatsVisible as its sporef.
     */
    JSVisibleStruct* createVisStruct(const SpaceObjectReference& whatsVisible, JSProxyPtr addParams = JSProxyPtr());

    // Get or create a JSProxyData for this object. Without data specified, this
    // will result in some default values.  This *doesn't* affect reference
    // count at all -- you can get a JSProxyPtr back that has just been
    // initialized with a ProxyObjectPtr refcount of 0, meaning if you release
    // the JSProxyPtr it will be completely cleaned up.
    JSProxyPtr getOrCreateProxyPtr(const SpaceObjectReference& whatsVisible, JSProxyPtr data = JSProxyPtr());


    // ProxyCreationListener Interface
    //  - Creates or increments and decrements refcount of
    //    JSProxyData. Updates data.
    virtual void onCreateProxy(ProxyObjectPtr p);
    virtual void onDestroyProxy(ProxyObjectPtr p);

    // PositionListener
    //  - Updates JSProxyData state
    //  - Destruction ignored, handled by onDestroyProxy
    virtual void updateLocation (const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef);
    virtual void destroyed(){}

    // MeshListener Interface
    //  - Updates JSProxyData state
    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef);
    virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef);
    virtual void onSetPhysics (ProxyObjectPtr proxy, const String& newphy,const SpaceObjectReference& sporef);


    // Indicate whether this object is still visible to on of your presences or
    // not.
    bool isVisible(const SpaceObjectReference& sporef);
    v8::Handle<v8::Value> isVisibleV8(const SpaceObjectReference& sporef);
private:
    // Friend JSProxyData so it can tell us to remove it.
    friend class JSProxyData;


    // Utility for initialializing a JSProxyData from a ProxyObject. Only used
    // internally to manage the lifetime based on ProxyObject
    // creation/destruction.
    JSProxyPtr getOrCreateProxyPtr(ProxyObjectPtr proxy);

    // Clean up our references to a JSProxyData. Only ever invoked by
    // JSProxyData's destructor when there are no more references to
    // it.
    void removeProxyData(const SpaceObjectReference& sporef);


    EmersonScript* emerScript;

    typedef std::map<SpaceObjectReference,JSProxyWPtr > SporefProxyMap;
    typedef SporefProxyMap::iterator SporefProxyMapIter;
    SporefProxyMap mProxies;

    typedef std::tr1::unordered_set<ProxyObjectPtr, ProxyObject::Hasher> TrackedObjectsMap;
    TrackedObjectsMap mTrackedObjects;
};

} //end namespace js
} //end namespace sirikata

#endif
