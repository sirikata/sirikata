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

#include "JSPattern.hpp"
#include "JSObjectStructs/JSEventHandlerStruct.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include "JSObjects/JSInvokableObject.hpp"
#include "JSEntityCreateInfo.hpp"
#include "JSObjectScript.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSVisibleManager.hpp"

namespace Sirikata {
namespace JS {


class EmersonScript : public JSObjectScript,
                      public JSVisibleManager,
                      public SessionEventListener
{

public:
    EmersonScript(HostedObjectPtr ho, const String& args, const String& script, JSObjectScriptManager* jMan);
    virtual ~EmersonScript();

    // SessionEventListener Interface
    virtual void onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name,HostedObject::PresenceToken token);
    virtual void onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name);
    
    //called by JSPresenceStruct.  requests the parent HostedObject disconnect
    //the presence associated with jspres
    void requestDisconnect(JSPresenceStruct* jspres);

    Time getHostedTime();
    void processMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);

    virtual void  notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier);
    virtual void  notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier);

    void handleTimeoutContext(v8::Persistent<v8::Function> cb,JSContextStruct* jscontext);


    //function gets called when presences are connected.  functToCall is the
    //function that gets called back.  Does so in context associated with
    //jscont.  Binds first arg to presence object associated with jspres.
    //mostly used for contexts and presences to execute their callbacks on
    //connection and disconnection events.
    void handlePresCallback( v8::Handle<v8::Function> funcToCall,JSContextStruct* jscont, JSPresenceStruct* jspres);

    v8::Handle<v8::Value> restorePresence(PresStructRestoreParams& psrp,JSContextStruct* jsctx);

    

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

    void sendMessageToEntity(const SpaceObjectReference& reffer, const SpaceObjectReference& from, const std::string& msgBody);


    //takes the c++ object jspres, creates a new visible object out of it, if we
    //don't already have a c++ visible object associated with it (if we do, use
    //that one), wraps that c++ object in v8, and returns it as a v8 object to
    //user
    v8::Persistent<v8::Object> presToVis(JSPresenceStruct* jspres, JSContextStruct* jscont);


    /** Set a timeout with a callback. */
    v8::Handle<v8::Value> create_timeout(double period, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont);

    v8::Handle<v8::Value> create_timeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared,JSContextStruct* jscont);

    void registerFixupSuspendable(JSSuspendable* jssuspendable, uint32 contID);


    /** create a new entity at the run time */
    void create_entity(EntityCreateInfo& eci);



    //handling basic datatypes for JSPresences
    void setVisualFunction(const SpaceObjectReference sporef, const std::string& newMeshString);
    void setPositionFunction(const SpaceObjectReference sporef, const Vector3f& posVec);
    void setVelocityFunction(const SpaceObjectReference sporef, const Vector3f& velVec);
    void setOrientationFunction(const SpaceObjectReference sporef, const Quaternion& quat);
    void setVisualScaleFunction(const SpaceObjectReference sporef, float newScale);
    void setOrientationVelFunction(const SpaceObjectReference sporef, const Quaternion& quat);

    void setQueryAngleFunction(const SpaceObjectReference sporef, const SolidAngle& sa);
    SolidAngle getQueryAngle(const SpaceObjectReference sporef);


    v8::Handle<v8::Value> getPhysicsFunction(const SpaceObjectReference sporef);
    void setPhysicsFunction(const SpaceObjectReference sporef, const String& newPhysicsString);

    /****Methods that return V8 wrappers for c++ objects **/


    /**
       Creates a JSVisibleStruct and wraps it in a persistent v8 object that is
       returned.  
       
       @param {SpaceObjectReference} visibleObj Will make a JSVisibleStruct out
       of this spaceobjectreference.

       @param {JSProxyData} addParams If don't have a proxy object in the world
       with sporef visibleObj, will try to fill in JSVisibleStruct data with
       these values (note: if NULL), fills in default values.
     */
    v8::Persistent<v8::Object> createVisiblePersistent(const SpaceObjectReference& visibleObj,JSProxyData* addParams, v8::Handle<v8::Context> ctx);


    v8::Handle<v8::Value> findVisible(const SpaceObjectReference& proximateObj);


    //wraps the c++ presence structure in a v8 object.
    v8::Local<v8::Object> wrapPresence(JSPresenceStruct* presToWrap, v8::Persistent<v8::Context>* ctxToWrapIn);

    /** create a new presence of this entity */
    v8::Handle<v8::Value> create_presence(const String& newMesh, v8::Handle<v8::Function> callback, JSContextStruct* jsctx, const Vector3d& poser, const SpaceID& spaceToCreateIn);


    Sirikata::JS::JSInvokableObject::JSInvokableObjectInt* runSimulation(const SpaceObjectReference& sporef, const String& simname);

    v8::Handle<v8::Value> requestReset(JSContextStruct* jscont);



    //registering position listeners to receive updates from loc
    bool registerPosAndMeshListener(SpaceObjectReference* sporef, SpaceObjectReference* ownPres,PositionListener* pl,MeshListener*ml, TimedMotionVector3f* loc, TimedMotionQuaternion* orient, BoundingSphere3f* bs, String* mesh, String* phy);
    bool deRegisterPosAndMeshListener(SpaceObjectReference* sporef, SpaceObjectReference* ownPres,PositionListener* pl,MeshListener* ml);


    /** Register an event pattern matcher and handler. */
    void registerHandler(JSEventHandlerStruct* jsehs);
    v8::Handle<v8::Object> makeEventHandlerObject(JSEventHandlerStruct* evHand, JSContextStruct* jscs);


    /**
       Timers and context suspendables are killed and deleted in contexts.
       However, EventHandlers and presences are also stored in EmersonScript,
       and EmersonScript must be told to kill them and remove them.

       All other suspendables can be killed from their context struct.
       JSEventHandlers cannot be because we need to ensure that we're out of the
       event loop before removing it from the list of events and killing it.
       And JSPresences, we need to remove from our list of presences and ask
       hosted object to remove it.
     */    
    void deleteHandler(JSEventHandlerStruct* toDelete);
    void deletePres(JSPresenceStruct* toDelete);

    
    void resetPresence(JSPresenceStruct* jspresStruct);


    JSObjectScriptManager* manager() const { return mManager; }


    JSContextStruct* rootContext() const { return mContext; }

    HostedObjectPtr mParent;

