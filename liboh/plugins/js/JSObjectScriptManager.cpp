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
#include "JSObjects/JSSystem.hpp"

#include "JSObjects/JSUtilObj.hpp"
#include "JSObjects/JSHandler.hpp"
#include "JSObjects/JSTimer.hpp"
#include "JSSerializer.hpp"
#include "JSPattern.hpp"

#include "JS_JSMessage.pbj.hpp"
#include "JSObjects/JSWhen.hpp"
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
        import_paths = new OptionValue("import-paths","../../liboh/plugins/js/scripts,../share/js/scripts",OptionValueType<std::list<String> >(),"Comma separated list of paths to import files from, searched in order for the requested import."),
        mDefaultScript = new OptionValue("default-script", "std/default.em", OptionValueType<String>(), "Default script to execute when an initial script is not specified."),
        NULL
    );

    mOptions = OptionSet::getOptions("jsobjectscriptmanager",this);
    mOptions->parse(arguments);

    createTemplates(); //these templates involve vec, quat, pattern, etc.
}

String JSObjectScriptManager::defaultScript() const {
    return mDefaultScript->as<String>();
}

/*
  EMERSON!: util
 */

void JSObjectScriptManager::createUtilTemplate()
{
    v8::HandleScope handle_scope;
    mUtilTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // An internal field holds the JSObjectScript*
    mUtilTemplate->SetInternalFieldCount(UTIL_TEMPLATE_FIELD_COUNT);


    mUtilTemplate->Set(v8::String::New("create_when"),v8::FunctionTemplate::New(JSUtilObj::ScriptCreateWhen));
    mUtilTemplate->Set(JS_STRING(sqrt),v8::FunctionTemplate::New(JSUtilObj::ScriptSqrtFunction));
    mUtilTemplate->Set(JS_STRING(acos),v8::FunctionTemplate::New(JSUtilObj::ScriptAcosFunction));
    mUtilTemplate->Set(JS_STRING(asin),v8::FunctionTemplate::New(JSUtilObj::ScriptAsinFunction));
    mUtilTemplate->Set(JS_STRING(cos),v8::FunctionTemplate::New(JSUtilObj::ScriptCosFunction));
    mUtilTemplate->Set(JS_STRING(sin),v8::FunctionTemplate::New(JSUtilObj::ScriptSinFunction));
    mUtilTemplate->Set(JS_STRING(rand),v8::FunctionTemplate::New(JSUtilObj::ScriptRandFunction));
    mUtilTemplate->Set(JS_STRING(pow),v8::FunctionTemplate::New(JSUtilObj::ScriptPowFunction));
    mUtilTemplate->Set(JS_STRING(exp),v8::FunctionTemplate::New(JSUtilObj::ScriptExpFunction));
    mUtilTemplate->Set(JS_STRING(abs),v8::FunctionTemplate::New(JSUtilObj::ScriptAbsFunction));
    mUtilTemplate->Set(v8::String::New("create_quoted"), v8::FunctionTemplate::New(JSUtilObj::ScriptCreateQuotedObject));
    mUtilTemplate->Set(v8::String::New("create_when_watched_item"), v8::FunctionTemplate::New(JSUtilObj::ScriptCreateWhenWatchedItem));
    mUtilTemplate->Set(v8::String::New("create_when_watched_list"), v8::FunctionTemplate::New(JSUtilObj::ScriptCreateWhenWatchedList));

    mUtilTemplate->Set(v8::String::New("create_when_timeout_lt"),v8::FunctionTemplate::New(JSUtilObj::ScriptCreateWhenTimeoutLT));

    mUtilTemplate->Set(v8::String::New("plus"), v8::FunctionTemplate::New(JSUtilObj::ScriptPlus));
    mUtilTemplate->Set(v8::String::New("minus"), v8::FunctionTemplate::New(JSUtilObj::ScriptMinus));
    
    mUtilTemplate->Set(v8::String::New("When"),mWhenTemplate);
    mUtilTemplate->Set(v8::String::New("Pattern"), mPatternTemplate);
    mUtilTemplate->Set(v8::String::New("Quaternion"), mQuaternionTemplate);
    mUtilTemplate->Set(v8::String::New("Vec3"), mVec3Template);
}


void JSObjectScriptManager::createQuotedTemplate()
{
    v8::HandleScope handle_scope;
    mQuotedTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    mQuotedTemplate->SetInternalFieldCount(QUOTED_TEMPLATE_FIELD_COUNT);
}

void JSObjectScriptManager::createWhenWatchedItemTemplate()
{
    v8::HandleScope handle_scope;
    mWhenWatchedItemTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    mWhenWatchedItemTemplate->SetInternalFieldCount(WHEN_WATCHED_ITEM_TEMPLATE_FIELD_COUNT);
}


