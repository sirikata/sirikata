/*  Sirikata
 *  JSObjectScript.cpp
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

#include <sirikata/core/util/KnownServices.hpp>


#include "JSObjectScript.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSLogging.hpp"

#include "JSSerializer.hpp"
#include "JSEventHandler.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSVec3.hpp"
#include "JSObjects/JSQuaternion.hpp"
#include "JSObjects/JSInvokableObject.hpp"

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/Random.hpp>

#include <sirikata/core/odp/Defs.hpp>
#include <vector>
#include <set>
#include "JSObjects/JSFields.hpp"
#include "JS_JSMessage.pbj.hpp"
#include "emerson/EmersonUtil.h"
#include "JSSystemNames.hpp"

#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSTimerStruct.hpp"
#include "JSObjects/JSObjectsUtils.hpp"



#define FIXME_GET_SPACE_OREF() \
    HostedObject::SpaceObjRefVec spaceobjrefs;              \
    mParent->getSpaceObjRefs(spaceobjrefs);                 \
    assert(spaceobjrefs.size() == 1);                 \
    SpaceID space = (spaceobjrefs.begin())->space(); \
    ObjectReference oref = (spaceobjrefs.begin())->object();


using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {

namespace {



void ProtectedJSCallback(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> target, v8::Handle<v8::Function> cb, int argc, v8::Handle<v8::Value> argv[]) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    Handle<Value> result;
    if (target->IsNull() || target->IsUndefined())
    {
        JSLOG(insane,"ProtectedJSCallback without target given.");
        result = cb->Call(ctx->Global(), argc, argv);
    }
    else
    {
        JSLOG(insane,"ProtectedJSCallback with target given.");
        result = cb->Call(target, argc, argv);
    }


    if (result.IsEmpty())
    {
        // FIXME what should we do with this exception?
        v8::String::Utf8Value error(try_catch.Exception());
        const char* cMsg = ToCString(error);
        JSLOG(error, "Uncaught exception: " << cMsg);
    }
}



void ProtectedJSCallback(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> target, v8::Handle<v8::Function> cb) {
    const int argc =
#ifdef _WIN32
		1
#else
		0
#endif
		;
    Handle<Value> argv[argc] = { };
    ProtectedJSCallback(ctx, target, cb, argc, argv);
}

}

JSObjectScript::EvalContext::EvalContext()
 : currentScriptDir(),
   currentOutputStream(&std::cout)
{}

JSObjectScript::EvalContext::EvalContext(const EvalContext& rhs)
 : currentScriptDir(rhs.currentScriptDir),
   currentOutputStream(rhs.currentOutputStream)
{}


JSObjectScript::ScopedEvalContext::ScopedEvalContext(JSObjectScript* _parent, const EvalContext& _ctx)
 : parent(_parent)
{
    parent->mEvalContextStack.push(_ctx);
}

JSObjectScript::ScopedEvalContext::~ScopedEvalContext() {
    assert(!parent->mEvalContextStack.empty());
    parent->mEvalContextStack.pop();
}




JSObjectScript::JSObjectScript(HostedObjectPtr ho, const String& args, JSObjectScriptManager* jMan)
 : mParent(ho),
   mManager(jMan)
{

    OptionValue* init_script;
    InitializeClassOptions(
        "jsobjectscript", this,
        // Default value allows us to use std libs in the build tree, starting
        // from build/cmake
        init_script = new OptionValue("init-script","",OptionValueType<String>(),"Script to run on startup."),
        NULL
    );

    OptionSet* options = OptionSet::getOptions("jsobjectscript", this);
    options->parse(args);

    // By default, our eval context has:
    // 1. Empty currentScriptDir, indicating it should only use explicitly
    //    specified search paths.
    mEvalContextStack.push(EvalContext());

    v8::HandleScope handle_scope;
    mContext = v8::Context::New(NULL, mManager->mGlobalTemplate);


    Local<Object> global_obj = mContext->Global();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
    global_proto->SetInternalField(0, External::New(this));
    // And we add an internal field to the system object as well to make it
    // easier to find the pointer in different calls. Note that in this case we
    // don't use the prototype -- non-global objects work as we would expect.
    Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));
    system_obj->SetInternalField(SYSTEM_TEMPLATE_JSOBJSCRIPT_FIELD,External::New(this));

    //hangs math, presences, and addressable off of system_obj
    initializeMath(system_obj);
    initializePresences(system_obj);
    initializeVisible(system_obj);


    mHandlingEvent = false;

    // If we have a script to load, load it.
    //Always import the library



    String script_name = init_script->as<String>();
    if (!script_name.empty())
    {
        JSLOG(info,"Have an initial script to import from " + script_name );
        import(script_name);
    }

    // Subscribe for session events
    mParent->addListener((SessionEventListener*)this);
    // And notify the script of existing ones
    HostedObject::SpaceObjRefVec spaceobjrefs;
    mParent->getSpaceObjRefs(spaceobjrefs);
    if (spaceobjrefs.size() > 1)
        JSLOG(fatal,"Error: Connected to more than one space.  Only enabling scripting for one space.");
    for(HostedObject::SpaceObjRefVec::const_iterator space_it = spaceobjrefs.begin(); space_it != spaceobjrefs.end(); space_it++)
        onConnected(mParent, *space_it);

    mParent->getObjectHost()->persistEntityState(String("scene.persist"));

}


//removes the object from the visible array if it exists (returns the object
//itself so it can be passed into any user callbacks).  Otherwise asserts false.
v8::Handle<v8::Value> JSObjectScript::removeVisible(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    Local<Object>removedProxObj;

    v8::Local<v8::Array> vis_array = v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::VISIBLE_ARRAY_NAME)));
    int removedIndex = -1;
    JSVisibleStruct* jsvis= NULL;
    for (int s=0; s < (int)vis_array->Length(); ++s)
    {
        removedProxObj= v8::Local<v8::Object>::Cast(vis_array->Get(s));

        std::string errorMessage = "Error in removeVisible.  Unable to decode object.";
        jsvis= JSVisibleStruct::decodeVisible(vis_array->Get(s),errorMessage);

        if (jsvis == NULL)
        {
            JSLOG(error, errorMessage);
            return v8::Undefined();
        }
        
         
        if ( ((*jsvis->whatIsVisible)== proximateObject->getObjectReference()) &&
            (*jsvis->visibleToWhom == querier))
        {
            removedIndex = s;
            break;
        }
    }

    //actually remove from array.
    if (removedIndex == -1)
    {
        JSLOG(error, "Couldn't find the visible entity in the visible array.  Looking for object: "<<proximateObject->getObjectReference());
        //printVisibleArray();
        return v8::Undefined();
    }
    else
    {
        v8::Local<v8::Array> newVis = v8::Array::New();

        //create a new visible array without the previous member.
        for (uint32 i = 0; i < (uint32)removedIndex; ++i)
            newVis->Set(i,vis_array->Get(i));

        for(uint32 i = removedIndex; i < vis_array->Length()-1; i++)
            newVis->Set(i,vis_array->Get(i+1));

        getSystemObject()->Set(v8::String::New(JSSystemNames::VISIBLE_ARRAY_NAME), newVis);
    }

    *jsvis->stillVisible = false;
    //removedProxObj->Set(v8::String::New(JSSystemNames::VISIBLE_OBJECT_STILL_VISIBLE_FIELD),v8::Boolean::New(false));
    return removedProxObj;
}

void JSObjectScript::printVisibleArray()
{
    v8::Local<v8::Array> vis_array = v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::VISIBLE_ARRAY_NAME)));
    Local<Object>removedProxObj;

    std::cout<<"\nPrinting array.  This is array length: "<<vis_array->Length()<<"\n\n";
    std::cout.flush();
    for (int s=0; s < (int)vis_array->Length(); ++s)
    {
        std::string errorMessage = "Inside of JSObjectScript::printVisibleArray.  ";
        JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(vis_array->Get(s), errorMessage);
        if (jsvis != NULL)
            jsvis->printData();
        else
            JSLOG(error, errorMessage);

    }
}


void  JSObjectScript::notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" went out of query of "<<querier<<".  Mostly just ignoring it.");

    // Invoke user callback
    PresenceMap::iterator iter = mPresences.find(querier);
    if (iter == mPresences.end())
    {
        JSLOG(error,"No presence associated with sporef "<<querier<<" exists in presence mapping when getting notifyProximateGone.  Taking no action.");
        return;
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    v8::Handle<v8::Value>removedProxVal = removeVisible(proximateObject,querier);
    if (removedProxVal->IsUndefined())
        return;

    if (! removedProxVal->IsObject())
        return;

    v8::Local<v8::Object>removedProxObj;
    removedProxObj = removedProxVal->ToObject();

    
    
    if ( !iter->second->mOnProxRemovedEventHandler.IsEmpty() && !iter->second->mOnProxRemovedEventHandler->IsUndefined() && !iter->second->mOnProxRemovedEventHandler->IsNull())
    {
        //check if have the the removed prox obj has

        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { removedProxObj };
        //FIXME: Potential memory leak: when will removedProxObj's
        //SpaceObjectReference field be garbage collected and deleted?
        JSLOG(info,"Issuing user callback for proximate object gone.  Argument passed");
        ProtectedJSCallback(mContext, v8::Handle<Object>::Cast(v8::Undefined()), iter->second->mOnProxRemovedEventHandler, argc, argv);
    }
}


void  JSObjectScript::notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" is within query of "<<querier<<".");

    // Invoke user callback
    PresenceMap::iterator iter = mPresences.find(querier);
    if (iter == mPresences.end())
    {
        JSLOG(error,"No presence associated with sporef "<<querier<<" exists in presence mapping when getting notifyProximate.  Taking no action.");
        return;
    }
    
    if ( !iter->second->mOnProxAddedEventHandler.IsEmpty() && !iter->second->mOnProxAddedEventHandler->IsUndefined() && !iter->second->mOnProxAddedEventHandler->IsNull())
    {
        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(mContext);

        v8::Handle<v8::Value> newVisibleVal = addVisible(proximateObject,querier);

        if (newVisibleVal->IsUndefined())
            return;

        if (! newVisibleVal->IsObject())
            return;

        v8::Local<v8::Object> newVisibleObj = newVisibleVal->ToObject();

        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { newVisibleObj };
        //FIXME: Potential memory leak: when will newAddrObj's
        //SpaceObjectReference field be garbage collected and deleted?
        JSLOG(info,"Issuing user callback for proximate object.");
        ProtectedJSCallback(mContext, v8::Handle<Object>::Cast(v8::Undefined()), iter->second->mOnProxAddedEventHandler, argc, argv);
    }
}


JSInvokableObject::JSInvokableObjectInt* JSObjectScript::runSimulation(const SpaceObjectReference& sporef, const String& simname)
{
    TimeSteppedSimulation* sim = mParent->runSimulation(sporef,simname);

    return new JSInvokableObject::JSInvokableObjectInt(sim);
}



void JSObjectScript::onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name) {
    //register for scripting messages from user
    SpaceID space_id = name.space();
    ObjectReference obj_refer = name.object();

    v8::HandleScope handle_scope;


    Handle<Object> system_obj = getSystemObject();

    mScriptingPort = mParent->bindODPPort(space_id, obj_refer, Services::SCRIPTING);
    if (mScriptingPort)
        mScriptingPort->receive( std::tr1::bind(&JSObjectScript::handleScriptingMessageNewProto, this, _1, _2, _3));

    //register port for messaging
    mMessagingPort = mParent->bindODPPort(space_id, obj_refer, Services::COMMUNICATION);
    if (mMessagingPort)
        mMessagingPort->receive( std::tr1::bind(&JSObjectScript::handleCommunicationMessageNewProto, this, _1, _2, _3));

    mCreateEntityPort = mParent->bindODPPort(space_id,obj_refer, Services::CREATE_ENTITY);


    // For some reason, this *must* come before addPresence(). It can also be
    // called anywhere *inside* addPresence, so there must be some issue with
    // the Context::Scope or HandleScope.
    addSelfField(name);

    
    // Add to system.presences array
    v8::Handle<v8::Object> new_pres = addPresence(name);

    // Invoke user callback
    if ( !mOnPresenceConnectedHandler.IsEmpty() && !mOnPresenceConnectedHandler->IsUndefined() && !mOnPresenceConnectedHandler->IsNull() ) {
        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { new_pres };
        ProtectedJSCallback(mContext, v8::Handle<Object>::Cast(v8::Undefined()), mOnPresenceConnectedHandler, argc, argv);
    }
}


void JSObjectScript::addSelfField(const SpaceObjectReference& myName)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    
    v8::Local<v8::Object> selfVisObj = mManager->mVisibleTemplate->NewInstance();

    JSVisibleStruct* mySelf = new JSVisibleStruct (this, myName, myName, true,mParent->requestCurrentPosition(myName.space(), myName.object()));

    
    selfVisObj->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,v8::External::New(mySelf));
    
    v8::Handle<v8::Object> sysObj = getSystemObject();
    sysObj->Set(v8::String::New(JSSystemNames::VISIBLE_SELF_NAME), selfVisObj);
}



void JSObjectScript::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    v8::Handle<v8::Object> sysObj = getSystemObject();
    // Remove Self variable
    if (sysObj->Has(v8::String::New(JSSystemNames::VISIBLE_SELF_NAME)))
        sysObj->Delete(v8::String::New(JSSystemNames::VISIBLE_SELF_NAME));

    
    // Remove from system.presences array
    removePresence(name);

    // FIXME this should get the presence but its already been deleted
    if ( !mOnPresenceConnectedHandler.IsEmpty() && !mOnPresenceDisconnectedHandler->IsUndefined() && !mOnPresenceDisconnectedHandler->IsNull() )
        ProtectedJSCallback(mContext, v8::Handle<Object>::Cast(v8::Undefined()), mOnPresenceDisconnectedHandler);
}




void JSObjectScript::create_entity(EntityCreateInfo& eci)
{
    FIXME_GET_SPACE_OREF();

    HostedObjectPtr obj = HostedObject::construct<HostedObject>(mParent->context(), mParent->getObjectHost(), UUID::random());
    obj->init();
    if (eci.scriptType != "")
        obj->initializeScript(eci.scriptType, eci.scriptOpts);


    obj->connect(space,
        eci.loc,
        BoundingSphere3f(Vector3f::nil(), eci.scale),
        eci.mesh,
        eci.solid_angle,
        UUID::null(),
        NULL);
}


void JSObjectScript::reboot()
{
  // Need to delete the existing context? v8 garbage collects?

  v8::HandleScope handle_scope;
  mContext = v8::Context::New(NULL, mManager->mGlobalTemplate);
  Local<Object> global_obj = mContext->Global();
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  global_proto->SetInternalField(0, External::New(this));
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));

  populateSystemObject(system_obj);

  //delete all handlers
  for (int s=0; s < (int) mEventHandlers.size(); ++s)
      delete mEventHandlers[s];

  mEventHandlers.clear();

}

void JSObjectScript::debugPrintString(std::string cStrMsgBody) const
{
    JSLOG(debug,"Is it working: " << cStrMsgBody);
}


JSObjectScript::~JSObjectScript()
{
    if (mScriptingPort)
        delete mScriptingPort;

    for(JSEventHandlerList::iterator handler_it = mEventHandlers.begin(); handler_it != mEventHandlers.end(); handler_it++)
        delete *handler_it;

    mEventHandlers.clear();

    mContext.Dispose();
}


bool JSObjectScript::valid() const
{
    return (mParent);
}



//This function broadcasts message msgToBCast to all objects that satisfy your solid
//angle query.  
void JSObjectScript::broadcastVisible(SpaceObjectReference* visibleTo,const std::string& msgToBCast)
{
    v8::HandleScope handle_scope;
    v8::Local<v8::Array> vis_array = v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::VISIBLE_ARRAY_NAME)));
    v8::Local<v8::Object>iterObj;


    for (int s=0; s < (int)vis_array->Length(); ++s)
    {
        std::string errorMessage = "Inside of JSObjectScript::broadcastVisible.  ";
        JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(vis_array->Get(s), errorMessage);

        if (jsvis == NULL)
        {
            JSLOG(error, errorMessage);
            return;
        }

        if (*jsvis->visibleToWhom == *visibleTo)
            sendMessageToEntity(jsvis->whatIsVisible,visibleTo,msgToBCast);
    }
}



void JSObjectScript::sendMessageToEntity(SpaceObjectReference* sporef, SpaceObjectReference* from, const std::string& msgBody) const
{
    ODP::Endpoint dest (sporef->space(),sporef->object(),Services::COMMUNICATION);
    MemoryReference toSend(msgBody);

    mMessagingPort->send(dest,toSend);
}



//Will compile and run code in the context ctx whose source is em_script_str.
v8::Handle<v8::Value>JSObjectScript::internalEval(v8::Persistent<v8::Context>ctx,const String& em_script_str)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    // Special casing emerson compilation
    //lkjs;
    // #ifdef EMERSON_COMPILE

    // String em_script_str_new = em_script_str;

    // if(em_script_str.empty())
    //     return v8::Undefined();

    // if(em_script_str.at(em_script_str.size() -1) != '\n')
    // {
    //     em_script_str_new.push_back('\n');
    // }

    // emerson_init();
    // String js_script_str = string(emerson_compile(em_script_str_new.c_str()));
    // JSLOG(insane, " Compiled JS script = \n" <<js_script_str);
    // std::cout<<"\n\n Compiled JS script = \n" <<js_script_str<<"\n";
    // std::cout.flush();

    // v8::Handle<v8::String> source = v8::String::New(js_script_str.c_str(), js_script_str.size());
    // #else

    // // assume the input string to be a valid js rather than emerson
    // v8::Handle<v8::String> source = v8::String::New(em_script_str.c_str(), em_script_str.size());

    // #endif


    
    v8::Handle<v8::String> source = v8::String::New(em_script_str.c_str(), em_script_str.size());
    

    // Compile
    //note, because using compile command, will run in the mContext context
    v8::Handle<v8::Script> script = v8::Script::Compile(source);
    if (script.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        std::string msg = std::string("Compile error: ") + std::string(*error);
        JSLOG(error, msg);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(msg.c_str())) );
    }

    // Execute
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        JSLOG(error, "Uncaught exception: " << *error);
        return try_catch.Exception();
    }

    if (!result->IsUndefined()) {
        v8::String::AsciiValue ascii(result);
        JSLOG(detailed, "Script result: " << *ascii);
    }

    return result;
}

//defaults to internalEvaling with mContext, and does a ScopedEvalContext.
v8::Handle<v8::Value> JSObjectScript::protectedEval(const String& em_script_str, const EvalContext& new_ctx)
{
    ScopedEvalContext sec(this, new_ctx);
    return internalEval(mContext,em_script_str);
}



/*
  This function grabs the string associated with cb, and recompiles it in the
  context ctx.  It then calls the newly recompiled function from within ctx with
  args specified by argv and argc.
 */
