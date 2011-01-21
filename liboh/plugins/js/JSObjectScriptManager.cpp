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
#include "JSObjects/JSVisible.hpp"
#include "JSObjects/JSFakeroot.hpp"
#include "JSObjects/JSSystem.hpp"
#include "JSObjects/JSMath.hpp"
#include "JSObjects/JSHandler.hpp"

#include "JSSerializer.hpp"
#include "JSPattern.hpp"

#include "JS_JSMessage.pbj.hpp"

#include "JSObjects/Addressable.hpp"
#include "JSObjects/JSPresence.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/JSInvokableObject.hpp"
#include "JSSystemNames.hpp"
#include "JSObjects/JSContext.hpp"



namespace Sirikata {
namespace JS {



ObjectScriptManager* JSObjectScriptManager::createObjectScriptManager(const Sirikata::String& arguments) {
    return new JSObjectScriptManager(arguments);
}



JSObjectScriptManager::JSObjectScriptManager(const Sirikata::String& arguments)
{
    OptionValue* import_paths;
    InitializeClassOptions(
        "jsobjectscriptmanager",this,
        // Default value allows us to use std libs in the build tree, starting
        // from build/cmake
        import_paths = new OptionValue("import-paths","../../liboh/plugins/js/scripts",OptionValueType<std::list<String> >(),"Comma separated list of paths to import files from, searched in order for the requested import."),
        NULL
    );

    mOptions = OptionSet::getOptions("jsobjectscriptmanager",this);
    mOptions->parse(arguments);

    createTemplates(); //these templates involve vec, quat, pattern, etc.
}



void JSObjectScriptManager::createMathTemplate()
{
    v8::HandleScope handle_scope;
    mMathTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // An internal field holds the JSObjectScript*
    mMathTemplate->SetInternalFieldCount(MATH_TEMPLATE_FIELD_COUNT);

    mMathTemplate->Set(JS_STRING(sqrt),v8::FunctionTemplate::New(JSMath::ScriptSqrtFunction));
    mMathTemplate->Set(JS_STRING(acos),v8::FunctionTemplate::New(JSMath::ScriptAcosFunction));
    mMathTemplate->Set(JS_STRING(asin),v8::FunctionTemplate::New(JSMath::ScriptAsinFunction));
    mMathTemplate->Set(JS_STRING(cos),v8::FunctionTemplate::New(JSMath::ScriptCosFunction));
    mMathTemplate->Set(JS_STRING(sin),v8::FunctionTemplate::New(JSMath::ScriptSinFunction));
    mMathTemplate->Set(JS_STRING(rand),v8::FunctionTemplate::New(JSMath::ScriptRandFunction));
    mMathTemplate->Set(JS_STRING(pow),v8::FunctionTemplate::New(JSMath::ScriptPowFunction));
    mMathTemplate->Set(JS_STRING(abs),v8::FunctionTemplate::New(JSMath::ScriptAbsFunction));
    
}

//these templates involve vec, quat, pattern, etc.
void JSObjectScriptManager::createTemplates()
{
    v8::HandleScope handle_scope;
    mVec3Template = v8::Persistent<v8::FunctionTemplate>::New(CreateVec3Template());
    mQuaternionTemplate = v8::Persistent<v8::FunctionTemplate>::New(CreateQuaternionTemplate());
    mPatternTemplate = v8::Persistent<v8::FunctionTemplate>::New(CreatePatternTemplate());

    createMathTemplate();
    createFakerootTemplate();
    createContextTemplate();
    
    createHandlerTemplate();
    createVisibleTemplate();    

    
    createJSInvokableObjectTemplate();
    createPresenceTemplate();    
    createSystemTemplate();

    //createTriggerableTemplate();
}



void JSObjectScriptManager::createFakerootTemplate()
{
    v8::HandleScope handle_scope;
    mFakerootTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    mFakerootTemplate->SetInternalFieldCount(FAKEROOT_TEMPLATE_FIELD_COUNT);
    
    
    mFakerootTemplate->Set(v8::String::New("sendHome"),v8::FunctionTemplate::New(JSFakeroot::root_sendHome));
    mFakerootTemplate->Set(v8::String::New("registerHandler"),v8::FunctionTemplate::New(JSFakeroot::root_registerHandler));
    mFakerootTemplate->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(JSFakeroot::root_timeout));
    mFakerootTemplate->Set(v8::String::New("print"), v8::FunctionTemplate::New(JSFakeroot::root_print));

        
    //check what permissions fake root is loaded with
    mFakerootTemplate->Set(v8::String::New("canSendMessage"), v8::FunctionTemplate::New(JSFakeroot::root_canSendMessage));
    mFakerootTemplate->Set(v8::String::New("canRecvMessage"), v8::FunctionTemplate::New(JSFakeroot::root_canRecvMessage));
    mFakerootTemplate->Set(v8::String::New("canProx"), v8::FunctionTemplate::New(JSFakeroot::root_canProx));

