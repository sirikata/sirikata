#ifndef __JS_VISIBLE_MANAGER_HPP__
#define __JS_VISIBLE_MANAGER_HPP__


#include <map>
#include <sirikata/core/util/Platform.hpp>
#include <tr1/memory>
#include "JSObjectStructs/JSVisibleStruct.hpp"

namespace Sirikata{
namespace JS{

class EmersonScript;
class JSProxyData;
typedef std::tr1::weak_ptr<JSProxyData>   JSProxyWPtr;
typedef std::tr1::shared_ptr<JSProxyData> JSProxyPtr;



struct JSProxyData 
{
    JSProxyData(EmersonScript* eScript)
     : emerScript(eScript)
    {}

    JSProxyData(EmersonScript* eScript,const SpaceObjectReference& _sporefToListenTo,const TimedMotionVector3f& _mLocation,const TimedMotionQuaternion& _mOrientation, const BoundingSphere3f& _mBounds,const String& _mMesh, const String& _mPhysics)
     : emerScript(eScript),
       sporefToListenTo(_sporefToListenTo),
       mLocation(_mLocation),
       mOrientation(_mOrientation),
       mBounds(_mBounds),
       mMesh(_mMesh),
       mPhysics(_mPhysics)
    {}

    JSProxyData(EmersonScript* eScript, JSProxyPtr from)
     : emerScript(eScript),
       sporefToListenTo(from->sporefToListenTo),
       mLocation(from->mLocation),
       mOrientation(from->mOrientation),
       mBounds(from->mBounds),
       mMesh(from->mMesh),
       mPhysics(from->mPhysics)
    {}
    
    // JSProxyData(EmersonScript* eScript,JSProxyData* from)
    //  : emerScript(eScript),
    //    sporefToListenTo(from->sporefToListenTo),
    //    mLocation(from->mLocation),
    //    mOrientation(from->mOrientation),
    //    mBounds(from->mBounds),
    //    mMesh(from->mMesh),
    //    mPhysics(from->mPhysics)
    // {}
    
    ~JSProxyData();

    
    EmersonScript*                  emerScript;
    SpaceObjectReference      sporefToListenTo;
    TimedMotionVector3f              mLocation;
    TimedMotionQuaternion         mOrientation;
    BoundingSphere3f                   mBounds;
    String                               mMesh;
    String                            mPhysics;
};





class JSVisibleManager : public ProxyCreationListener,
                         public PositionListener,
                         public MeshListener
{
public:
    JSVisibleManager(EmersonScript* eScript);
    ~JSVisibleManager();

    

    /**
       Creates a new visible struct with sporef whatsVisible.  First checks if
       already have visible with sporef whatsVisible in mProxies.  If do,
       creates new visible struct from weak pointer stored there.  If do not,
       then checks if hosted object has any proxy objects with given sporef, if
       not, then tries to load from addParams.  If addParams is null, then just
       creates a blank proxy data object with whatsVisible as its sporef.
     */
    JSVisibleStruct* createVisStruct(const SpaceObjectReference& whatsVisible, JSProxyData* addParams= NULL);


    /**
       If ho does not have a proxy obj with whatsVisible as its sporef, checks
       addParams if addParams is non-null, returned proxyptr will be
       initialized with the data in addParams.  Otherwise, what's returned will
       have defaults for all its position, velocity, etc. fields (likely 0s.).
     */
    JSProxyPtr createProxyPtr(const SpaceObjectReference& whatsVisible, JSProxyPtr addParams);

  
    
    /**
       If a new proxy object is available and its sporef is in mProxies, update
       mProxies and with the new proxies fields and set JSVisibleManager as a
       listener for it.
     */
    virtual void onCreateProxy(ProxyObjectPtr p);
    
    /**
       If sporef of p exists in mProxies, then at some point, we must have
       registered JSVisibleManager as a position and mesh listener for p.
       De-register JSVisibleManager on destruction.
     */
    virtual void onDestroyProxy(ProxyObjectPtr p);
    /**
       Don't need to do anything for destroyed because we catch destruction from onDestroyProxy.
     */
    virtual void destroyed()
    {}
    
    
    virtual void updateLocation (const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef);
    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef);
    virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef);
    virtual void onSetPhysics (ProxyObjectPtr proxy, const String& newphy,const SpaceObjectReference& sporef);

    /**
       The destructor of the visible object should call this function with its
       sporef as argument.

       Removes corresponding entry from mProxies and de-registers all listeners
       that were registered through objecthostproxymanager.
     */
    void stopTrackingVis(const SpaceObjectReference& sporef);

    bool isVisible(const SpaceObjectReference& sporef);
    
private:

  
    /**
       Runs through all the presences on this entity.  For each presence, checks
       if have a proxy object associated with sporef.  Return the proxy object
       with the most up to date position update.
    */
    ProxyObjectPtr getMostUpToDate(const SpaceObjectReference& sporef);

    /**
       Sets JSVisibleManager.hpp as a listener for all proxy objects that have
       sporef toSetListenersFor.
     */
    void setListeners(const SpaceObjectReference& toSetListenersFor);

    /**
       Removes JSVisibleManager.hpp from being a listener for all proxy objects
       that have the sporef toRemoveListenersFor.
     */
    void removeListeners(const SpaceObjectReference& toRemoveListenersFor);
    
    

    v8::Handle<v8::Value> isVisibleV8(const SpaceObjectReference& sporef);
    
    EmersonScript* emerScript;
    
    typedef std::map<SpaceObjectReference,JSProxyWPtr > SporefProxyMap;
    typedef SporefProxyMap::iterator SporefProxyMapIter;
    SporefProxyMap mProxies;
    
};


/**
   @param nameProxy After calling this, nameProxy will be a shared pointer
   (pointing to JSProxyData), with
   the most up-to-date data associated with the sporef with name nameToMatch in mProxies.
   @param {sporef} nameToMatch Searches mProxies for JSProxyData* with the key
   nameToMatch.  If finds one, loads that JSProxyData into nameProxy.
   Otherwise, creates a new one, and inserts it.
   @param {String} errorWhere Should be the name of the function that is calling
   this macro.  Prints logging information if receive a position update,
   etc. for proxy objects that we didn't realize had already been created.
 */
#define INLINE_GET_OR_CREATE_NEW_JSPROXY_DATA(nameProxy,nameToMatch,errorWhere) \
JSProxyPtr nameProxy;\
{\
    SporefProxyMapIter iter = mProxies.find(nameToMatch);\
    if (iter == mProxies.end())\
    {\
        JSLOG(error,"Error in " #errorWhere "of JSVisibleManager.  Should have received a createProxy call for sporef before an update.  Adding element in case.");\
\
        nameProxy =JSProxyPtr(new JSProxyData(emerScript)); \
        mProxies[nameToMatch] = JSProxyWPtr(nameProxy); \
    }\
    else\
        nameProxy = JSProxyPtr(iter->second);    \
}

} //end namespace js
} //end namespace sirikata

#endif
