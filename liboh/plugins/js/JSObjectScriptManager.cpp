/*  Sirikata
 *  JSObjectScriptManager.cpp
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

#include <sirikata/oh/Platform.hpp>

#include "JSObjectScriptManager.hpp"
#include "JSObjectScript.hpp"

#include "JSObjects/JSVec3.hpp"
#include "JSObjects/JSQuaternion.hpp"
#include "JSObjects/JSSystem.hpp"
#include "JSObjects/JSHandler.hpp"

#include "JSSerializer.hpp"
#include "JSPattern.hpp"

#include "JS_JSMessage.pbj.hpp"

#include "JSObjects/Addressable.hpp"
#include "JSObjects/JSPresence.hpp"

namespace Sirikata {
namespace JS {



ObjectScriptManager* JSObjectScriptManager::createObjectScriptManager(const Sirikata::String& arguments) {
    return new JSObjectScriptManager(arguments);
}



JSObjectScriptManager::JSObjectScriptManager(const Sirikata::String& arguments)
{
    createSystemTemplate();
    createAddressableTemplate();
    createHandlerTemplate();
	createPresenceTemplate();
	
}


void JSObjectScriptManager::createSystemTemplate()
{
    v8::HandleScope handle_scope;
    mGlobalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    // An internal field holds the JSObjectScript*
    mGlobalTemplate->SetInternalFieldCount(1);

    // And we expose some functionality directly
    v8::Handle<v8::ObjectTemplate> system_templ = v8::ObjectTemplate::New();
    // An internal field holds the JSObjectScript*

    system_templ->SetInternalFieldCount(1);

    // Functions / types
    system_templ->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(JSSystem::ScriptTimeout));
    system_templ->Set(v8::String::New("print"), v8::FunctionTemplate::New(JSSystem::Print));
    system_templ->Set(v8::String::New("import"), v8::FunctionTemplate::New(JSSystem::ScriptImport));
    system_templ->Set(v8::String::New("__test"), v8::FunctionTemplate::New(JSSystem::__ScriptGetTest));
    system_templ->Set(v8::String::New("__broadcast"),v8::FunctionTemplate::New(JSSystem::__ScriptTestBroadcastMessage));

    system_templ->Set(v8::String::New("reboot"),v8::FunctionTemplate::New(JSSystem::ScriptReboot));

	system_templ->Set(v8::String::New("create_entity"), v8::FunctionTemplate::New(JSSystem::ScriptCreateEntity));
    


    //these are mutable fields
	
    system_templ->SetAccessor(JS_STRING(visual), JSSystem::ScriptGetVisual, JSSystem::ScriptSetVisual);
    system_templ->SetAccessor(JS_STRING(scale), JSSystem::ScriptGetScale, JSSystem::ScriptSetScale);

    system_templ->SetAccessor(JS_STRING(position), JSSystem::ScriptGetPosition, JSSystem::ScriptSetPosition);
    system_templ->SetAccessor(JS_STRING(velocity), JSSystem::ScriptGetVelocity, JSSystem::ScriptSetVelocity);
    system_templ->SetAccessor(JS_STRING(orientation), JSSystem::ScriptGetOrientation, JSSystem::ScriptSetOrientation);
    system_templ->SetAccessor(JS_STRING(angularAxis), JSSystem::ScriptGetAxisOfRotation, JSSystem::ScriptSetAxisOfRotation);
    system_templ->SetAccessor(JS_STRING(angularVelocity), JSSystem::ScriptGetAngularSpeed, JSSystem::ScriptSetAngularSpeed);


    mVec3Template = v8::Persistent<v8::FunctionTemplate>::New(CreateVec3Template());
    system_templ->Set(v8::String::New("Vec3"), mVec3Template);

    mQuaternionTemplate = v8::Persistent<v8::FunctionTemplate>::New(CreateQuaternionTemplate());
    system_templ->Set(v8::String::New("Quaternion"), mQuaternionTemplate);

    mPatternTemplate = v8::Persistent<v8::FunctionTemplate>::New(CreatePatternTemplate());
    system_templ->Set(JS_STRING(Pattern), mPatternTemplate);

    /**
       FIXME: need to add way to remove a handler.
     **/
    system_templ->Set(JS_STRING(registerHandler),v8::FunctionTemplate::New(JSSystem::ScriptRegisterHandler));
    mGlobalTemplate->Set(v8::String::New("system"), system_templ);
}