    mFakerootTemplate->Set(v8::String::New("toString"), v8::FunctionTemplate::New(JSFakeroot::root_toString));
    mFakerootTemplate->Set(v8::String::New("getPosition"), v8::FunctionTemplate::New(JSFakeroot::root_getPosition));
    
    //add basic templates: vec3, quat, math
    addBaseTemplates(mFakerootTemplate);
}



//no reboot.
//no create_entity
//no import
//no create_presence
//no update_addressable
//no motion (for now).  May special-case motion stuff
void JSObjectScriptManager::createContextTemplate()
{
    v8::HandleScope handle_scope;
    // And we expose some functionality directly
    mContextTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // An internal field holds the JSObjectScript*
    mContextTemplate->SetInternalFieldCount(CONTEXT_TEMPLATE_FIELD_COUNT);

    // Functions / types
    //suspend,kill,resume,execute
    mContextTemplate->Set(v8::String::New("execute"), v8::FunctionTemplate::New(JSContext::ScriptExecute));

}

//takes in a template (likely either the context template or the system template)
void JSObjectScriptManager::addBaseTemplates(v8::Persistent<v8::ObjectTemplate> tempToAddTo)
{
    tempToAddTo->Set(JS_STRING(Pattern), mPatternTemplate);
    tempToAddTo->Set(v8::String::New("Quaternion"), mQuaternionTemplate);
    tempToAddTo->Set(v8::String::New("Vec3"), mVec3Template);
    tempToAddTo->Set(v8::String::New("Math"),mMathTemplate);
}

//should be the same as the previous function.
void JSObjectScriptManager::addBaseTemplates(v8::Handle<v8::ObjectTemplate>  tempToAddTo)
{
    tempToAddTo->Set(JS_STRING(Pattern), mPatternTemplate);
    tempToAddTo->Set(v8::String::New("Quaternion"), mQuaternionTemplate);
    tempToAddTo->Set(v8::String::New("Vec3"), mVec3Template);
    tempToAddTo->Set(v8::String::New("Math"),mMathTemplate);
}


//it looks like I can't figure out how to inherit system template functionality
//from object template.
void JSObjectScriptManager::createSystemTemplate()
{
    v8::HandleScope handle_scope;
    mGlobalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    // An internal field holds the JSObjectScript*
    mGlobalTemplate->SetInternalFieldCount(1);

    // And we expose some functionality directly
    v8::Handle<v8::ObjectTemplate> system_templ = v8::ObjectTemplate::New();
    // An internal field holds the JSObjectScript*
    system_templ->SetInternalFieldCount(SYSTEM_TEMPLATE_FIELD_COUNT);

    // Functions / types
    system_templ->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(JSSystem::ScriptTimeout));
    system_templ->Set(v8::String::New("print"), v8::FunctionTemplate::New(JSSystem::Print));
    system_templ->Set(v8::String::New("import"), v8::FunctionTemplate::New(JSSystem::ScriptImport));
    system_templ->Set(v8::String::New("reboot"),v8::FunctionTemplate::New(JSSystem::ScriptReboot));
    system_templ->Set(v8::String::New("create_entity"), v8::FunctionTemplate::New(JSSystem::ScriptCreateEntity));
    system_templ->Set(v8::String::New("create_presence"), v8::FunctionTemplate::New(JSSystem::ScriptCreatePresence));


    //when creating a context, should also optionally take in a callback for what should happen if presence associated gets disconnected from space;
    system_templ->Set(v8::String::New("create_context"),v8::FunctionTemplate::New(JSSystem::ScriptCreateContext));

    system_templ->Set(v8::String::New("onPresenceConnected"),v8::FunctionTemplate::New(JSSystem::ScriptOnPresenceConnected));
    system_templ->Set(v8::String::New("onPresenceDisconnected"),v8::FunctionTemplate::New(JSSystem::ScriptOnPresenceDisconnected));
    system_templ->Set(JS_STRING(registerHandler),v8::FunctionTemplate::New(JSSystem::ScriptRegisterHandler));
    //system_templ->Set(v8::String::New("registerUniqueMessageCode"),New(JSSystem::registerUniqueMessageCode));
    
    //math, vec, quaternion, etc.
    addBaseTemplates(system_templ);
    //add the system template to the global template
    mGlobalTemplate->Set(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME), system_templ);
}




void JSObjectScriptManager::createJSInvokableObjectTemplate()
{
  v8::HandleScope handle_scope;

  mInvokableObjectTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
  mInvokableObjectTemplate->SetInternalFieldCount(JSSIMOBJECT_TEMPLATE_FIELD_COUNT);
  mInvokableObjectTemplate->Set(v8::String::New("invoke"), v8::FunctionTemplate::New(JSInvokableObject::invoke)); 
}