void JSObjectScript::ProtectedJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Object> target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[])
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;


    v8::String::Utf8Value v8Source( cb->ToString());
    const char* cMsg = ToCString(v8Source);

    internalEval(ctx,String(ToCString(v8Source)));

    v8::Handle<v8::Value> funcName = cb->GetName();
    v8::String::Utf8Value v8funcNameString (funcName->ToString());
    const char* funcNameCStr = ToCString(v8funcNameString);

    if ( ! v8::Context::GetCurrent()->Global()->Has(funcName->ToString()))
    {
        JSLOG(error, "Uncaught exception: do not have the function name: "<< funcNameCStr <<" in current context.");
        return;
    }

    v8::Handle<v8::Value> compiledFunctionValue = v8::Context::GetCurrent()->Global()->Get(funcName->ToString());
    if (! compiledFunctionValue->IsObject())
    {
        JSLOG(error, "Uncaught exception: name is not associated with an object: "<< funcNameCStr <<" in current context.");
        return;
    }

    v8::Handle<v8::Object>  compiledFunction = compiledFunctionValue->ToObject();
    if (! compiledFunction->IsFunction())
    {
        JSLOG(error, "Uncaught exception: name is not associated with a function: "<< funcNameCStr <<" in current context.");
        return;
    }

    v8::Handle<v8::Function> funcInCtx = v8::Handle<v8::Function>::Cast(compiledFunction);

    Handle<Value> result;
    if (target->IsNull() || target->IsUndefined())
    {
        JSLOG(insane,"ProtectedJSCallback without target given.");
        result = funcInCtx->Call(ctx->Global(), argc, argv);
    }
    else
    {
        JSLOG(insane,"ProtectedJSCallback with target given.");
        result = funcInCtx->Call(target, argc, argv);
    }

    if (result.IsEmpty())
    {
        // FIXME what should we do with this exception?
        v8::String::Utf8Value error(try_catch.Exception());
        const char* cMsg = ToCString(error);
        JSLOG(error, "Uncaught exception: " << cMsg);
    }
}