void JSObjectScriptManager::createWhenWatchedListTemplate()
{
    v8::HandleScope handle_scope;
    mWhenWatchedListTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    mWhenWatchedListTemplate->SetInternalFieldCount(WHEN_WATCHED_LIST_TEMPLATE_FIELD_COUNT);
}



void JSObjectScriptManager::createWhenTemplate()
{
    v8::HandleScope handle_scope;
    mWhenTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // An internal field holds the JSObjectScript*
    mWhenTemplate->SetInternalFieldCount(WHEN_TEMPLATE_FIELD_COUNT);

    mWhenTemplate->Set(v8::String::New("suspend"),v8::FunctionTemplate::New(JSWhen::WhenSuspend));
    mWhenTemplate->Set(v8::String::New("resume"),v8::FunctionTemplate::New(JSWhen::WhenResume));
    mWhenTemplate->Set(v8::String::New("getWhenLastPredState"),v8::FunctionTemplate::New(JSWhen::WhenGetLastPredState));
}


/*
  EMERSON: additional types
*/


//these templates involve vec, quat, pattern, etc.
void JSObjectScriptManager::createTemplates()
{
    v8::HandleScope handle_scope;
    mVec3Template        = v8::Persistent<v8::FunctionTemplate>::New(CreateVec3Template());
    mQuaternionTemplate  = v8::Persistent<v8::FunctionTemplate>::New(CreateQuaternionTemplate());
    mPatternTemplate     = v8::Persistent<v8::FunctionTemplate>::New(CreatePatternTemplate());


    createWhenTemplate();
    createQuotedTemplate();

    createUtilTemplate();


    createWhenWatchedItemTemplate();
    createWhenWatchedListTemplate();


    createHandlerTemplate();
    createVisibleTemplate();

    createTimerTemplate();
    createJSInvokableObjectTemplate();
    createPresenceTemplate();
    createSystemTemplate();
    createContextTemplate();
    createContextGlobalTemplate();
}



/*
  EMERSON!: timer
 */
void JSObjectScriptManager::createTimerTemplate()
{
    v8::HandleScope handle_scope;
    mTimerTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    mTimerTemplate->SetInternalFieldCount(TIMER_JSTIMER_TEMPLATE_FIELD_COUNT);

    mTimerTemplate->Set(v8::String::New("resetTimer"),v8::FunctionTemplate::New(JSTimer::resetTimer));
    mTimerTemplate->Set(v8::String::New("clear"),v8::FunctionTemplate::New(JSTimer::clear));
    mTimerTemplate->Set(v8::String::New("suspend"),v8::FunctionTemplate::New(JSTimer::suspend));
    mTimerTemplate->Set(v8::String::New("reset"),v8::FunctionTemplate::New(JSTimer::resume));
    mTimerTemplate->Set(v8::String::New("isSuspended"),v8::FunctionTemplate::New(JSTimer::isSuspended));

}


/*
  EMERSON!: system
 */