//creating the addressable template.  addressable is an array within system that 
void JSObjectScriptManager::createAddressableTemplate()
{
    v8::HandleScope handle_scope;
    mAddressableTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    // An internal field holds the external address of the addressable object
    mAddressableTemplate->SetInternalFieldCount(3);

    //these function calls are defined in JSObjects/Addressable.hpp
    mAddressableTemplate->Set(v8::String::New("__debugRef"),v8::FunctionTemplate::New(JSAddressable::__debugRef));
    mAddressableTemplate->Set(v8::String::New("sendMessage"),v8::FunctionTemplate::New(JSAddressable::__addressableSendMessage));
    mAddressableTemplate->Set(v8::String::New("toString"),v8::FunctionTemplate::New(JSAddressable::toString));
}

void JSObjectScriptManager::createPresenceTemplate()
{
  v8::HandleScope handle_scope;
  
  // Ideally we want the addressable template to be a prototype of presencetemplate
  //All that can be done to presences can be done to the addressble too
  /*
  if(mAddressableTemplate.IsEmpty())
  {
    createAddressableTemplate();  
  }
  */
  
  mPresenceTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New()); 
  
  mPresenceTemplate->SetInternalFieldCount(2);

  // add stuff to the presence template
  // something like setMesh

  mPresenceTemplate->Set(v8::String::New("toString"), v8::FunctionTemplate::New(JSPresence::toString));
  mPresenceTemplate->Set(v8::String::New("setMesh"), v8::FunctionTemplate::New(JSPresence::setMesh));
 

}


//a handler is returned whenever you register a handler in system.
//should be able to print data of handler (pattern + cb),
//should be able to cancel handler    -----> canceling handler kills this
//object.  remove this pattern from being checked for.
//should be able to renew handler     -----> Re-register handler.
void JSObjectScriptManager::createHandlerTemplate()
{
    v8::HandleScope handle_scope;
    mHandlerTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // one field is the JSObjectScript associated with it
    // the other field is a pointer to the associated JSEventHandler.
    mHandlerTemplate->SetInternalFieldCount(2);
    mHandlerTemplate->Set(v8::String::New("printContents"), v8::FunctionTemplate::New(JSHandler::__printContents));
    mHandlerTemplate->Set(v8::String::New("suspend"),v8::FunctionTemplate::New(JSHandler::__suspend));
    mHandlerTemplate->Set(v8::String::New("isSuspended"),v8::FunctionTemplate::New(JSHandler::__isSuspended));
    mHandlerTemplate->Set(v8::String::New("resume"),v8::FunctionTemplate::New(JSHandler::__resume));
    mHandlerTemplate->Set(v8::String::New("clear"),v8::FunctionTemplate::New(JSHandler::__clear));
}

      
JSObjectScriptManager::~JSObjectScriptManager()
{
}

ObjectScript *JSObjectScriptManager::createObjectScript(HostedObjectPtr ho,const Arguments& args)
{
    JSObjectScript* new_script = new JSObjectScript(ho, args, this);
    if (!new_script->valid()) {
        delete new_script;
        return NULL;
    }
    return new_script;
}

void JSObjectScriptManager::destroyObjectScript(ObjectScript*toDestroy){
    delete toDestroy;
}

//DEBUG FUNCTION.
void JSObjectScriptManager::testPrint()
{
    std::cout<<"\n\nTest print for js object script manager\n\n";
}

} // namespace JS
} // namespace JS