/*
  executeInContext takes in a context, that you want to execute the function
  funcToCall in.  argv are the arguments to funcToCall from the current context,
  and are counted by argc.
 */
v8::Handle<v8::Value> JSObjectScript::executeInContext(v8::Persistent<v8::Context> &contExecIn, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    JSLOG(insane, "executing script in alternate context");

    std::vector<v8::Handle<v8::Context> >contextVec;
    while(v8::Context::InContext())
    {
        JSLOG(insane, "peeling away previous context I was in.");
        contextVec.push_back(v8::Context::GetCurrent());
        v8::Context::GetCurrent()->Exit();
    }

    //entering new context associated with
    JSLOG(insane, "entering new context associated with JSContextStruct.");
    contExecIn->Enter();

    JSLOG(insane, "Evaluating function in context associated with JSContextStruct.");
    ProtectedJSFunctionInContext(contExecIn, v8::Handle<v8::Object>::Cast(v8::Undefined()),funcToCall, argc, argv);

    JSLOG(insane, "Exiting new context associated with JSContextStruct.");
    contExecIn->Exit();

    //restore previous contexts
    std::vector<v8::Handle<v8::Context> >::reverse_iterator revIt;
    for (revIt= contextVec.rbegin(); revIt != contextVec.rend(); ++revIt)
    {
        JSLOG(insane, "restoring previous context I was in.");
        (*revIt)->Enter();
    }

    JSLOG(insane, "execution in alternate context complete");
    return v8::Undefined();
}