void JSObjectScriptManager::createVisibleTemplate()
{
    v8::HandleScope handle_scope;
    mVisibleTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    // An internal field holds the external address of the visible object
    mVisibleTemplate->SetInternalFieldCount(VISIBLE_FIELD_COUNT);

    
    //these function calls are defined in JSObjects/JSVisible.hpp
    mVisibleTemplate->Set(v8::String::New("__debugRef"),v8::FunctionTemplate::New(JSVisible::__debugRef));
    mVisibleTemplate->Set(v8::String::New("sendMessage"),v8::FunctionTemplate::New(JSVisible::__visibleSendMessage));
    mVisibleTemplate->Set(v8::String::New("toString"),v8::FunctionTemplate::New(JSVisible::toString));
    mVisibleTemplate->Set(v8::String::New("getPosition"),v8::FunctionTemplate::New(JSVisible::getPosition));
    mVisibleTemplate->Set(v8::String::New("getStillVisible"),v8::FunctionTemplate::New(JSVisible::getStillVisible));
    mVisibleTemplate->Set(v8::String::New("checkEqual"),v8::FunctionTemplate::New(JSVisible::checkEqual));
}

void JSObjectScriptManager::createPresenceTemplate()
{
  v8::HandleScope handle_scope;

  mPresenceTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
  mPresenceTemplate->SetInternalFieldCount(PRESENCE_FIELD_COUNT);


  //These are not just accessors because we need to ensure that we can deal with
  //their failure conditions.  (Have callbacks).

  mPresenceTemplate->Set(v8::String::New("toString"), v8::FunctionTemplate::New(JSPresence::toString));

  //meshes
  mPresenceTemplate->Set(v8::String::New("getMesh"),v8::FunctionTemplate::New(JSPresence::getMesh));
  mPresenceTemplate->Set(v8::String::New("setMesh"),v8::FunctionTemplate::New(JSPresence::setMesh));

  //positions
  mPresenceTemplate->Set(v8::String::New("getPosition"),v8::FunctionTemplate::New(JSPresence::getPosition));
  mPresenceTemplate->Set(v8::String::New("setPosition"),v8::FunctionTemplate::New(JSPresence::setPosition));

  //velocities
  mPresenceTemplate->Set(v8::String::New("getVelocity"),v8::FunctionTemplate::New(JSPresence::getVelocity));
  mPresenceTemplate->Set(v8::String::New("setVelocity"),v8::FunctionTemplate::New(JSPresence::setVelocity));

  //orientations
  mPresenceTemplate->Set(v8::String::New("setOrientation"),v8::FunctionTemplate::New(JSPresence::setOrientation));
  mPresenceTemplate->Set(v8::String::New("getOrientation"),v8::FunctionTemplate::New(JSPresence::getOrientation));

  //orientation velocities
  mPresenceTemplate->Set(v8::String::New("setOrientationVel"),v8::FunctionTemplate::New(JSPresence::setOrientationVel));
  mPresenceTemplate->Set(v8::String::New("getOrientationVel"),v8::FunctionTemplate::New(JSPresence::getOrientationVel));

  //scale
  mPresenceTemplate->Set(v8::String::New("setScale"),v8::FunctionTemplate::New(JSPresence::setScale));
  mPresenceTemplate->Set(v8::String::New("getScale"),v8::FunctionTemplate::New(JSPresence::getScale));

  //callback on prox addition and removal
  mPresenceTemplate->Set(v8::String::New("onProxAdded"),v8::FunctionTemplate::New(JSPresence::ScriptOnProxAddedEvent));
  mPresenceTemplate->Set(v8::String::New("onProxRemoved"),v8::FunctionTemplate::New(JSPresence::ScriptOnProxRemovedEvent));
  
    
  // Query angle
  mPresenceTemplate->Set(v8::String::New("setQueryAngle"),v8::FunctionTemplate::New(JSPresence::setQueryAngle));

  //set up graphics
  mPresenceTemplate->Set(v8::String::New("runSimulation"),v8::FunctionTemplate::New(JSPresence::runSimulation));

  //send broadcast message
  mPresenceTemplate->Set(v8::String::New("broadcastVisible"), v8::FunctionTemplate::New(JSPresence::broadcastVisible));

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
    mHandlerTemplate->SetInternalFieldCount(JSHANDLER_FIELD_COUNT);
    mHandlerTemplate->Set(v8::String::New("printContents"), v8::FunctionTemplate::New(JSHandler::__printContents));
    mHandlerTemplate->Set(v8::String::New("suspend"),v8::FunctionTemplate::New(JSHandler::__suspend));
    mHandlerTemplate->Set(v8::String::New("isSuspended"),v8::FunctionTemplate::New(JSHandler::__isSuspended));
    mHandlerTemplate->Set(v8::String::New("resume"),v8::FunctionTemplate::New(JSHandler::__resume));
    mHandlerTemplate->Set(v8::String::New("clear"),v8::FunctionTemplate::New(JSHandler::__clear));
}


JSObjectScriptManager::~JSObjectScriptManager()
{
}

ObjectScript* JSObjectScriptManager::createObjectScript(HostedObjectPtr ho, const String& args)
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