void JSObjectScriptManager::createSystemTemplate()
{
    v8::HandleScope handle_scope;
    mSystemTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    mSystemTemplate->SetInternalFieldCount(SYSTEM_TEMPLATE_FIELD_COUNT);


    mSystemTemplate->Set(v8::String::New("sendHome"),v8::FunctionTemplate::New(JSSystem::root_sendHome));
    mSystemTemplate->Set(v8::String::New("registerHandler"),v8::FunctionTemplate::New(JSSystem::root_registerHandler));
    mSystemTemplate->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(JSSystem::root_timeout));
    mSystemTemplate->Set(v8::String::New("print"), v8::FunctionTemplate::New(JSSystem::root_print));

    mSystemTemplate->Set(v8::String::New("import"), v8::FunctionTemplate::New(JSSystem::root_import));



    //check what permissions fake root is loaded with
    mSystemTemplate->Set(v8::String::New("canSendMessage"), v8::FunctionTemplate::New(JSSystem::root_canSendMessage));
    mSystemTemplate->Set(v8::String::New("canRecvMessage"), v8::FunctionTemplate::New(JSSystem::root_canRecvMessage));
    mSystemTemplate->Set(v8::String::New("canProx"), v8::FunctionTemplate::New(JSSystem::root_canProx));
    mSystemTemplate->Set(v8::String::New("canImport"),v8::FunctionTemplate::New(JSSystem::root_canImport));

    mSystemTemplate->Set(v8::String::New("canCreatePresence"), v8::FunctionTemplate::New(JSSystem::root_canCreatePres));
    mSystemTemplate->Set(v8::String::New("canCreateEntity"), v8::FunctionTemplate::New(JSSystem::root_canCreateEnt));
    mSystemTemplate->Set(v8::String::New("canEval"), v8::FunctionTemplate::New(JSSystem::root_canEval));


    
    mSystemTemplate->Set(v8::String::New("getPosition"), v8::FunctionTemplate::New(JSSystem::root_getPosition));
    mSystemTemplate->Set(v8::String::New("getVersion"),v8::FunctionTemplate::New(JSSystem::root_getVersion));

    //this doesn't work now.
    mSystemTemplate->Set(v8::String::New("eval"), v8::FunctionTemplate::New(JSSystem::root_scriptEval));
    mSystemTemplate->Set(v8::String::New("create_context"),v8::FunctionTemplate::New(JSSystem::root_createContext));
    mSystemTemplate->Set(v8::String::New("create_presence"), v8::FunctionTemplate::New(JSSystem::root_createPresence));
    mSystemTemplate->Set(v8::String::New("create_entity"), v8::FunctionTemplate::New(JSSystem::root_createEntity));

    mSystemTemplate->Set(v8::String::New("onPresenceConnected"),v8::FunctionTemplate::New(JSSystem::root_onPresenceConnected));
    mSystemTemplate->Set(v8::String::New("onPresenceDisconnected"),v8::FunctionTemplate::New(JSSystem::root_onPresenceDisconnected));


    mSystemTemplate->Set(JS_STRING(__presence_constructor__), mPresenceTemplate);
    mSystemTemplate->Set(v8::String::New("require"), v8::FunctionTemplate::New(JSSystem::root_require));
    mSystemTemplate->Set(v8::String::New("reset"),v8::FunctionTemplate::New(JSSystem::root_reset));
    mSystemTemplate->Set(v8::String::New("set_script"),v8::FunctionTemplate::New(JSSystem::root_setScript));

}

/*
  EMERSON!: context
 */
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
    mContextTemplate->Set(v8::String::New("suspend"), v8::FunctionTemplate::New(JSContext::ScriptSuspend));
    mContextTemplate->Set(v8::String::New("resume"), v8::FunctionTemplate::New(JSContext::ScriptResume));
    mContextTemplate->Set(v8::String::New("clear"), v8::FunctionTemplate::New(JSContext::ScriptClear));

}


void JSObjectScriptManager::createContextGlobalTemplate()
{
    v8::HandleScope handle_scope;
    // And we expose some functionality directly
    mContextGlobalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    mContextGlobalTemplate->Set(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME),mSystemTemplate);
    mContextGlobalTemplate->Set(v8::String::New(JSSystemNames::UTIL_OBJECT_NAME), mUtilTemplate);
}







/*
  EMERSON!: invokable
 */
void JSObjectScriptManager::createJSInvokableObjectTemplate()
{
  v8::HandleScope handle_scope;

  mInvokableObjectTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
  mInvokableObjectTemplate->SetInternalFieldCount(JSSIMOBJECT_TEMPLATE_FIELD_COUNT);
  mInvokableObjectTemplate->Set(v8::String::New("invoke"), v8::FunctionTemplate::New(JSInvokableObject::invoke));
}


/*
  EMERSON!: visible
 */
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
    mVisibleTemplate->Set(v8::String::New("getVelocity"),v8::FunctionTemplate::New(JSVisible::getVelocity));
    mVisibleTemplate->Set(v8::String::New("getOrientation"),v8::FunctionTemplate::New(JSVisible::getOrientation));
    mVisibleTemplate->Set(v8::String::New("getOrientationVel"),v8::FunctionTemplate::New(JSVisible::getOrientationVel));
    mVisibleTemplate->Set(v8::String::New("getScale"),v8::FunctionTemplate::New(JSVisible::getScale));

    mVisibleTemplate->Set(v8::String::New("getStillVisible"),v8::FunctionTemplate::New(JSVisible::getStillVisible));
    mVisibleTemplate->Set(v8::String::New("checkEqual"),v8::FunctionTemplate::New(JSVisible::checkEqual));
    mVisibleTemplate->Set(v8::String::New("dist"),v8::FunctionTemplate::New(JSVisible::dist));
}


/*
  EMERSON!: presence
 */