v8::Handle<v8::Value> JSObjectScript::addVisible(ProxyObjectPtr proximateObject,const SpaceObjectReference& querier)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    v8::Local<v8::Array> vis_array = v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::VISIBLE_ARRAY_NAME)));
    std::string errorMessage = "In addVisible of JSObjectScript.  ";
    JSVisibleStruct* jsvis   = NULL;
    
    for (int s=0; s < (int) vis_array->Length(); ++s)
    {
        jsvis = JSVisibleStruct::decodeVisible(vis_array->Get(s),errorMessage);
        if (jsvis == NULL)
        {
            JSLOG(error,errorMessage);
            return v8::Undefined();
        }

        if ((*jsvis->whatIsVisible == proximateObject->getObjectReference()) && (*jsvis->visibleToWhom == querier))
        {
            JSLOG(info, "Already had the associated object in visible array.  Returning empty.");
            return v8::Undefined();
        }
    }

    JSLOG(info, "Did not already have the associated object in visible array.  Creating new one.");
    //means that we don't already have this object in visible array.
    uint32 new_pos = vis_array->Length();

    //create the visible object associated with the new proxy object
    Local<Object> newVisObj = mManager->mVisibleTemplate->NewInstance();

    
    JSVisibleStruct* toAdd= new JSVisibleStruct(this, proximateObject->getObjectReference(),querier,true, mParent->requestCurrentPosition(proximateObject));

    newVisObj->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,External::New(toAdd));
    
    vis_array->Set(v8::Number::New(new_pos),newVisObj);
    return newVisObj;
}




