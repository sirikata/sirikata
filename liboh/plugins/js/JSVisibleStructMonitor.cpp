#include "JSVisibleStructMonitor.hpp"
#include "JSObjectStructs/JSPositionListener.hpp"
#include "JSLogging.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "EmersonScript.hpp"

namespace Sirikata{
namespace JS{


JSVisibleStructMonitor::JSVisibleStructMonitor()
{
}

JSVisibleStructMonitor::~JSVisibleStructMonitor()
{
}


//see comments in hpp file above listenFromNoneMap and mObjectsToFollow
void JSVisibleStructMonitor::checkNotifyNowVis(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleFrom)
{
    JSVisibleStruct* noListenFrom = checkNoListenFrom(sporefVisible);
    if (noListenFrom != NULL)
        noListenFrom->notifyVisible();

    JSVisibleStruct* hasListenerFrom = checkListenFromVisStructs(sporefVisible,sporefVisibleFrom);
    if (hasListenerFrom != NULL)
        hasListenerFrom->notifyVisible();
}


void JSVisibleStructMonitor::checkNotifyNowNotVis(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleFrom)
{
    //sends notification to objects that are are being watched from associated
    //presence sporefVisibleFrom
    JSVisibleStruct* jsvis =checkListenFromVisStructs(sporefVisible,sporefVisibleFrom);
    if (jsvis != NULL)
        jsvis->notifyNotVisible();

    checkNotifyNowNotVis_noVisFrom(sporefVisible);
}

//only notify visible structs that do not have an associated local presence from
//which the object is visible that they are no longer visible if they are
//no longer visible to any visible structs that *do* have a local presence
//associated with them.
void JSVisibleStructMonitor::checkNotifyNowNotVis_noVisFrom(const SpaceObjectReference& sporefVisible)
{
    //first check if there are any non-visible objects to potentially notify
    //that they are now visible.
    JSVisibleStruct* jsvis = checkNoListenFrom(sporefVisible);
    if (jsvis != NULL)
    {
        //checks if there exists a visible structs that is watching object in world
        //with sporef sporefVisible and with a local presence that the visible
        //struct is watching the object from, *and* that is stillvisible.
        JSVisibleStruct* watchingWithFrom = checkWatchingWithFrom(sporefVisible);
        
        //if nothing else is watching the presence, notify the jsvisible struct
        //that it is no longer visible.
        if (watchingWithFrom == NULL)
            jsvis->notifyNotVisible();

    }
}


//those visible structs that receive an update from 
void JSVisibleStructMonitor::checkForwardUpdate(const SpaceObjectReference& sporefVisible, const TimedMotionVector3f& tmv, const TimedMotionQuaternion& tmq,const BoundingSphere3f& newBounds)
{
    SpaceToVisMapIter iter = listenFromNoneMap.find(sporefVisible);
    if (iter != listenFromNoneMap.end())
        iter->second->updateLocation(tmv,tmq,newBounds,sporefVisible);
}

void JSVisibleStructMonitor::checkForwardUpdateMesh(const SpaceObjectReference& sporefVisible,ProxyObjectPtr proxptr,Transfer::URI const& newMesh)
{
    SpaceToVisMapIter iter = listenFromNoneMap.find(sporefVisible);

    if (iter != listenFromNoneMap.end())
        iter->second->onSetMesh(proxptr,newMesh,sporefVisible);
}




//If we have a visible struct that matches the attached sporefs, then return
//it.  otherwise, return null.
JSVisibleStruct* JSVisibleStructMonitor::checkVisStructExists(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleFrom)
{
    if (sporefVisibleFrom == SpaceObjectReference::null())
        return checkNoListenFrom(sporefVisible);

    return checkListenFromVisStructs(sporefVisible,sporefVisibleFrom);
}

JSVisibleStruct* JSVisibleStructMonitor::checkVisStructExists(const SpaceObjectReference& sporefVisible)
{
    ListenFromMapIter iter= mObjectsToFollow.find(sporefVisible);

    if (iter != mObjectsToFollow.end())
    {
        JSVisibleStruct* jsvis = NULL;
        for (SpaceToVisMapIter stvmi = iter->second.begin(); stvmi  != iter->second.end(); ++stvmi)
        {
            if (stvmi->second->getStillVisibleCPP())
                return stvmi->second;

            jsvis = stvmi->second;
        }
        return jsvis;
    }

    return NULL;
}


//This function runs through the map that does not know from whom an object is
//visible.
//returns visible struct if it's found in the list of items.  otherwise, returns null.
JSVisibleStruct* JSVisibleStructMonitor::checkNoListenFrom(const SpaceObjectReference& sporefVisible)
{
    SpaceToVisMapIter iter = listenFromNoneMap.find(sporefVisible);
    if (iter == listenFromNoneMap.end())
        return NULL;

    return iter->second;
}


//This runs through the map that knows from whom the objects are visible.
//in this case sporefVisible is visible to presence with space object reference
//of sporefVisibleFrom
JSVisibleStruct* JSVisibleStructMonitor::checkListenFromVisStructs(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleFrom)
{
    //check if we have any jsvisibleStructs that are trying to follow sporefVisible
    ListenFromMapIter tl_from_mi = mObjectsToFollow.find(sporefVisible);
    if (tl_from_mi == mObjectsToFollow.end())
        return NULL;

    //check if we have any jsvisibleStructs that are associated with presence sporefVisibleFrom
    SpaceToVisMapIter  spaceToVisIter = tl_from_mi->second.find(sporefVisibleFrom);
    if (spaceToVisIter == tl_from_mi->second.end())
        return NULL;

    return spaceToVisIter->second;
}




//if there's an object in the world with sporef A, then this
//function runs through all the visible structs that are watching it that have
//an associated local presence watching it from, and
//returns one of them that is still visible.
//otherwise, returns null.
JSVisibleStruct* JSVisibleStructMonitor::checkWatchingWithFrom(const SpaceObjectReference& sporefVisible)
{
    ListenFromMapIter iter = mObjectsToFollow.find(sporefVisible);

    if (iter == mObjectsToFollow.end())
        return NULL;

    for (SpaceToVisMapIter spVisIter = iter->second.begin(); spVisIter != iter->second.end(); ++spVisIter)
    {
        if (spVisIter->second->getStillVisibleCPP())
            return spVisIter->second;
    }
    return NULL;
}




//test whether a visibleStruct already exists that has identical
//whatsVisible and toWhom sporefs.  If it does, returns that visible struct.
//if it does not, creates a new visible struct, registers it with the monitor,
//and returns it.
JSVisibleStruct* JSVisibleStructMonitor::createVisStruct(EmersonScript* emerscript, const SpaceObjectReference& whatsVisible, const SpaceObjectReference& visibleTo, VisAddParams* addParams)
{
    if (visibleTo == SpaceObjectReference::null())
        return createVisStructFromNone(emerscript,whatsVisible);

    return createVisStructFromHaveListeners(emerscript,whatsVisible,visibleTo, addParams);
}



//checks mObjectsToFollow if we already have a visible struct that is tracking a
//proximate object in the world that has sporef whatsVisible from an internal
//presence with sporef toWhom.  If we do, then return it.  If we don't, create it.
//When creating it, check to see if we should notify any jsvisiblestructs in
//listenFromNoneMap to update their visibility
JSVisibleStruct* JSVisibleStructMonitor::createVisStructFromHaveListeners(EmersonScript* emerscript,const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom, VisAddParams* addParams)
{

    //check mObjectsToFollow if already have a jsvisiblestruct with that is
    //tracking a proxy object with sporef whatsVisible that is visible to a
    //presence with sporef toWhom
    ListenFromMapIter iter = mObjectsToFollow.find(whatsVisible);
    if (iter != mObjectsToFollow.end())
    {
        SpaceToVisMapIter spVisIter = iter->second.find(toWhom);
        if (spVisIter != iter->second.end())
        {
            if ((addParams != NULL) && (addParams->mIsVisible != NULL) && (*addParams->mIsVisible))
                spVisIter->second->notifyVisible();

            //means we already have this visible struct.  Return it
            return spVisIter->second;
        }
    }

    //do not already have a visible struct.  create a new one.
    JSVisibleStruct* returner = new JSVisibleStruct(emerscript,whatsVisible,toWhom,addParams);

    
    //actually insert the visible struct into our maps
    mObjectsToFollow[whatsVisible].insert(std::pair<SpaceObjectReference,JSVisibleStruct*>(toWhom,returner));
    
    //check if need to update any vis structs in listenFromNoneMap to now be
    //visible and to get new positions.
    updateFromNoneMap(returner,whatsVisible);

    return returner;
}





//checks listenFromNoneMap to see if there is a JSVisibleStruct that is
//watching an object with sporef whatsVisible.  If there is, then returns it
//otherwise, creates a new one, looks through mObjectsToFollow to determine if
//this new jsvisiblestruct should be visible, provides an update for it if
//it should, and returns it.
JSVisibleStruct* JSVisibleStructMonitor::createVisStructFromNone(EmersonScript* emerscript, const SpaceObjectReference& whatsVisible)
{
    //already have a jsvisiblestruct that is watching whatsVisible, but has no
    //associated presence watching from.  Return this jsvisiblestruct.
    JSVisibleStruct* jsvis = checkNoListenFrom(whatsVisible);
    if (jsvis != NULL)
        return jsvis;

        
    //do not already have a jsvisiblestruct that is watching whatsvisible.
    //Creating a new one.
    JSVisibleStruct* returner = new JSVisibleStruct(emerscript,whatsVisible, SpaceObjectReference::null(),NULL);
    //add to map.
    listenFromNoneMap[whatsVisible] = returner;
    
    //now check if should be visible.  sets to notifyNowVisible and gives new
    //timedmotionvector, timedmotionquaternion, and boundingsphere
    updateForFromNone(returner,whatsVisible);

    return returner;
}




//this function takes in a visible struct that we will use to upd
//If there is a visible struct that is in listenFromNoneMap indexed by
//whatsVisible, and this jsvisiblestruct wasn't already visible, (and jsvis is visible),
//then we tell that visible struct that it is now visible and we ask jsvis to
//update it.
void JSVisibleStructMonitor::updateFromNoneMap(JSVisibleStruct* jsvis,const SpaceObjectReference& whatsVisible)
{
    if (jsvis->getStillVisibleCPP())
    {
        SpaceToVisMapIter spVisIter = listenFromNoneMap.find(whatsVisible);
        if (spVisIter != listenFromNoneMap.end())
        {
            if (! spVisIter->second->getStillVisibleCPP())
            {
                jsvis->updateOtherJSPosListener(spVisIter->second);
                spVisIter->second->notifyVisible();
            }
        }
    }
}




//this function looks through mObjectsToFollow to see if any of there is a
//jsvisiblestruct that is watching an object with sporef whatsVisible.  If there
//is, and that object's stillVisible field is marked, then we send to jsvis a
//notifyNowVisible command + we request that the jsvisiblestruct in
//mobjectstofollow send an update to jsvis.
void JSVisibleStructMonitor::updateForFromNone(JSVisibleStruct* jsvis,const SpaceObjectReference& whatsVisible)
{
    JSVisibleStruct* watchingWithFrom = checkWatchingWithFrom(whatsVisible);

    //no visible structs can perform the update on jsvis
    if (watchingWithFrom == NULL)
        return;

    watchingWithFrom->updateOtherJSPosListener(jsvis);
    jsvis->notifyVisible();
}


//takes in jsvis and removes jsvis from mObjetsToFollow and/or listenFromNoneMap
//if they exist.
void JSVisibleStructMonitor::deRegisterVisibleStruct(JSVisibleStruct* jsvis)
{
    JSLOG(insane,"Attempting to deRegister visiblestruct in JSVisibleStructMonitor.cpp");
    if ((jsvis->getToListenFrom() == NULL) ||
        *(jsvis->getToListenFrom()) == SpaceObjectReference::null())
    {
        //means that this visibleStruct does not have
        //an associated presence that is watching it.
        //should be removed from listenFromNoneMap (if it exists there).
        deRegisterVisibleStructFromNoneMap(jsvis);
        return;
    }

    
    //means that this visibleStruct *does* have an
    //associated presence that is watching it.
    //should remove it from mObjectsToFollow
    deRegisterVisibleStructFromObjectsMap(jsvis);
}


//means that this visibleStruct does not have
//an associated presence that is watching it.
//should be removed from listenFromNoneMap (if it exists there).
void JSVisibleStructMonitor::deRegisterVisibleStructFromNoneMap(JSVisibleStruct* jsvis)
{
    JSLOG(insane,"Attempting to deRegister a visible struct with sporef " << *(jsvis->getToListenTo()) <<" from listenFromNoneMap");
    SpaceToVisMapIter iter = listenFromNoneMap.find(*(jsvis->getToListenTo()));
    if (iter == listenFromNoneMap.end())
    {
        JSLOG(info,"Issue deRegistering visible struct from listenFromNoneMap in JSVisibleStructMonitor.cpp: no associated entry to remove.  Possibly concerning");
        return;
    }

    //entry existed.  removing it now.
    listenFromNoneMap.erase(iter);
}


//means that this visibleStruct *does* have an
//associated presence that is watching it.
//should remove it from mObjectsToFollow
void JSVisibleStructMonitor::deRegisterVisibleStructFromObjectsMap(JSVisibleStruct* jsvis)
{
    JSLOG(insane,"Attempting to deRegister a visible struct with sporef " << *(jsvis->getToListenTo()) <<" that is being watched by my presence with sporef: "<<*(jsvis->getToListenFrom()));

    ListenFromMapIter iter = mObjectsToFollow.find(*(jsvis->getToListenTo()));
    if (iter == mObjectsToFollow.end())
    {
        JSLOG(info,"Issue deRegistering visible struct from mObjectsToFollow in JSVisibleStructMonitor.cpp: no associated entry to remove.  Possibly concerning");
        return;
    }

    SpaceToVisMapIter spVisIter = iter->second.find(*(jsvis->getToListenFrom()));
    if (spVisIter == iter->second.end())
    {
        JSLOG(info,"Issue deRegistering visible struct from mObjectsToFollow in JSVisibleStructMonitor.cpp: no associated entry to remove.  Possibly concerning");
        return;
    }


    //see comments before last if statement.
    bool shouldCheckNotifyNowNotVis = false;
    if (spVisIter->second->getStillVisibleCPP())
        shouldCheckNotifyNowNotVis = true;

    
    //remove the entry from our maps.
    iter->second.erase(spVisIter);

    
    //if deregistered an entry that another visible struct was relying on for
    //updates, need to notify it that it is no longer visible.  Only will happen
    //if the item that we de-registered was still visible when we were removing
    //it (as tested by shouldCheckNotifyNowNotvis).
    if (shouldCheckNotifyNowNotVis)
        checkNotifyNowNotVis_noVisFrom(*(jsvis->getToListenTo()));
}




}//end namespace JS
}//end namespace Sirikata