private:

    //wraps internal c++ jsvisiblestruct in a v8 object
    v8::Persistent<v8::Object> createVisiblePersistent(JSVisibleStruct* jsvis, v8::Handle<v8::Context> ctxToCreateIn);

    //Called internally by script when guaranteed to be outside of handler
    //execution loop.
    void killScript();

    typedef std::vector<JSEventHandlerStruct*> JSEventHandlerList;
    JSEventHandlerList mEventHandlers;

    void resetScript();

    void handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);

    v8::Handle<v8::Object> getMessageSender(const ODP::Endpoint& src, const ODP::Endpoint& dst);

    void flushQueuedHandlerEvents();
    bool mHandlingEvent;
    bool mResetting;
    bool mKilling;
    JSEventHandlerList mQueuedHandlerEventsAdd;
    JSEventHandlerList mQueuedHandlerEventsDelete;


    //Does not delete handler.  Removes it from the eventHandlerList.
    void removeHandler(JSEventHandlerStruct* toRemove);


    //This function returns to you the current value of present token and
    //incrmenets presenceToken so that get a unique one each time.  If
    //presenceToken is equal to default_presence_token, increments one beyond it
    //so that don't start inadvertently returning the DEFAULT_PRESENCE_TOKEN;
    HostedObject::PresenceToken incrementPresenceToken();


    void  printAllHandlers();
    void  printStackFrame(std::stringstream&, v8::Local<v8::StackFrame>);

    // Adds/removes presences from the javascript's system.presences array.
    //returns the jspresstruct associated with new object
    JSPresenceStruct* addConnectedPresence(const SpaceObjectReference& sporef,HostedObject::PresenceToken token);


    //looks through all previously connected presneces (located in mPresences).
    //returns the corresponding jspresencestruct that has a spaceobjectreference
    //that matches sporef.
    JSPresenceStruct* findPresence(const SpaceObjectReference& sporef);

    //debugging code to output the sporefs of all the presences that I have in mPresences
    void printMPresences();


    std::map< SpaceObjectReference ,ODP::Port* >mMessagingPortMap;
    ODP::Port* mCreateEntityPort;


    void callbackUnconnected(const SpaceObjectReference& name, HostedObject::PresenceToken token);
    HostedObject::PresenceToken presenceToken;

    typedef std::map<SpaceObjectReference, JSPresenceStruct*> PresenceMap;
    typedef PresenceMap::iterator PresenceMapIter;
    PresenceMap mPresences;

    typedef std::vector<JSPresenceStruct*> PresenceVec;
    PresenceVec mUnconnectedPresences;
};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