void JSObjectScript::print(const String& str) {
    assert(!mEvalContextStack.empty());
    std::ostream* os = mEvalContextStack.top().currentOutputStream;
    assert(os != NULL);
    (*os) << str;
}


// void JSObjectScript::prev_timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb, JSContextStruct* jscont)
// {
//     // FIXME using the raw pointer isn't safe
//     FIXME_GET_SPACE_OREF();

//     Network::IOService* ioserve = mParent->getIOService();

//     ioserve->post(
//         dur,
//         std::tr1::bind(&JSObjectScript::handleTimeout,
//             this,
//             target,
//             cb
//         ));
// }



v8::Handle<v8::Value> JSObjectScript::create_timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont)
{
    //create timerstruct
    Network::IOService* ioserve = mParent->getIOService();
    JSTimerStruct* jstimer = new JSTimerStruct(this,dur,target,cb,jscont,ioserve);
    
    
    v8::HandleScope handle_scope;

    //create an object
    v8::Handle<v8::Object> returner  = mManager->mTimerTemplate->NewInstance();
    
    returner->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD,External::New(jstimer));

    return returner;
}




//third arg may be null to evaluate in global context
void JSObjectScript::handleTimeoutContext(v8::Persistent<v8::Object> target, v8::Persistent<v8::Function> cb,JSContextStruct* jscontext)
{
    if (jscontext == NULL)
        ProtectedJSCallback(mContext, target, cb);
    else
        ProtectedJSCallback(jscontext->mContext, target, cb);
}



v8::Handle<v8::Value> JSObjectScript::import(const String& filename) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    using namespace boost::filesystem;

    // Search through the import paths to find the file to import, searching the
    // current directory first if it is non-empty.
    path full_filename;
    path filename_as_path(filename);
    assert(!mEvalContextStack.empty());
    EvalContext& ctx = mEvalContextStack.top();
    if (!ctx.currentScriptDir.empty()) {
        path fq = ctx.currentScriptDir / filename_as_path;
        if (boost::filesystem::exists(fq))
            full_filename = fq;
    }
    if (full_filename.empty()) {
        std::list<String> search_paths = mManager->getOptions()->referenceOption("import-paths")->as< std::list<String> >();
        // Always search the current directory as a last resort
        search_paths.push_back(".");
        for (std::list<String>::iterator pit = search_paths.begin(); pit != search_paths.end(); pit++) {
            path fq = path(*pit) / filename_as_path;
            if (boost::filesystem::exists(fq)) {
                full_filename = fq;
                break;
            }
        }
    }

    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for import named");
        errorMessage+=filename;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
    }

    // Now try to read in and run the file.
    FILE * pFile;
    long lSize;
    char * buffer;
    long result;

    pFile = fopen (full_filename.string().c_str(), "r" );
    if (pFile == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Couldn't open file for import.")) );

    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    std::string contents(lSize, '\0');

    result = fread (&(contents[0]), 1, lSize, pFile);
    fclose (pFile);

    if (result != lSize)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Failure reading file for import.")) );

    EvalContext new_ctx(ctx);
    new_ctx.currentScriptDir = full_filename.parent_path().string();
    return protectedEval(contents, new_ctx);
}



//position


// need to ensure that the sender object is an visible of type spaceobject reference rather than just having an object reference;
JSEventHandler* JSObjectScript::registerHandler(const PatternList& pattern, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb, v8::Persistent<v8::Object>& sender)
{
    JSEventHandler* new_handler= new JSEventHandler(pattern, target, cb,sender);

    if ( mHandlingEvent)
    {
        //means that we're in the process of handling an event, and therefore
        //cannot push onto the event handlers list.  instead, add it to another
        //vector, which are additional changes to make after we've tried to
        //match all events.
        mQueuedHandlerEventsAdd.push_back(new_handler);
    }
    else
        mEventHandlers.push_back(new_handler);

    return new_handler;
}


//for debugging
void JSObjectScript::printAllHandlerLocations()
{
    std::cout<<"\nPrinting event handlers for "<<this<<": \n";
    for (int s=0; s < (int) mEventHandlers.size(); ++s)
        std::cout<<"\t"<<mEventHandlers[s];

    std::cout<<"\n\n\n";
}




v8::Handle<v8::Value> JSObjectScript::getVisibleFromArray(const SpaceObjectReference& visobj, const SpaceObjectReference& vistowhom)
{
    v8::Local<v8::Array> vis_array = v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::VISIBLE_ARRAY_NAME)));
    std::string errorMessage = "In addVisible of JSObjectScript.  Serious error.  ";
    JSVisibleStruct* jsvis   = NULL;
    
    for (int s=0; s < (int) vis_array->Length(); ++s)
    {
        jsvis = JSVisibleStruct::decodeVisible(vis_array->Get(s),errorMessage);
        if (jsvis == NULL)
        {
            JSLOG(error,errorMessage);
            continue;
        }

        if ((*jsvis->whatIsVisible == visobj) &&
            (*jsvis->visibleToWhom == vistowhom))
            return vis_array->Get(s);
    }

    return v8::Undefined();
}


/*
 * From the odp::endpoint & src and destination, checks if the corresponding
 * visible object already existed in the visible array.  If it does, return the
 * associated visible object.  If it doesn't, then return a new visible object
 * with stillVisible false associated with this object.
 */
v8::Handle<v8::Object> JSObjectScript::getMessageSender(const ODP::Endpoint& src, const ODP::Endpoint& dst)
{
    SpaceObjectReference from(src.space(), src.object());
    SpaceObjectReference to  (dst.space(), dst.object());

    v8::Handle<v8::Value> visFromArrayVal = getVisibleFromArray(from, to);

    if (visFromArrayVal->IsObject())
        return visFromArrayVal->ToObject();  //we found the object that we were
                                             //looking for in the visible
                                             //array.  returning it here.

    
    //didn't find the object that we were looking for in the visible array.
    v8::HandleScope handle_scope;
    v8::Handle<v8::Object> returner = mManager->mVisibleTemplate->NewInstance();

    JSVisibleStruct* visStruct = new JSVisibleStruct(this, from, to, false, Vector3d());
    returner->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,External::New(visStruct));
    return returner;
}


