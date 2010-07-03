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

namespace Sirikata {
namespace JS {

class JSObjectScript : public ObjectScript {
public:
    //JSObjectScript(HostedObjectPtr ho, const ObjectScriptManager::Arguments&
    //args, v8::Persistent<v8::ObjectTemplate>& global_template,
    //v8::Persistent<v8::ObjectTemplate>& oref_template);
    JSObjectScript(HostedObjectPtr ho, const ObjectScriptManager::Arguments& args, JSObjectScriptManager* jMan);
    ~JSObjectScript();

    bool forwardMessagesTo(MessageService*);
    bool endForwardingMessagesTo(MessageService*);
    bool processRPC(const RoutableMessageHeader &receivedHeader, const std::string &name, MemoryReference args, MemoryBuffer &returnValue);
    void processMessage(const RoutableMessageHeader& header, MemoryReference body);

	void updateAddressable();

    /** Returns true if this script is valid, i.e. if it was successfully loaded
     *  and initialized.
     */
    bool valid() const;

    /** Dummy callback for testing exposing new functionality to scripts. */
    void test() const;
    void bftm_testSendMessageBroadcast(const std::string& msgToBCast) const;
    void bftm_debugPrintString(std::string cStrMsgBody) const;
    void sendMessageToEntity(ObjectReference* reffer, const std::string& msgBody) const;
    void sendMessageToEntity(int numIndex, const std::string& msgBody) const;
    int  getAddressableSize();
    

    /** Set a timeout with a callback. */
    void timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb);

    /** Import a file, executing its contents in the root object's scope. */
    v8::Handle<v8::Value> import(const String& filename);
    
	/** reboot the state of the script, basically reset the state */
	void reboot();

	/** create a new entity at the run time */
	void create_entity(Vector3d&);

    v8::Handle<v8::String> getVisual();
    void setVisual(v8::Local<v8::Value>& newvis);

    v8::Handle<v8::Value> getVisualScale();
    void setVisualScale(v8::Local<v8::Value>& newvis);

    v8::Handle<v8::Value> getPosition();
    void setPosition(v8::Local<v8::Value>& newval);
    v8::Handle<v8::Value> getVelocity();
    void setVelocity(v8::Local<v8::Value>& newval);
    v8::Handle<v8::Value> getOrientation();
    void setOrientation(v8::Local<v8::Value>& newval);
    v8::Handle<v8::Value> getAxisOfRotation();
    void setAxisOfRotation(v8::Local<v8::Value>& newval);
    v8::Handle<v8::Value> getAngularSpeed();
    void setAngularSpeed(v8::Local<v8::Value>& newval);


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

    typedef std::vector<JSEventHandler*> JSEventHandlerList;
    JSEventHandlerList mEventHandlers;

    
    void handleTimeout(v8::Persistent<v8::Object> target, v8::Persistent<v8::Function> cb);
    
    void handleScriptingMessage(const RoutableMessageHeader& hdr, MemoryReference payload);
    void bftm_handleCommunicationMessage(const RoutableMessageHeader& hdr, MemoryReference payload);
    void bftm_handleCommunicationMessage_old(const RoutableMessageHeader& hdr, MemoryReference payload);
    void bftm_getAllMessageable(std::vector<ObjectReference*>&allAvailableObjectReferences) const;
    v8::Handle<v8::Value> protectedEval(const String& script_str);


    v8::Local<v8::Object> getMessageSender(const RoutableMessageHeader& msgHeader);

    void flushQueuedHandlerEvents();
    bool mHandlingEvent;
    JSEventHandlerList mQueuedHandlerEventsAdd;
    JSEventHandlerList mQueuedHandlerEventsDelete;
    void removeHandler(JSEventHandler* toRemove);

    
    HostedObjectPtr mParent;
    v8::Persistent<v8::Context> mContext;

    typedef std::vector<ObjectReference*> AddressableList;
    AddressableList mAddressableList;


    Handle<Object> getSystemObject();
    Handle<Object> getGlobalObject();

    void populateAddressable(Handle<Object>& system_obj );
    void printAllHandlerLocations();
    void populatePresences(Handle<Object>& system_obj );
    void populateSystemObject(Handle<Object>& system_obj );
    

    ODP::Port* mScriptingPort;
    ODP::Port* mMessagingPort;

    JSObjectScriptManager* mManager;

};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
