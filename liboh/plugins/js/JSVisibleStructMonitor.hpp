#ifndef __JS_VISIBLE_STRUCT_MONITOR_HPP__
#define __JS_VISIBLE_STRUCT_MONITOR_HPP__

#include "JSObjectStructs/JSVisibleStruct.hpp"
#include <map>
#include <sirikata/core/util/Platform.hpp>


namespace Sirikata{
namespace JS{

class JSObjectScript;
class JSVisibleStruct;

class JSVisibleStructMonitor
{
public:
    
    //constructor requires zero args
    JSVisibleStructMonitor();
    ~JSVisibleStructMonitor();


    //can only create jsvisiblestructs through visiblestruct monitor
    //if the visible struct you want to create already exists (whatsVisible and
    //toWhom match up with existing jsvisiblestructs in listenFromNoneMap or
    //mObjectsToFollow, then returns that visible struct.  Otherwise, creates
    //new one.
    //If there is no associated local presence that his monitoring the presence,
    //then toWhom should be SpaceObjectReference::null();
    //note: when creating a visible struct that is associated with a presence,
    //you should provide the same sporef for both whatsVisible and toWhom.
    JSVisibleStruct* createVisStruct(JSObjectScript* jsobjscript, const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom, bool visibleCurrently);

    
    //Returns null if do not have a visible struct that has sporefVisible and
    //sporefVisibleFrom associated with them.  Otherwise, returns the
    //visiblestruct.  Note, when looking for a visible struct associated with a 
    //presence, you should provide the presence's sporef for both sporefVisible
    //and sporefVisibleFrom
    JSVisibleStruct* checkVisStructExists(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleFrom);

    JSVisibleStruct* checkVisStructExists(const SpaceObjectReference& sporefVisible);

    //when pinto tells me that an object with sporef sporefVisible is now within
    //the query of presence with sporefVisibleTo, this function should get
    //called to pass along the notification.
    void checkNotifyNowVis(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleTo);

    //when pinto tells me that an object with sporef sporefVisible is now *not* within
    //the query of presence with sporefVisibleTo, this function should get
    //called to pass along the notification.
    void checkNotifyNowNotVis(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleTo);

    
    void deRegisterVisibleStruct(JSVisibleStruct* jsvis);

    
    void checkForwardUpdate(const SpaceObjectReference& sporefVisible, const TimedMotionVector3f& tmv, const TimedMotionQuaternion& tmq,const BoundingSphere3f& newBounds);    

    
private:


    void checkNotifyNowNotVis_noVisFrom(const SpaceObjectReference& sporefVisible);

    //helper functions for createVisStruct.
    JSVisibleStruct* createVisStructFromNone(JSObjectScript* jsobjscript, const SpaceObjectReference& whatsVisible);
    JSVisibleStruct* createVisStructFromHaveListeners(JSObjectScript* jsobjscript,const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom, bool visibleCurrently);
    

    //helper functions for deRegisterVisibleStruct function
    void deRegisterVisibleStructFromNoneMap(JSVisibleStruct* jsvis);
    void deRegisterVisibleStructFromObjectsMap(JSVisibleStruct* jsvis);

    //helper functions for checkVisStructExists.  Checks if visibleStruct
    //associated with sporefVisible and sporefVisibleFrom exist already in
    //listenFromNoneMap and mObjectsToFollow, respectively.
    //checks if visible struct watching sporefVisible exists in listenFromNoneMap
    JSVisibleStruct* checkNoListenFrom(const SpaceObjectReference& sporefVisible);
    JSVisibleStruct* checkListenFromVisStructs(const SpaceObjectReference& sporefVisible, const SpaceObjectReference& sporefVisibleFrom);

    //returns a visible struct from mObjectsToFollow that is watching an object
    //with sporef sporefVisible, and that has its stillVisible flag as true.
    //If no such object exists, returns null
    JSVisibleStruct* checkWatchingWithFrom(const SpaceObjectReference& sporefVisible);

    //looks through mObjectsToFollow for a jsvisiblestruct that is watching
    //whatsvisible and is still visible itself.  if find one, request it to
    //perform an update on arg jsvis.
    void updateForFromNone(JSVisibleStruct* jsvis,const SpaceObjectReference& whatsVisible);

    //looks through listenFromNoneMap for a visiblestruct tracking proxy
    //associated with whatsVisible.  If find one, request jsvis to update it and
    //notify it that it's now visible.
    void updateFromNoneMap(JSVisibleStruct* jsvis,const SpaceObjectReference& whatsVisible);

    
    typedef std::map<SpaceObjectReference, JSVisibleStruct*> SpaceToVisMap;
    typedef SpaceToVisMap::iterator SpaceToVisMapIter;

    //this map is indexed on the spaceobject reference of the object that is
    //being watched.  It should be used to track those visible structs that were
    //registered without having a presence actually see them.
    SpaceToVisMap listenFromNoneMap;
    
        
    typedef std::map<SpaceObjectReference, SpaceToVisMap> ListenFromMap;
    typedef ListenFromMap::iterator ListenFromMapIter;

    //let's say A is visible to you through your presence B.
    //then you would say x = mObjectsToFollow[A]
    //and then x[B] to get the visible struct associated with
    //that object in the world.
    ListenFromMap mObjectsToFollow;

};

}//close namespace js
}//close namespace sirikata

#endif