void JSObjectScript::handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Object> obj = v8::Object::New();


    v8::Handle<v8::Object> msgSender = getMessageSender(src,dst);
    //try deserialization

    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromArray(payload.data(), payload.size());

//bftm:clean all this up later

    if (! parsed)
    {
        JSLOG(error,"Cannot parse the message that I received on this port");
        assert(false);
    }

    bool deserializeWorks = JSSerializer::deserializeObject( js_msg,obj);

    if (! deserializeWorks)
        return;


    // Checks if matches some handler.  Try to dispatch the message
    bool matchesSomeHandler = false;


    //cannot affect the event handlers when we are executing event handlers.
    mHandlingEvent = true;

    for (int s=0; s < (int) mEventHandlers.size(); ++s)
    {
        if (mEventHandlers[s]->matches(obj,msgSender))
        {
            // Adding support for the knowing the message properties too
            int argc = 2;

            Handle<Value> argv[2] = { obj, msgSender };
            ProtectedJSCallback(mContext, mEventHandlers[s]->target, mEventHandlers[s]->cb, argc, argv);

            matchesSomeHandler = true;
        }
    }
    mHandlingEvent = false;
    flushQueuedHandlerEvents();

    /*
      FIXME: What should I do if the message that I receive does not match any handler?
     */
    if (!matchesSomeHandler) {
        JSLOG(info,"Message did not match any files");
    }
}



//This function takes care of all of the event handling changes that were queued
//while we were trying to match event happenings.
//adds all outstanding changes and then deletes all outstanding in that order.
void JSObjectScript::flushQueuedHandlerEvents()
{
    //Adding
    for (int s=0; s < (int)mQueuedHandlerEventsAdd.size(); ++s)
    {
        //add handlers requested to be added during matching of handlers
        mEventHandlers.push_back(mQueuedHandlerEventsAdd[s]);
    }
    mQueuedHandlerEventsAdd.clear();


    //deleting
    for (int s=0; s < (int)mQueuedHandlerEventsDelete.size(); ++s)
    {
        //remove handlers requested to be deleted during matching of handlers
        removeHandler(mQueuedHandlerEventsDelete[s]);
    }

    for (int s=0; s < (int) mQueuedHandlerEventsDelete.size(); ++s)
    {
        //actually delete the patterns
        //have to do this sort of tortured structure with comparing against
        //nulls in order to prevent deleting something twice (a user may have
        //tried to get rid of this handler multiple times).
        if (mQueuedHandlerEventsDelete[s] != NULL)
        {
            delete mQueuedHandlerEventsDelete[s];
            mQueuedHandlerEventsDelete[s] = NULL;
        }
    }
    mQueuedHandlerEventsDelete.clear();
}



void JSObjectScript::removeHandler(JSEventHandler* toRemove)
{
    JSEventHandlerList::iterator iter = mEventHandlers.begin();
    while (iter != mEventHandlers.end())
    {
        if ((*iter) == toRemove)
            iter = mEventHandlers.erase(iter);
        else
            ++iter;
    }
}

//takes in an event handler, if not currently handling an event, removes the
//handler from the vector and deletes it.  Otherwise, adds the handler for
//removal and deletion later.
void JSObjectScript::deleteHandler(JSEventHandler* toDelete)
{
    if (mHandlingEvent)
    {
        mQueuedHandlerEventsDelete.push_back(toDelete);
        return;
    }

    removeHandler(toDelete);
    delete toDelete;
    toDelete = NULL;
}



//this function is bound to the odp port for scripting messages.  It receives
//the commands that users type into the command terminal on the visual and
//parses them.
void JSObjectScript::handleScriptingMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    Sirikata::JS::Protocol::ScriptingMessage scripting_msg;
    bool parsed = scripting_msg.ParseFromArray(payload.data(), payload.size());
    if (!parsed) {
        JSLOG(fatal, "Parsing failed.");
        return;
    }

    Sirikata::JS::Protocol::ScriptingMessage scripting_reply;

    // Handle all requests
    for(int32 ii = 0; ii < scripting_msg.requests_size(); ii++)
    {
        Sirikata::JS::Protocol::ScriptingRequest req = scripting_msg.requests(ii);
        String script_str = req.body();

        // Replace output stream in context with string stream to collect
        // resulting output.
        std::stringstream output;
        assert(!mEvalContextStack.empty());
        EvalContext new_ctx(mEvalContextStack.top());
        new_ctx.currentOutputStream = &output;
        // No new context info currently, just copy the previous one
        protectedEval(script_str, new_ctx);

        if (output.str().size() > 0) {
            Sirikata::JS::Protocol::IScriptingReply reply = scripting_reply.add_replies();
            reply.set_id(req.id());
            reply.set_body(output.str());
        }
    }

    if (scripting_reply.replies_size() > 0) {
        std::string serialized_scripting_reply;
        scripting_reply.SerializeToString(&serialized_scripting_reply);
        mScriptingPort->send(src, MemoryReference(serialized_scripting_reply));
    }
}


//This function takes in a jseventhandler, and wraps a javascript object with
//it.  The function is called by registerEventHandler in JSSystem, which returns
//the js object this function creates to the user.
v8::Handle<v8::Object> JSObjectScript::makeEventHandlerObject(JSEventHandler* evHand)
{
    v8::Context::Scope context_scope(mContext);
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mHandlerTemplate->NewInstance();

    returner->SetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD, External::New(evHand));
    returner->SetInternalField(JSHANDLER_JSOBJSCRIPT_FIELD, External::New(this));

    return returner;
}



Handle<Object> JSObjectScript::getGlobalObject()
{
  // we really don't need a persistent handle
  //Local handles should work
  return mContext->Global();
}


