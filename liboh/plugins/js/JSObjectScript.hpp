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

#ifndef __SIRIKATA_JS_OBJECT_SCRIPT_HPP__
#define __SIRIKATA_JS_OBJECT_SCRIPT_HPP__



#include <string>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/proxyobject/SessionEventListener.hpp>

#include <boost/filesystem.hpp>

#include <v8.h>

#include "JSPattern.hpp"
#include "JSObjectStructs/JSEventHandlerStruct.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include "JSObjects/JSInvokableObject.hpp"
#include "JSObjectStructs/JSWhenWatchedItemStruct.hpp"
#include "JSObjectStructs/JSWhenStruct.hpp"
#include "JSVisibleStructMonitor.hpp"


namespace Sirikata {
namespace JS {

struct EntityCreateInfo
{
    String scriptType;
    String scriptOpts;
    SpaceID spaceID;
    Location loc;
    float  scale;
    String mesh;
    SolidAngle solid_angle;
};



class JSObjectScript : public ObjectScript,
                       public SessionEventListener,
                       public JSVisibleStructMonitor
{

public:

    static JSObjectScript* decodeSystemObject(v8::Handle<v8::Value> toDecode, String& errorMessage);

    
    JSObjectScript(HostedObjectPtr ho, const String& args, JSObjectScriptManager* jMan);
    virtual ~JSObjectScript();

    // SessionEventListener Interface
    virtual void onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name,int token);
    virtual void onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name);

    Time getHostedTime();
    void processMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);

    virtual void  notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier);
    virtual void  notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier);

    v8::Handle<v8::Value> handleTimeoutContext(v8::Handle<v8::Object> target, v8::Persistent<v8::Function> cb,JSContextStruct* jscontext);
    v8::Handle<v8::Value> handleTimeoutContext(v8::Persistent<v8::Function> cb,v8::Handle<v8::Context>* jscontext);
    
    v8::Handle<v8::Value> executeInContext(v8::Persistent<v8::Context> &contExecIn, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv);

    //this function returns a context with
    v8::Handle<v8::Value> createContext(JSPresenceStruct* presAssociatedWith,SpaceObjectReference* canMessage,bool sendEveryone, bool recvEveryone, bool proxQueries);


    String tokenizeWhenPred(const String& whenPredAsString);
    void addWhen(JSWhenStruct* whenToAdd);
    void removeWhen(JSWhenStruct* whenToRemove);
    
    
    /** Returns true if this script is valid, i.e. if it was successfully loaded
     *  and initialized.
     */
    bool valid() const;

    String createNewValueInContext(v8::Handle<v8::Value> val, v8::Handle<v8::Context> ctx);
    
    /** Dummy callback for testing exposing new functionality to scripts. */
    void debugPrintString(std::string cStrMsgBody) const;
    void sendMessageToEntity(SpaceObjectReference* reffer, SpaceObjectReference* from, const std::string& msgBody) const;


    /** Print the given string to the current output. */
    void print(const String& str);
    
    /** Set a timeout with a callback. */
    v8::Handle<v8::Value> create_timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont);

    v8::Handle<v8::Value> createWhenWatchedItem(v8::Handle<v8::Array> itemArray);
    v8::Handle<v8::Value> createWhenWatchedItem(JSWhenWatchedItemStruct* wwis);
    v8::Handle<v8::Value> createWhenWatchedList(std::vector<JSWhenWatchedItemStruct*> wwisVec);

    
    /** Import a file, executing its contents in the root object's scope. */
    v8::Handle<v8::Value> import(const String& filename);

    /** reboot the state of the script, basically reset the state */
    void reboot();

    Handle<v8::Context> context() { return mContext;}
    /** create a new entity at the run time */
    void create_entity(EntityCreateInfo& eci);


    
    //handling basic datatypes for JSPresences
    v8::Handle<v8::Value> getVisualFunction(const SpaceObjectReference* sporef);
    v8::Handle<v8::Value> getVisualScaleFunction(const SpaceObjectReference* sporef);
    void setVisualFunction(const SpaceObjectReference* sporef, const std::string& newMeshString);
    void setPositionFunction(const SpaceObjectReference* sporef, const Vector3f& posVec);
    void setVelocityFunction(const SpaceObjectReference* sporef, const Vector3f& velVec);
    void setOrientationFunction(const SpaceObjectReference* sporef, const Quaternion& quat);
    void setVisualScaleFunction(const SpaceObjectReference* sporef, float newScale);
    void setOrientationVelFunction(const SpaceObjectReference* sporef, const Quaternion& quat);
    void setQueryAngleFunction(const SpaceObjectReference* sporef, const SolidAngle& sa);


    /****Methods that return V8 wrappers for c++ objects **/
    //attempts to make a new jsvisiblestruct if don't already have one in
    //jsvismonitor matching visibleObj+visibleTo.  Wraps the c++ jsvisiblestruct
    //in v8 object.
    v8::Local<v8::Object> createVisibleObject(const SpaceObjectReference& visibleObj,const SpaceObjectReference& visibleTo,bool isVisible, v8::Handle<v8::Context> ctx);

    /** create a new presence of this entity */
    v8::Handle<v8::Value> create_presence(const String& newMesh, v8::Handle<v8::Function> callback );
    v8::Handle<v8::Value> createWhen(v8::Handle<v8::Array>predArray, v8::Handle<v8::Function> callback, JSContextStruct* associatedContext);
    v8::Handle<v8::Value> createQuoted(const String& toQuote);

    
    
    Sirikata::JS::JSInvokableObject::JSInvokableObjectInt* runSimulation(const SpaceObjectReference& sporef, const String& simname);



    //registering position listeners to receive updates from loc
    bool registerPosListener(SpaceObjectReference* sporef, SpaceObjectReference* ownPres,PositionListener* pl,TimedMotionVector3f* loc, TimedMotionQuaternion* orient);
    bool deRegisterPosListener(SpaceObjectReference* sporef, SpaceObjectReference* ownPres,PositionListener* pl);
    

    /** Register an event pattern matcher and handler. */
    JSEventHandlerStruct* registerHandler(const PatternList& pattern, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb,v8::Persistent<v8::Object>& sender);
    v8::Handle<v8::Object> makeEventHandlerObject(JSEventHandlerStruct* evHand);

    void deleteHandler(JSEventHandlerStruct* toDelete);

    void registerOnPresenceConnectedHandler(v8::Persistent<v8::Function>& cb) {
        mOnPresenceConnectedHandler = cb;
    }
    void registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function>& cb) {
        mOnPresenceDisconnectedHandler = cb;
    }


    //visible struct management: for auto-notifying visible structs for when
    //they become visible and when they stop being visible (ie enter and leave
    //query to pinto).
    void deRegisterVisibleStruct(JSVisibleStruct* jsvis);


    
    JSObjectScriptManager* manager() const { return mManager; }
    
    v8::Handle<v8::Value> internalEval(v8::Persistent<v8::Context>ctx,const String& em_script_str);
    v8::Handle<v8::Function> functionValue(const String& em_script_str);