void JSObjectScriptManager::createPresenceTemplate()
{
  v8::HandleScope handle_scope;

  mPresenceTemplate = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
  //mPresenceTemplate->SetInternalFieldCount(PRESENCE_FIELD_COUNT);

  v8::Local<v8::Template> proto_t = mPresenceTemplate->PrototypeTemplate();

  //These are not just accessors because we need to ensure that we can deal with
  //their failure conditions.  (Have callbacks).

   //v8::Local<v8::Template> proto_t = mPresenceTemplate->PrototypeTemplate();

  proto_t->Set(v8::String::New("toString"), v8::FunctionTemplate::New(JSPresence::toString));

  //meshes
  proto_t->Set(v8::String::New("getMesh"),v8::FunctionTemplate::New(JSPresence::getMesh));
  proto_t->Set(v8::String::New("setMesh"),v8::FunctionTemplate::New(JSPresence::setMesh));

  //positions
  proto_t->Set(v8::String::New("getPosition"),v8::FunctionTemplate::New(JSPresence::getPosition));
  proto_t->Set(v8::String::New("setPosition"),v8::FunctionTemplate::New(JSPresence::setPosition));

  //velocities
  proto_t->Set(v8::String::New("getVelocity"),v8::FunctionTemplate::New(JSPresence::getVelocity));
  proto_t->Set(v8::String::New("setVelocity"),v8::FunctionTemplate::New(JSPresence::setVelocity));

  //orientations
  proto_t->Set(v8::String::New("setOrientation"),v8::FunctionTemplate::New(JSPresence::setOrientation));
  proto_t->Set(v8::String::New("getOrientation"),v8::FunctionTemplate::New(JSPresence::getOrientation));

  //orientation velocities
  proto_t->Set(v8::String::New("setOrientationVel"),v8::FunctionTemplate::New(JSPresence::setOrientationVel));
  proto_t->Set(v8::String::New("getOrientationVel"),v8::FunctionTemplate::New(JSPresence::getOrientationVel));

  //scale
  proto_t->Set(v8::String::New("setScale"),v8::FunctionTemplate::New(JSPresence::setScale));
  proto_t->Set(v8::String::New("getScale"),v8::FunctionTemplate::New(JSPresence::getScale));

  //callback on prox addition and removal
  proto_t->Set(v8::String::New("__hidden_onProxAdded"),v8::FunctionTemplate::New(JSPresence::ScriptOnProxAddedEvent));
  proto_t->Set(v8::String::New("__hidden_onProxRemoved"),v8::FunctionTemplate::New(JSPresence::ScriptOnProxRemovedEvent));
  // proto_t->Set(v8::String::New("onProxAdded"),v8::FunctionTemplate::New(JSPresence::ScriptOnProxAddedEvent));
  // proto_t->Set(v8::String::New("onProxRemoved"),v8::FunctionTemplate::New(JSPresence::ScriptOnProxRemovedEvent));

  

  //using JSVisible sendmessage so that don't have to re-write a bunch of code.
  proto_t->Set(v8::String::New("sendMessage"),v8::FunctionTemplate::New(JSVisible::__visibleSendMessage));

  // Query angle
  proto_t->Set(v8::String::New("setQueryAngle"),v8::FunctionTemplate::New(JSPresence::setQueryAngle));

  //set up graphics
  proto_t->Set(v8::String::New("_runSimulation"),v8::FunctionTemplate::New(JSPresence::runSimulation));

  //convert this presence object into a visible object
  proto_t->Set(v8::String::New("toVisible"),v8::FunctionTemplate::New(JSPresence::toVisible));

  proto_t->Set(v8::String::New("suspend"), v8::FunctionTemplate::New(JSPresence::pres_suspend));
  proto_t->Set(v8::String::New("resume"), v8::FunctionTemplate::New(JSPresence::pres_resume));



  // For instance templates
  v8::Local<v8::ObjectTemplate> instance_t = mPresenceTemplate->InstanceTemplate();
  instance_t->SetInternalFieldCount(PRESENCE_FIELD_COUNT);

}



//a handler is returned whenever you register a handler in system.
//should be able to print data of handler (pattern + cb),
//should be able to cancel handler    -----> canceling handler kills this
//object.  remove this pattern from being checked for.
//should be able to renew handler     -----> Re-register handler.
/*
  EMERSON!: handler
 */
void JSObjectScriptManager::createHandlerTemplate()
{
    v8::HandleScope handle_scope;
    mHandlerTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // one field is the JSObjectScript associated with it
    // the other field is a pointer to the associated JSEventHandler.
    mHandlerTemplate->SetInternalFieldCount(JSHANDLER_FIELD_COUNT);
    mHandlerTemplate->Set(v8::String::New("printContents"), v8::FunctionTemplate::New(JSHandler::_printContents));
    mHandlerTemplate->Set(v8::String::New("suspend"),v8::FunctionTemplate::New(JSHandler::_suspend));
    mHandlerTemplate->Set(v8::String::New("isSuspended"),v8::FunctionTemplate::New(JSHandler::_isSuspended));
    mHandlerTemplate->Set(v8::String::New("resume"),v8::FunctionTemplate::New(JSHandler::_resume));
    mHandlerTemplate->Set(v8::String::New("clear"),v8::FunctionTemplate::New(JSHandler::_clear));
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
