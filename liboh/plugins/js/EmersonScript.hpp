/*  Sirikata
 *  JSObjectScript.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SIRIKATA_EMERSON_OBJECT_SCRIPT_HPP__
#define __SIRIKATA_EMERSON_OBJECT_SCRIPT_HPP__



#include <string>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/proxyobject/SessionEventListener.hpp>

#include <boost/filesystem.hpp>

#include <v8.h>


#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include "JSObjects/JSInvokableObject.hpp"
#include "JSEntityCreateInfo.hpp"
#include "JSObjectScript.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSVisibleManager.hpp"
#include "JS_JSMessage.pbj.hpp"
#include "EmersonMessagingManager.hpp"
#include "EmersonHttpManager.hpp"
#include <sirikata/core/util/SerializationCheck.hpp>
#include "JSCtx.hpp"

namespace Sirikata {
namespace JS {


class EmersonScript : public JSObjectScript,
                      public SessionEventListener,
                      public EmersonMessagingManager
{

public:
    EmersonScript(HostedObjectPtr ho, const String& args,
        const String& script, JSObjectScriptManager* jMan,
        JSCtx* ctx);


    virtual ~EmersonScript();

    // Sirikata::Service Interface
    virtual void start();
    virtual void stop();

    // SessionEventListener Interface
    //called from main strand posts to object strand.
    virtual void onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name,HostedObject::PresenceToken token);
    virtual void onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name);

    //called by JSPresenceStruct.  requests the parent HostedObject disconnect
    //the presence associated with jspres
    void requestDisconnect(JSPresenceStruct* jspres);

    Time getHostedTime();


    virtual void  notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier);
    virtual void  notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier);


    /*
      Payload should be able to be parsed into JS::Proctocol::JSMessage.  If it
      can be, and deserialization is successful, processes the scripting
      message. (via deserializeMsgAndDispatch.)
      Return has no meaning.
     */
    virtual bool handleScriptCommRead(
        const SpaceObjectReference& src, const SpaceObjectReference& dst, const std::string& payload);


    /**
       Callback for unreliable messages.
       Payload should be able to be parsed into JS::Proctocol::JSMessage.  If it
       can be, and deserialization is successful, processes the scripting
       message. (via deserializeMsgAndDispatch.)
     */
    void handleScriptCommUnreliable (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);

    boost::any invokeInvokable(std::vector<boost::any>& params,v8::Persistent<v8::Function> function_);


    // Post this function to an IOService to add an event to be handled. Must
    // take liveness token because while waiting to be processed the object may,
    // e.g., be killed.
    void invokeCallbackInContext(Liveness::Token alive, v8::Persistent<v8::Function> cb, JSContextStruct* jscontext);


    //function gets called when presences are connected.  functToCall is the
    //function that gets called back.  Does so in context associated with
    //jscont.  Binds first arg to presence object associated with jspres.
    //mostly used for contexts and presences to execute their callbacks on
    //connection and disconnection events.
    void handlePresCallback( v8::Handle<v8::Function> funcToCall,JSContextStruct* jscont, JSPresenceStruct* jspres);

    v8::Handle<v8::Value> restorePresence(PresStructRestoreParams& psrp,JSContextStruct* jsctx);


    /**
       @param {sporef} localPresSporef The space object reference for the
       local presence for which a visible has either moved into its prox set or
       out of its prox set.

       @param {JSVisibleStruct*} jsvis The visible struct associated with the
       presence that moved into or out of local presence's prox set.

       @param{bool} isGone True if trying to fire a proximity removal event,
       false if trying to fire a proximity added event.

       Enters context associated with jscontextStruct, and fires either its
       onProxAdded or onProxRemoved callback function.
     */
    void fireProxEvent(const SpaceObjectReference& localPresSporef,
        JSVisibleStruct* jsvis, JSContextStruct* jscont, bool isGone);




    /** Returns true if this script is valid, i.e. if it was successfully loaded
     *  and initialized.
     */
    bool valid() const;

    /**
       This function runs through the entire list of presences associated with
       this JSObjectScript, and then frees those that are not part of that vector.
     */
    void killOtherPresences(JSPresVec& jspresVec);


    v8::Handle<v8::Value> killEntity(JSContextStruct* jscont);

    /**
       Sends a message over the odp port from local presence that has sporef
       from to some other presence in world with sporef receiver.

       Gets port to send over as value of mMessagingPortMap associated with key
       from.
     */
    void sendMessageToEntityUnreliable(const SpaceObjectReference& receiver, const SpaceObjectReference& from, const std::string& msgBody);


    //takes the c++ object jspres, creates a new visible object out of it, if we
    //don't already have a c++ visible object associated with it (if we do, use
    //that one), wraps that c++ object in v8, and returns it as a v8 object to
    //user
    v8::Local<v8::Object> presToVis(JSPresenceStruct* jspres, JSContextStruct* jscont);

    // Create event handler
    v8::Handle<v8::Value> create_event(v8::Persistent<v8::Function>& cb, JSContextStruct* jscont);

    /** Set a timeout with a callback. */
    v8::Handle<v8::Value> create_timeout(double period, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont);

    v8::Handle<v8::Value> create_timeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared,JSContextStruct* jscont);

    void registerFixupSuspendable(JSSuspendable* jssuspendable, uint32 contID);


    /** create a new entity at the run time */
    void create_entity(EntityCreateInfo& eci);


    //JSContextStructs request the JSObjectScript to call finishClear on them
    //when doing so won't invalidate any iterators on the JSObjectScript.
    virtual void registerContextForClear(JSContextStruct* jscont);


    /**
       Creates a JSVisibleStruct and wraps it in a persistent v8 object that is
       returned.

       @param {SpaceObjectReference} visibleObj Will make a JSVisibleStruct out
       of this spaceobjectreference.

       @param {JSVisibleDataPtr} addParams If don't have a proxy object in the world
       with sporef visibleObj, will try to fill in JSVisibleStruct data with
       these values (note: if NULL), fills in default values.
     */
    v8::Local<v8::Object> createVisibleWeakPersistent(
        const SpaceObjectReference& presID, const SpaceObjectReference& visibleObj, JSVisibleDataPtr addParams);
    v8::Local<v8::Object> createVisibleWeakPersistent(
        const SpaceObjectReference& visibleObj, JSVisibleDataPtr addParams);

    //handling basic datatypes for JSPresences
    void setLocation(const SpaceObjectReference sporef, const TimedMotionVector3f& loc);
    void setOrientation(const SpaceObjectReference sporef, const TimedMotionQuaternion& orient);
    void setBounds(const SpaceObjectReference sporef, const BoundingSphere3f& bounds);
    void setVisual(const SpaceObjectReference sporef, const std::string& newMeshString);

    String getQuery(const SpaceObjectReference& sporef) const;
    void setQueryFunction(const SpaceObjectReference sporef, const String& query);

    void setPhysicsFunction(const SpaceObjectReference sporef, const String& newPhysicsString);

    /****Methods that return V8 wrappers for c++ objects **/


    v8::Handle<v8::Value> findVisible(const SpaceObjectReference& proximateObj);


    //wraps the c++ presence structure in a v8 object.
    v8::Local<v8::Object> wrapPresence(JSPresenceStruct* presToWrap, v8::Persistent<v8::Context>* ctxToWrapIn);

    //If a simulation for presence with sporef, sporef, and name simname already
    //exist, just return an object corresponding to those.
    Sirikata::JS::JSInvokableObject::JSInvokableObjectInt* runSimulation(const SpaceObjectReference& sporef, const String& simname);

    /**
       @param {JSContextStruct} jscont: just checks to ensure that jscont is
       root sandbox before calling reset. (Otherwise, throws error.)
       @param proxSetVis : When we reset, we want to be able to replay all the
       visibles in an entity's prox result set.  The key of this map is the
       visible whose prox set is contained in the value of the map (ie in the
       vector).

     */
    v8::Handle<v8::Value> requestReset(JSContextStruct* jscont, const std::map <SpaceObjectReference,std::vector <SpaceObjectReference> > & proxSetVis);


    /**
       Timers and context suspendables are killed and deleted in contexts.
       However, presences are also stored in EmersonScript,
       and EmersonScript must be told to kill them and remove them.

       And JSPresences, we need to remove from our list of presences and ask
       hosted object to remove it.
    */
    void deletePres(JSPresenceStruct* toDelete);


    void resetPresence(JSPresenceStruct* jspresStruct);


    JSObjectScriptManager* manager() const { return mManager; }


    JSContextStruct* rootContext() const { return mContext; }


    JSVisibleManager jsVisMan;

    HostedObjectPtr mParent;

