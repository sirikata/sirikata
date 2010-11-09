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

#include <v8.h>

#include "JSPattern.hpp"
#include "JSEventHandler.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSPresenceStruct.hpp"


namespace Sirikata {
namespace JS {

class JSObjectScript : public ObjectScript {
public:
    //JSObjectScript(HostedObjectPtr ho, const ObjectScriptManager::Arguments&
    //args, v8::Persistent<v8::ObjectTemplate>& global_template,
    //v8::Persistent<v8::ObjectTemplate>& oref_template);
    JSObjectScript(HostedObjectPtr ho, const ObjectScriptManager::Arguments& args, JSObjectScriptManager* jMan);
    virtual ~JSObjectScript();

    void processMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);
    virtual void updateAddressable();


    virtual void attachScript(const String&);

    /** Returns true if this script is valid, i.e. if it was successfully loaded
     *  and initialized.
     */
    bool valid() const;

    /** Dummy callback for testing exposing new functionality to scripts. */
    void test() const;
    void bftm_testSendMessageBroadcast(const std::string& msgToBCast) const;
    void bftm_debugPrintString(std::string cStrMsgBody) const;
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
    void setVisualScaleFunction(const SpaceObjectReference* sporef, v8::Local<v8::Value>& newvis);

    v8::Handle<v8::Value> getOrientationVelFunction(const SpaceObjectReference* sporef);
    void setOrientationVelFunction(const SpaceObjectReference* sporef, const Quaternion& quat);


    v8::Handle<v8::Value> getAxisOfRotation();
    void setAxisOfRotation(v8::Local<v8::Value>& newval);
    v8::Handle<v8::Value> getAngularSpeed();
    void setAngularSpeed(v8::Local<v8::Value>& newval);

    void runGraphics(const SpaceObjectReference& sporef, const String& simname);


    /** Register an event pattern matcher and handler. */
    //void registerHandler(const PatternList& pattern, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb);
    JSEventHandler* registerHandler(const PatternList& pattern, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb,v8::Persistent<v8::Object>& sender);
    v8::Handle<v8::Object> makeEventHandlerObject(JSEventHandler* evHand);

    void deleteHandler(JSEventHandler* toDelete);

    // Presence version of the access handlers
    v8::Handle<v8::Value> getPosition(SpaceID&);
    void setPosition(SpaceID&, v8::Local<v8::Value>& newval);

    v8::Handle<v8::Value> getVelocity(SpaceID&);
    void setVelocity(SpaceID&, v8::Local<v8::Value>& newval);

private:
    typedef std::vector<SpaceObjectReference*> AddressableList;
    AddressableList mAddressableList;

    typedef std::vector<JSEventHandler*> JSEventHandlerList;
    JSEventHandlerList mEventHandlers;


    void handleTimeout(v8::Persistent<v8::Object> target, v8::Persistent<v8::Function> cb);

    void handleScriptingMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);
    void handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);
    void getAllMessageable(AddressableList&allAvailableObjectReferences) const;
    v8::Handle<v8::Value> protectedEval(const String& script_str);



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
    void printAllHandlerLocations();
    void populatePresences(Handle<Object>& system_obj );
    void populateSystemObject(Handle<Object>& system_obj );
    void populateMath(Handle<Object>& system_obj);


    void initializePresences(Handle<Object>& system_obj);
    void clearAllPresences(Handle<Object>& system_obj);

    ODP::Port* mScriptingPort;
    ODP::Port* mMessagingPort;
    ODP::Port* mCreateEntityPort;


    JSObjectScriptManager* mManager;

    typedef std::vector<JSPresenceStruct*> PresenceList;
    PresenceList mPresenceList;

    JSPresenceStruct* mPres;

};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