Handle<Object> JSObjectScript::getSystemObject()
{
  HandleScope handle_scope;
  Local<Object> global_obj = mContext->Global();
  // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
  // The template actually generates the root objects prototype, not the root
  // itself.
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  // And we add an internal field to the system object as well to make it
  // easier to find the pointer in different calls. Note that in this case we
  // don't use the prototype -- non-global objects work as we would expect.
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));

  Persistent<Object> ret_obj = Persistent<Object>::New(system_obj);
  return ret_obj;
}


void JSObjectScript::initializeVisible(Handle<Object>&system_obj)
{
    v8::Context::Scope context_scope(mContext);
    // Create the space for the visibles, they get filled in by
    // onConnected/onCreateProxy calls
    v8::Local<v8::Array> arrayObj = v8::Array::New();
    system_obj->Set(v8::String::New(JSSystemNames::VISIBLE_ARRAY_NAME), arrayObj);
}


//called to build the presences array as well as to build the presence keyword
void JSObjectScript::initializePresences(Handle<Object>& system_obj)
{
    v8::Context::Scope context_scope(mContext);
    // Create the space for the presences, they get filled in by
    // onConnected/onDisconnected calls
    v8::Local<v8::Array> arrayObj = v8::Array::New();
    system_obj->Set(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME), arrayObj);
}

v8::Handle<v8::Object> JSObjectScript::addPresence(const SpaceObjectReference& sporef)
{
    HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    // Get the presences array
    v8::Local<v8::Array> presences_array =
        v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME)));
    uint32 new_pos = presences_array->Length();

    // Create the object for the new presence
    Local<Object> js_pres = mManager->mPresenceTemplate->NewInstance();
    JSPresenceStruct* presToAdd = new JSPresenceStruct(this, sporef);
    js_pres->SetInternalField(PRESENCE_FIELD_PRESENCE,External::New(presToAdd));

    // Add to our internal map
    mPresences[sporef] = presToAdd;

    // Insert into the presences array
    presences_array->Set(v8::Number::New(new_pos), js_pres);

    return js_pres;
}

//Generates a non-colliding 
uint32 JSObjectScript::registerUniqueMessageCode()
{
    uint32 returner = randInt<uint32>(0,MAX_MESSAGE_CODE);

    uint32 counter = 0;
    while( uniqueMessageCodeExists(returner))
    {
        returner = randInt<uint32>(0,MAX_MESSAGE_CODE);
        ++counter;
        if (counter > MAX_SEARCH_OPEN_CODE)
            break; //give up and just overload an object message.
    }

    mMessageCodes[returner] = true;
    return returner;
}

bool JSObjectScript::unregisterUniqueMessageCode(uint32 toUnregister)
{
    ScriptMessageCodes::iterator iter = mMessageCodes.find(toUnregister);
    if (iter == mMessageCodes.end())
        return false;

    mMessageCodes.erase(iter);
    return true;
}


bool JSObjectScript::uniqueMessageCodeExists(uint32 code)
{
    ScriptMessageCodes::iterator iter = mMessageCodes.find(code);
    return iter != mMessageCodes.end();
}



//presAssociatedWith: who the messages that this context's fakeroot sends will
//be from
//canMessage: who you can always send messages to.
//sendEveryone creates fakeroot that can send messages to everyone besides just
//who created you.
//recvEveryone means that you can receive messages from everyone besides just
//who created you.
//proxQueries means that you can issue proximity queries yourself, and latch on
//callbacks for them.
v8::Handle<v8::Value> JSObjectScript::createContext(JSPresenceStruct* presAssociatedWith,SpaceObjectReference* canMessage,bool sendEveryone, bool recvEveryone, bool proxQueries)
{
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mContextTemplate->NewInstance();
    returner->SetInternalField(CONTEXT_FIELD_CONTEXT_STRUCT, External::New(new JSContextStruct(this,presAssociatedWith,canMessage,sendEveryone,recvEveryone,proxQueries, mManager->mContextTemplate

            )));

    return returner;
}



void JSObjectScript::removePresence(const SpaceObjectReference& sporef)
{
    // Find and remove from internal storage
    PresenceMap::iterator internal_it = mPresences.find(sporef);
    if (internal_it == mPresences.end()) {
        JSLOG(error, "Got removePresence call for Presence that wasn't being tracked.");
        return;
    }
    JSPresenceStruct* presence = internal_it->second;
    delete presence;
    mPresences.erase(internal_it);

    // Get presences array, find and remove from that array.
    HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Array> presences_array
        = v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME)));
    // FIXME this is really annoying and completely unsafe. system.presences
    // shouldn't really be an array
    int32 remove_idx = -1;
    for(uint32 i = 0; i < presences_array->Length(); i++) {
        v8::Local<v8::Object> pres_i =
            v8::Local<v8::Object>::Cast(presences_array->Get(i));
        v8::Local<v8::External> pres_handle =
            v8::Local<v8::External>::Cast(pres_i->GetInternalField(PRESENCE_FIELD_PRESENCE));
        if (((JSPresenceStruct*)pres_handle->Value()) == presence) {
            remove_idx = i;
            break;
        }
    }
    if (remove_idx == -1) {
        JSLOG(error, "Couldn't find presence in system.presences array.");
        return;
    }
    for(uint32 i = remove_idx; i < presences_array->Length()-1; i++)
        presences_array->Set(i, presences_array->Get(i+1));
    presences_array->Delete(presences_array->Length()-1);
}

//this function can be called to re-initialize the system object's state
void JSObjectScript::populateSystemObject(Handle<Object>& system_obj)
{
    DEPRECATED(js);
}


void JSObjectScript::initializeMath(Handle<Object>& system_obj)
{
    v8::Context::Scope context_scope(mContext);

    Local<Object> mathObject = mManager->mMathTemplate->NewInstance();
    //no internal field to set for math object

    //attach math object to system object.
    system_obj->Set(v8::String::New(JSSystemNames::MATH_OBJECT_NAME),mathObject);
}