private:
    // EvalContext tracks the current state w.r.t. eval-related statements which
    // may change in response to user actions (changing directory) or due to the
    // way the system defines actions (e.g. import searches the current script's
    // directory before trying other import paths).
    struct EvalContext {
        EvalContext();
        EvalContext(const EvalContext& rhs);

        boost::filesystem::path currentScriptDir;
        std::ostream* currentOutputStream;
    };
    // This is a helper which adds an EvalContext to the stack and ensures that
    // when it goes out of scope it is removed. This will almost always be the
    // right way to add and remove an EvalContext from the stack, ensure
    // multiple exit paths from a method don't cause the stack to become
    // incorrect.
    struct ScopedEvalContext {
        ScopedEvalContext(JSObjectScript* _parent, const EvalContext& _ctx);
        ~ScopedEvalContext();

        JSObjectScript* parent;
    };
    friend class ScopedEvalContext;

    std::stack<EvalContext> mEvalContextStack;


    //wraps internal c++ jsvisiblestruct in a v8 object
    v8::Local<v8::Object> createVisibleObject(JSVisibleStruct* jsvis, v8::Handle<v8::Context> ctxToCreateIn);
    
    
    void checkWhens();


    
    typedef std::vector<JSEventHandlerStruct*> JSEventHandlerList;
    JSEventHandlerList mEventHandlers;


    // Handlers for presence connection events
    v8::Persistent<v8::Function> mOnPresenceConnectedHandler;
    v8::Persistent<v8::Function> mOnPresenceDisconnectedHandler;



    void handleScriptingMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);
    void handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);
    v8::Handle<v8::Value> protectedEval(const String& script_str, const EvalContext& new_ctx);



    v8::Handle<v8::Value> ProtectedJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> executeJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Function> funcInCtx,int argc, v8::Handle<v8::Object>*target, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> compileFunctionInContext(v8::Persistent<v8::Context>ctx, v8::Handle<v8::Function>&cb);



    v8::Handle<v8::Object> getMessageSender(const ODP::Endpoint& src, const ODP::Endpoint& dst);

    void addSelfField(const SpaceObjectReference& myName);
    
    
    void flushQueuedHandlerEvents();
    bool mHandlingEvent;
    JSEventHandlerList mQueuedHandlerEventsAdd;
    JSEventHandlerList mQueuedHandlerEventsDelete;
    void removeHandler(JSEventHandlerStruct* toRemove);


    HostedObjectPtr mParent;
    v8::Persistent<v8::Context> mContext;



    Handle<Object> getSystemObject();
    Handle<Object> getGlobalObject();
    void printAllHandlerLocations();
    void initializePresences(Handle<Object>& system_obj);
    void populateSystemObject(Handle<Object>& system_obj );



    void  printStackFrame(std::stringstream&, v8::Local<v8::StackFrame>);
    // Adds/removes presences from the javascript's system.presences array.
    v8::Handle<v8::Object> addConnectedPresence(const SpaceObjectReference& sporef,int token);
    v8::Handle<v8::Object> addPresence(JSPresenceStruct* presToAdd);
    void removePresence(const SpaceObjectReference& sporef);




    ODP::Port* mScriptingPort;
    ODP::Port* mMessagingPort;
    ODP::Port* mCreateEntityPort;

    JSObjectScriptManager* mManager;

    WhenMap mWhens;

    void callbackUnconnected(const SpaceObjectReference& name, int token);
    int presenceToken;
    uint64 hiddenObjectCount;    

    typedef std::map<SpaceObjectReference, JSPresenceStruct*> PresenceMap;
    typedef PresenceMap::iterator PresenceMapIter;
    PresenceMap mPresences;

    typedef std::vector<JSPresenceStruct*> PresenceVec;
    PresenceVec mUnconnectedPresences;
};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
