#include "JSVisibleManager.hpp"
#include "JSObjectStructs/JSPositionListener.hpp"
#include "JSLogging.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "EmersonScript.hpp"


namespace Sirikata{
namespace JS{


JSProxyData::~JSProxyData()
{
    emerScript->stopTrackingVis(sporefToListenTo);
}



JSVisibleManager::JSVisibleManager(EmersonScript* eScript)
 : emerScript(eScript)
{
}

JSVisibleManager::~JSVisibleManager()
{
}


JSVisibleStruct* JSVisibleManager::createVisStruct(const SpaceObjectReference& whatsVisible,JSProxyData* addParams)
{
    JSProxyPtr toCreateFrom = createProxyPtr( whatsVisible, JSProxyPtr(addParams));
    return new JSVisibleStruct(toCreateFrom);
}


void JSVisibleManager::stopTrackingVis(const SpaceObjectReference& sporef)
{
    SporefProxyMapIter iter = mProxies.find(sporef);
    if (iter == mProxies.end())
    {
        JSLOG(error, "Error in stopTrackingVis.  Requesting to stop tracking a visible that we had not previously been tracking.");
        return;
    }

    mProxies.erase(iter);
    removeListeners(sporef);
}


void JSVisibleManager::removeListeners(const SpaceObjectReference& toRemoveListenersFor)
{
    std::vector<SpaceObjectReference> sporefVec;
    emerScript->mParent->getSpaceObjRefs(sporefVec);

    ProxyObjectPtr returner;
    
    for (std::vector<SpaceObjectReference>::iterator spIter = sporefVec.begin();
         spIter != sporefVec.end(); ++spIter)
    {
        ProxyObjectPtr poPtr = emerScript->mParent->getProxyManager(spIter->space(),spIter->object())->getProxyObject(toRemoveListenersFor);

        if (!poPtr)
            continue;

        poPtr->PositionProvider::removeListener(this);
        poPtr->MeshProvider::removeListener(this);
    }
}


JSProxyPtr JSVisibleManager::createProxyPtr(const SpaceObjectReference& whatsVisible, JSProxyPtr addParams)
{
    SporefProxyMapIter proxIter = mProxies.find(whatsVisible);
    if (proxIter != mProxies.end())
        return JSProxyPtr(proxIter->second);
    
    ProxyObjectPtr ptr= getMostUpToDate(whatsVisible);

    // Had a proxy object that HostedObject was monitoring.  Load it into
    // mProxies and returning.
    if (ptr)
    {
        JSProxyPtr jspd (new JSProxyData(emerScript,whatsVisible,ptr->getTimedMotionVector(),ptr->getTimedMotionQuaternion(),ptr->getBounds(),ptr->getMesh().toString(),ptr->getPhysics()));

        mProxies[whatsVisible] = JSProxyWPtr(jspd);
        setListeners(whatsVisible);
        return jspd;
    }

    //hosted object does not have a visible with sporef whatsVisible.

    //load additional data on the visible from addParams
    if (addParams)
    {
        if (addParams->sporefToListenTo != whatsVisible)
            JSLOG(error, "Erorr in createVisStruct of JSVisibleManager.  Expected sporefs of new visible should match whatsVisible");

        
        JSProxyPtr returner (new JSProxyData(emerScript,addParams));
        mProxies[whatsVisible] = JSProxyWPtr(returner);
        return returner;
    }

    
    //Have no record of this visible.  Creating a new entry just for it.
    JSProxyPtr returner ( new JSProxyData(emerScript));
    returner->sporefToListenTo = whatsVisible;
    mProxies[whatsVisible] = JSProxyWPtr(returner);
    return returner;
}




void JSVisibleManager::updateLocation (const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef)
{
    INLINE_GET_OR_CREATE_NEW_JSPROXY_DATA(jspd,sporef,updateLocation);
    
    jspd->sporefToListenTo = sporef;
    jspd->mLocation        = newLocation;
    jspd->mOrientation     = newOrient;
    jspd->mBounds          = newBounds;
}



void JSVisibleManager::onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef)
{
    INLINE_GET_OR_CREATE_NEW_JSPROXY_DATA(jspd,sporef,onSetMesh);
    jspd->mMesh  = newMesh.toString();
}


void JSVisibleManager::onSetScale (ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef)
{
    INLINE_GET_OR_CREATE_NEW_JSPROXY_DATA(jspd,sporef,onSetScale);
    jspd->mBounds = BoundingSphere3f(jspd->mBounds.center(),newScale);
}


void JSVisibleManager::onSetPhysics (ProxyObjectPtr proxy, const String& newphy,const SpaceObjectReference& sporef)
{
    INLINE_GET_OR_CREATE_NEW_JSPROXY_DATA(jspd,sporef,onSetMesh);
    jspd->mPhysics  = newphy;    
}



ProxyObjectPtr JSVisibleManager::getMostUpToDate(const SpaceObjectReference& sporef)
{
    std::vector<SpaceObjectReference> sporefVec;
    emerScript->mParent->getSpaceObjRefs(sporefVec);

    ProxyObjectPtr returner;
    
    for (std::vector<SpaceObjectReference>::iterator spIter = sporefVec.begin();
         spIter != sporefVec.end(); ++spIter)
    {
        ProxyObjectPtr poPtr = emerScript->mParent->getProxyManager(spIter->space(),spIter->object())->getProxyObject(sporef);

        if (!poPtr)
            continue;
        
        if (!returner)
            returner = poPtr;
        else
        {
            //set returner to the proxyobjptr with the freshest position update.
            if(returner->getTimedMotionVector().updateTime() < poPtr->getTimedMotionVector().updateTime())
                returner = poPtr;
        }
    }
    return returner;
}

void JSVisibleManager::setListeners(const SpaceObjectReference& toSetListenersFor)
{
    std::vector<SpaceObjectReference> sporefVec;
    emerScript->mParent->getSpaceObjRefs(sporefVec);
    for (std::vector<SpaceObjectReference>::iterator spIter = sporefVec.begin();
         spIter != sporefVec.end(); ++spIter)
    {
        ProxyObjectPtr poPtr = emerScript->mParent->getProxyManager(spIter->space(),spIter->object())->getProxyObject(toSetListenersFor);
        if (!poPtr)
            continue;

        poPtr->PositionProvider::addListener(this);
        poPtr->MeshProvider::addListener(this);
    }
}


bool JSVisibleManager::isVisible(const SpaceObjectReference& sporef)
{
    if (getMostUpToDate(sporef))
        return true;
    
    return false;
}

v8::Handle<v8::Value> JSVisibleManager::isVisibleV8(const SpaceObjectReference& sporef)
{
    return v8::Boolean::New(isVisible(sporef));
}


void JSVisibleManager::onCreateProxy(ProxyObjectPtr p)
{
    SporefProxyMapIter findIt = mProxies.find(p->getObjectReference());
    if (findIt != mProxies.end())
    {
        //listen for updats from p as well as updating data in mProxies
        p->PositionProvider::addListener(this);
        p->MeshProvider::addListener(this);

        JSProxyPtr ptr (findIt->second);
        ptr->mLocation    = p->getTimedMotionVector();
        ptr->mOrientation = p->getTimedMotionQuaternion();
        ptr->mBounds      = p->getBounds();
        ptr->mMesh        = p->getMesh().toString();
        ptr->mPhysics     = p->getPhysics();
    }
}


void JSVisibleManager::onDestroyProxy(ProxyObjectPtr p)
{
    SporefProxyMapIter findIt = mProxies.find(p->getObjectReference());
    if (findIt != mProxies.end())
    {
        p->PositionProvider::removeListener(this);
        p->MeshProvider::removeListener(this);
    }
}






}//end namespace JS
}//end namespace Sirikata