void JSObjectScript::create_presence(const SpaceID& new_space,std::string new_mesh)
{
  // const HostedObject::SpaceSet& spaces = mParent->spaces();
  // SpaceID spaceider = *(spaces.begin());

  FIXME_GET_SPACE_OREF();
  const BoundingSphere3f& bs = BoundingSphere3f(Vector3f(0, 0, 0), 1);

  //mParent->connectToSpace(new_space, mParent->getSharedPtr(), mParent->getLocation(spaceider),bs, mParent->getUUID());

  //FIXME: may want to start in a different place.
  Location startingLoc = mParent->getLocation(space,oref);

  //mParent->connect(new_space,startingLoc,bs, new_mesh,mParent->getUUID());

  NOT_IMPLEMENTED(js);// Must fix create_presence to use new connect interface
  assert(false);

  //FIXME: will need to add this presence to the presences vector.
  //but only want to do so when the function has succeeded.
}


//FIXME: Hard coded default mesh below
void JSObjectScript::create_presence(const SpaceID& new_space)
{
    NOT_IMPLEMENTED(js);
    assert(false);
    create_presence(new_space,"http://www.sirikata.com/content/assets/tetra.dae");
}


void JSObjectScript::setOrientationVelFunction(const SpaceObjectReference* sporef,const Quaternion& quat)
{
    mParent->requestOrientationVelocityUpdate(sporef->space(),sporef->object(),quat);
}

v8::Handle<v8::Value> JSObjectScript::getOrientationVelFunction(const SpaceObjectReference* sporef)
{
    Quaternion returner = mParent->requestCurrentOrientationVel(sporef->space(),sporef->object());
    return CreateJSResult(mContext, returner);
}



void JSObjectScript::setPositionFunction(const SpaceObjectReference* sporef, const Vector3f& posVec)
{
    mParent->requestPositionUpdate(sporef->space(),sporef->object(),posVec);
}

v8::Handle<v8::Value> JSObjectScript::getPositionFunction(const SpaceObjectReference* sporef)
{
    Vector3d vec3 = mParent->requestCurrentPosition(sporef->space(),sporef->object());
    return CreateJSResult(mContext,vec3);
}



//velocity
void JSObjectScript::setVelocityFunction(const SpaceObjectReference* sporef, const Vector3f& velVec)
{
    mParent->requestVelocityUpdate(sporef->space(),sporef->object(),velVec);
}

v8::Handle<v8::Value> JSObjectScript::getVelocityFunction(const SpaceObjectReference* sporef)
{
    Vector3f vec3f = mParent->requestCurrentVelocity(sporef->space(),sporef->object());
    return CreateJSResult(mContext,vec3f);
}


v8::Handle<v8::Value>JSObjectScript::returnProxyPosition(SpaceObjectReference*   sporef,SpaceObjectReference*   spVisTo, Vector3d* position)
{
    bool updateSucceeded = updatePosition(sporef, spVisTo, position);
    if (! updateSucceeded)
        JSLOG(error, "No sporefs associated with position.  Returning undefined.");
    
    return CreateJSResult(mContext,*position);
}


bool JSObjectScript::updatePosition(SpaceObjectReference* sporef, SpaceObjectReference* spVisTo,Vector3d* position)
{
    ProxyObjectPtr p;
    bool succeeded =  mParent->getProxyObjectFrom(spVisTo,sporef,p);

    if (succeeded)
        *position = mParent->requestCurrentPosition(p);


    return succeeded;
}


v8::Handle<v8::Value> JSObjectScript::printPositionFunction(const SpaceObjectReference* sporef,const SpaceObjectReference*   spVisTo)
{
    ProxyObjectPtr p;
    bool succeeded =  mParent->getProxyObjectFrom(spVisTo,sporef,p);

    if (succeeded)
    {
        Vector3d vec3d = p->getPosition();
        std::cout << "Printing position :" << vec3d << "\n";
        return v8::Undefined();
    }

    std::cout<<"\n\n  Error: no association for given sporefs\n\n";
    assert(false);
    return v8::Undefined();

}





//orientation
void  JSObjectScript::setOrientationFunction(const SpaceObjectReference* sporef, const Quaternion& quat)
{
    mParent->requestOrientationDirectionUpdate(sporef->space(),sporef->object(),quat);
}

v8::Handle<v8::Value> JSObjectScript::getOrientationFunction(const SpaceObjectReference* sporef)
{
    Quaternion curOrientation = mParent->requestCurrentOrientation(sporef->space(),sporef->object());
    return CreateJSResult(mContext,curOrientation);
}


//scale
v8::Handle<v8::Value> JSObjectScript::getVisualScaleFunction(const SpaceObjectReference* sporef)
{
    float curscale = mParent->requestCurrentBounds(sporef->space(),sporef->object()).radius();
    return CreateJSResult(mContext, curscale);
}

void JSObjectScript::setVisualScaleFunction(const SpaceObjectReference* sporef, float newscale)
{
    BoundingSphere3f bnds = mParent->requestCurrentBounds(sporef->space(),sporef->object());
    bnds = BoundingSphere3f(bnds.center(), newscale);
    mParent->requestBoundsUpdate(sporef->space(),sporef->object(), bnds);
}


//mesh
v8::Handle<v8::Value> JSObjectScript::getVisualFunction(const SpaceObjectReference* sporef)
{
    Transfer::URI uri_returner;
    bool hasMesh = mParent->requestMeshUri(sporef->space(),sporef->object(),uri_returner);

    if (! hasMesh)
        return v8::Undefined();

    std::string string_returner = uri_returner.toString();
    return v8::String::New(string_returner.c_str(), string_returner.size());
}



//FIXME: May want to have an error handler for this function.
void  JSObjectScript::setVisualFunction(const SpaceObjectReference* sporef, const std::string& newMeshString)
{
    //FIXME: need to also pass in the object reference
    mParent->requestMeshUpdate(sporef->space(),sporef->object(),newMeshString);
}


//just sets the solid angle query for the object.
void JSObjectScript::setQueryAngleFunction(const SpaceObjectReference* sporef, const SolidAngle& sa)
{
    mParent->requestQueryUpdate(sporef->space(), sporef->object(), sa);
}





} // namespace JS
} // namespace Sirikata