/**
       Contexts are allowed to initiate http requests.  To do so, they should
       get a copy of each EmersonScript's httpPtr and make requests directly
       from there.  Similarly, in contexts' destructors, they should also notify
       emHttpPtr that the context was destroyed.
     */
    EmersonHttpPtr getEmersonHttpPtr()
    {
        return emHttpPtr;
    }


    /**
       msgToSend contains a serialized v8 object that will be sent from sandbox
       with id sendingSandbox to sandbox with id receivingSandbox.  Posts dow
       into processSandboxMessage, which actually dispatches the message in the
       correct context.

       Returns undefined.
     */
    v8::Handle<v8::Value> sendSandbox(const String& msgToSend, uint32 senderID, uint32 receiverID);


    virtual void postCallbackChecks();
private:

    void postDestroy(Liveness::Token alive);

    // Helper for disconnections
    void unsubscribePresenceEvents(const SpaceObjectReference& name);
    // Helper for *clearing* presences (not disconnections). When the presence
    // struct is destroyed (i.e. gc, shutdown), we can then clear out references
    // to the presence's data.
    void removePresenceData(const SpaceObjectReference& sporefToDelete);


    //wraps internal c++ jsvisiblestruct in a v8 object
    v8::Local<v8::Object> createVisibleWeakPersistent(JSVisibleStruct* jsvis);

    //Called internally by script when guaranteed to be outside of handler
    //execution loop.
    void killScript();
    void resetScript();

    /*
      Deserializes the object contained in js_msg, checks if have any pattern
      handlers that match it, and dispatch their callbacks if there are.

      If have any handlers that match pattern of message, dispatch their associated
      callbacks.
     */
    bool handleScriptCommRead(
        const SpaceObjectReference& src, const SpaceObjectReference& dst,
        Sirikata::JS::Protocol::JSMessage js_msg);


    bool mHandlingEvent;
    bool mResetting;
    bool mKilling;


    //This function returns to you the current value of present token and
    //incrmenets presenceToken so that get a unique one each time.  If
    //presenceToken is equal to default_presence_token, increments one beyond it
    //so that don't start inadvertently returning the DEFAULT_PRESENCE_TOKEN;
    HostedObject::PresenceToken incrementPresenceToken();

    void  printStackFrame(std::stringstream&, v8::Local<v8::StackFrame>);

    // Adds/removes presences from the javascript's system.presences array.
    //returns the jspresstruct associated with new object
    JSPresenceStruct* addConnectedPresence(const SpaceObjectReference& sporef,HostedObject::PresenceToken token);

    /**
       Tries to decode string as a v8 object.  Lookup senderID and receiverID in
       mContStructMap to find associated JSContextStructs.  If they don't exist,
       silently drop.  If they do, then check if sender has a callback for
       sandbox messages defined on it.  If it does, then call it with the
       deserialized message object in it.
     */
    void processSandboxMessage(
        String msgToSend, uint32 senderID, uint32 receiverID,
        Liveness::Token alive);


    /**
       Removes jscont from context map and tells it to finish its clear methods.
     */
    void finishContextClear(JSContextStruct* jscont);


    //looks through all previously connected presneces (located in mPresences).
    //returns the corresponding jspresencestruct that has a spaceobjectreference
    //that matches sporef.
    JSPresenceStruct* findPresence(const SpaceObjectReference& sporef);


    typedef std::map<SpaceObjectReference, ODP::Port*> MessagingPortMap;
    MessagingPortMap mMessagingPortMap;


    void callbackUnconnected(ProxyObjectPtr proxy, HostedObject::PresenceToken token);
    HostedObject::PresenceToken presenceToken;

    typedef std::map<SpaceObjectReference, JSPresenceStruct*> PresenceMap;
    typedef PresenceMap::iterator PresenceMapIter;
    PresenceMap mPresences;

    /**
       When we are resetting, we want to replay all the visibles that presences
       had received in their prox result set.  This map temporarily holds a
       list of these visibles (fills in requestReset, empties in resetScript).
       Index of map corresponds to presence whose prox set value is associated
       with.

       Note: visibleManager class takes care of deleting memory allocated for
       JSVisibleStructs.
     */
    std::map<SpaceObjectReference,std::vector<JSVisibleStruct*> >resettingVisiblesResultSet;

    typedef std::vector<JSPresenceStruct*> PresenceVec;
    PresenceVec mUnconnectedPresences;

    //we do not want to invalidate message receiving iterator, so keep separate
    //tabs of all the context structs that we were supposed to delete when we
    //received a message.
    std::vector<JSContextStruct*> contextsToClear;

    EmersonHttpPtr emHttpPtr;


    // Called within mStrand.
    void iOnDisconnected(
        SessionEventProviderPtr from, const SpaceObjectReference& name,
        Liveness::Token alive);
    void eCreateEntityFinish(ObjectHost* oh,EntityCreateInfo& eci);

    //When we initiate stop of object from killEntity, we do not actually
    //want to call letDie in iStop.  This is because we've already locked
    //liveness of Script.  If letDie is false, then, we don't call letDie until
    //we get to destructor.
    void iStop(Liveness::Token alive, bool letDie);

    void iHandleScriptCommRead(
        const SpaceObjectReference& src, const SpaceObjectReference& dst,
        const String& payload,Liveness::Token alive);

    void iHandleScriptCommUnreliable(
        const ODP::Endpoint& src, const ODP::Endpoint& dst,
        MemoryReference payload,Liveness::Token alive);

    void mainStrandCompletePresConnect(Location newLoc,BoundingSphere3f bs,
        PresStructRestoreParams psrp,HostedObject::PresenceToken presToke,
        Liveness::Token alive);

    void iNotifyProximateGone(
        ProxyObjectPtr proximateObject, const SpaceObjectReference& querier,
        Liveness::Token alive);

    void iResetProximateHelper(
        JSVisibleStruct* proxVis, const SpaceObjectReference& proxTo);

    void  iNotifyProximate(
        ProxyObjectPtr proximateObject, const SpaceObjectReference& querier,
        Liveness::Token alive);

    void iOnConnected(SessionEventProviderPtr from,
        const SpaceObjectReference& name, HostedObject::PresenceToken token,
        bool duringInit,Liveness::Token alive);

    void iInvokeInvokable(
        std::vector<boost::any>& params,v8::Persistent<v8::Function> function_,
        Liveness::Token alive);


    //simname, sporef
    typedef std::vector< std::pair<String,SpaceObjectReference> > SimVec;
    SimVec mSimulations;

};

#define EMERSCRIPT_SERIAL_CHECK()\
    Sirikata::SerializationCheck::Scoped sc (JSObjectScript::mCtx->serializationCheck());


} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
