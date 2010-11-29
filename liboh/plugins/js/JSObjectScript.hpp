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

#ifndef _SIRIKATA_JS_OBJECT_SCRIPT_HPP_
#define _SIRIKATA_JS_OBJECT_SCRIPT_HPP_



#include <string>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/proxyobject/SessionEventListener.hpp>

#include <boost/filesystem.hpp>

#include <v8.h>

#include "JSPattern.hpp"
#include "JSEventHandler.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSPresenceStruct.hpp"
#include <sirikata/proxyobject/ProxyCreationListener.hpp>


namespace Sirikata {
namespace JS {


class JSObjectScript : public ObjectScript,
                       public SessionEventListener,
                       public ProxyCreationListener
{

public:
    JSObjectScript(HostedObjectPtr ho, const String& args, JSObjectScriptManager* jMan);
    virtual ~JSObjectScript();

    // SessionEventListener Interface
    virtual void onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name);
    virtual void onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name);


    void processMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);
    virtual void updateAddressable();

    virtual void onCreateProxy(ProxyObjectPtr p);
    virtual void onDestroyProxy(ProxyObjectPtr p);
    
    /** Returns true if this script is valid, i.e. if it was successfully loaded
     *  and initialized.
     */
    bool valid() const;

    /** Dummy callback for testing exposing new functionality to scripts. */
    void test() const;
    void testSendMessageBroadcast(const std::string& msgToBCast) const;
    void debugPrintString(std::string cStrMsgBody) const;
    void sendMessageToEntity(SpaceObjectReference* reffer, const std::string& msgBody) const;
    void sendMessageToEntity(int numIndex, const std::string& msgBody) const;
    int  getAddressableSize();


    /** Set a timeout with a callback. */
    void timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb);


    /** Import a file, executing its contents in the root object's scope. */
    v8::Handle<v8::Value> import(const String& filename);

    /** reboot the state of the script, basically reset the state */
    void reboot();

    /** create a new entity at the run time */
    void create_entity(Vector3d& vec, String& script_name, String& mesh_name);

    /** create a new presence of this entity */
    //void create_presence(const SpaceID&);
    void create_presence(const SpaceID& new_space,std::string new_mesh);
    void create_presence(const SpaceID& new_space);


    v8::Handle<v8::Value> getVisualFunction(const SpaceObjectReference* sporef);
    void  setVisualFunction(const SpaceObjectReference* sporef, const std::string& newMeshString);

    void setPositionFunction(const SpaceObjectReference* sporef, const Vector3f& posVec);
    v8::Handle<v8::Value> getPositionFunction(const SpaceObjectReference* sporef);

    void setVelocityFunction(const SpaceObjectReference* sporef, const Vector3f& velVec);
    v8::Handle<v8::Value> getVelocityFunction(const SpaceObjectReference* sporef);

    void  setOrientationFunction(const SpaceObjectReference* sporef, const Quaternion& quat);
    v8::Handle<v8::Value> getOrientationFunction(const SpaceObjectReference* sporef);

    v8::Handle<v8::Value> getVisualScaleFunction(const SpaceObjectReference* sporef);
    void setVisualScaleFunction(const SpaceObjectReference* sporef, float newScale);

    v8::Handle<v8::Value> getOrientationVelFunction(const SpaceObjectReference* sporef);
    void setOrientationVelFunction(const SpaceObjectReference* sporef, const Quaternion& quat);

    void setQueryAngleFunction(const SpaceObjectReference* sporef, const SolidAngle& sa);

    void runSimulation(const SpaceObjectReference& sporef, const String& simname);


    /** Register an event pattern matcher and handler. */
    JSEventHandler* registerHandler(const PatternList& pattern, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb,v8::Persistent<v8::Object>& sender);
    v8::Handle<v8::Object> makeEventHandlerObject(JSEventHandler* evHand);

    void deleteHandler(JSEventHandler* toDelete);


    void registerOnPresenceConnectedHandler(v8::Persistent<v8::Function>& cb) {
        mOnPresenceConnectedHandler = cb;
    }
    void registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function>& cb) {
        mOnPresenceDisconnectedHandler = cb;
    }


    // Presence version of the access handlers
    v8::Handle<v8::Value> getPosition(SpaceID&);
    void setPosition(SpaceID&, v8::Local<v8::Value>& newval);

    v8::Handle<v8::Value> getVelocity(SpaceID&);
    void setVelocity(SpaceID&, v8::Local<v8::Value>& newval);

private:
    // EvalContext tracks the current state w.r.t. eval-related statements which
    // may change in response to user actions (changing directory) or due to the
    // way the system defines actions (e.g. import searches the current script's
    // directory before trying other import paths).
    struct EvalContext {
        boost::filesystem::path currentScriptDir;
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

    typedef std::vector<SpaceObjectReference*> AddressableList;
    AddressableList mAddressableList;

    typedef std::vector<JSEventHandler*> JSEventHandlerList;
    JSEventHandlerList mEventHandlers;


    // Handlers for presence connection events
    v8::Persistent<v8::Function> mOnPresenceConnectedHandler;
    v8::Persistent<v8::Function> mOnPresenceDisconnectedHandler;

    void handleTimeout(v8::Persistent<v8::Object> target, v8::Persistent<v8::Function> cb);

    void handleScriptingMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);
    void handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);
    void getAllMessageable(AddressableList&allAvailableObjectReferences) const;
    v8::Handle<v8::Value> protectedEval(const String& script_str, const EvalContext& new_ctx);



//    v8::Local<v8::Object> getMessageSender(const RoutableMessageHeader& msgHeader);
    v8::Local<v8::Object> getMessageSender(const ODP::Endpoint& src);

    void flushQueuedHandlerEvents();
    bool mHandlingEvent;
    JSEventHandlerList mQueuedHandlerEventsAdd;
    JSEventHandlerList mQueuedHandlerEventsDelete;
    void removeHandler(JSEventHandler* toRemove);


    HostedObjectPtr mParent;
    v8::Persistent<v8::Context> mContext;


    Handle<Object> getSystemObject();
    Handle<Object> getGlobalObject();

    void populateAddressable(Handle<Object>& system_obj );
    void createMessageableArray();
    void printAllHandlerLocations();
    void initializePresences(Handle<Object>& system_obj);
    void populateSystemObject(Handle<Object>& system_obj );
    void populateMath(Handle<Object>& system_obj);

    // Adds/removes presences from the javascript's system.presences array.
    v8::Handle<v8::Object> addPresence(const SpaceObjectReference& sporef);
    void removePresence(const SpaceObjectReference& sporef);

    ODP::Port* mScriptingPort;
    ODP::Port* mMessagingPort;
    ODP::Port* mCreateEntityPort;


    JSObjectScriptManager* mManager;


    typedef std::map<SpaceObjectReference, JSPresenceStruct*> PresenceMap;
    PresenceMap mPresences;

};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
