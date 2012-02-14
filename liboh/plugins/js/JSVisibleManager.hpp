#ifndef __JS_VISIBLE_MANAGER_HPP__
#define __JS_VISIBLE_MANAGER_HPP__

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include "JSUtil.hpp"
#include "JSCtx.hpp"
#include "JSVisibleData.hpp"

namespace Sirikata{
namespace JS{

class EmersonScript;
class JSVisibleStruct;

/** JSVisibleManager manages the data underlying visibles and presences in
 *  Emerson. For each visible, there isn't currently one canonical visible
 *  object in Emerson. Instead, we can create many of them on demand as we need
 *  them, pointing at the single, internal JSVisibleData. JSVisibleData is saved in
 *  the JSVisibleManager as a weak pointer so it can be cleaned up automatically
 *  when v8 cleans up all visibles pointing to the data. Visibles and presences
 *  maintain shared pointers to keep them alive while those objects need them,
 *  and their C++ data structures are cleaned up automatically when v8 collects
 *  the JavaScript objects.
 *
 *  JSVisibleDatas represent the current known state for a
 *  visible. Information could be coming from multiple ProxyObjects,
 *  or even none at all, e.g. if we found out about the visible via a
 *  message. JSVisibleData is created due to new ProxyObjects or due to
 *  createVisStruct calls (in that case containing empty
 *  information). Either of two things keep the JSVisibleData alive:
 *  living ProxyObjectPtrs (through reference count + shared_ptr) and
 *  existing v8 visible/presence objects (through shared_ptr to JSVisibleData).
 *  This setup allows us to trivially manage listening to
 *  ProxyObjects, always update the JSVisibleData as we get updates from
 *  ProxyObjects, and makes dealing with v8 garbage collection easy.
 */
class JSVisibleManager :
        public ProxyCreationListener,
        public JSVisibleDataListener,
        public PositionListener,
        public MeshListener
{
public:
    JSVisibleManager(JSCtx* ctx);
    virtual ~JSVisibleManager();
    
    typedef boost::recursive_mutex RMutex;
    RMutex vmMtx;

    
    /**
       Creates a new visible struct with sporef whatsVisible.  First checks if
       already have visible with sporef whatsVisible in mProxies.  If do,
       creates new visible struct from weak pointer stored there.  If do not,
       then checks if hosted object has any proxy objects with given sporef, if
       not, then tries to load from addParams.  If addParams is null, then just
       creates a blank proxy data object with whatsVisible as its sporef.
     */
    JSVisibleStruct* createVisStruct(EmersonScript* parent, const SpaceObjectReference& whatsVisible, JSVisibleDataPtr addParams = JSVisibleDataPtr());

    // Looks up or creates the data for a visible, but leaves it
    // uninitialized. You should only call this if you know data will be
    // available or if you follow it up by initializing the data (e.g. with a
    // ProxyObject).
    JSAggregateVisibleDataPtr getOrCreateVisible(const SpaceObjectReference& whatsVisible);

    // ProxyCreationListener Interface
    //  - Creates or increments and decrements refcount of
    //    JSVisibleData. Updates data.
    virtual void onCreateProxy(ProxyObjectPtr p);
    virtual void onDestroyProxy(ProxyObjectPtr p);

    // PositionListener
    //  - Updates JSProxyData state
    //  - Destruction ignored, handled by onDestroyProxy
    virtual void updateLocation (ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef);

    // MeshListener Interface
    //  - Updates JSProxyData state
    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef);
    virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef);
    virtual void onSetPhysics (ProxyObjectPtr proxy, const String& newphy,const SpaceObjectReference& sporef);

    // Indicate whether this object is still visible to on of your presences or
    // not.
    bool isVisible(const SpaceObjectReference& sporef);
    v8::Handle<v8::Value> isVisibleV8(const SpaceObjectReference& sporef);



protected:
    void iOnDestroyProxy(ProxyObjectPtr p);


    // Invoked when we received an update on a Proxy, making it the most up-to-date.
    void iUpdatedProxy(ProxyObjectPtr p);

private:

    void iOnCreateProxy(ProxyObjectPtr p);
    void clearVisibles();

    

    // Clean up our references to a JSVisibleData. Only ever invoked by
    // JSVisibleData's destructor when there are no more references to
    // it.
    virtual void removeVisibleData(JSVisibleData* data);

    JSCtx* mCtx;

    typedef std::map<SpaceObjectReference, JSAggregateVisibleDataWPtr > SporefProxyMap;
    typedef SporefProxyMap::iterator SporefProxyMapIter;
    SporefProxyMap mProxies;

    typedef std::tr1::unordered_set<ProxyObjectPtr, ProxyObject::Hasher> TrackedObjectsMap;
    TrackedObjectsMap mTrackedObjects;


    friend class EmersonScript;
};

} //end namespace js
} //end namespace sirikata

#endif
