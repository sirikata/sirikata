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
#include "JSObjectStructs/JSEventHandlerStruct.hpp"
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
#include "emerson/EmersonException.h"
#include "lexWhenPred/LexWhenPredUtil.h"
#include "emerson/Util.h"
#include "JSSystemNames.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSTimerStruct.hpp"
#include "JSObjects/JSObjectsUtils.hpp"
#include "JSObjectStructs/JSUtilStruct.hpp"
#include <boost/lexical_cast.hpp>
#include "JSVisibleStructMonitor.hpp"


using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {

namespace {

String exceptionAsString(v8::TryCatch& try_catch) {
    stringstream os;

    v8::HandleScope handle_scope;
    v8::String::Utf8Value exception(try_catch.Exception());
    const char* exception_string = ToCString(exception);
    v8::Handle<v8::Message> message = try_catch.Message();

    // Print (filename):(line number): (message).
    if (!message.IsEmpty()) {
        v8::String::Utf8Value filename(message->GetScriptResourceName());
        const char* filename_string = ToCString(filename);
        int linenum = message->GetLineNumber();
        os << filename_string << ':' << linenum << ": " << exception_string << "\n";
        // Print line of source code.
        v8::String::Utf8Value sourceline(message->GetSourceLine());
        const char* sourceline_string = ToCString(sourceline);
        os << sourceline_string << "\n";
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn();
        for (int i = 0; i < start; i++)
            os << " ";
        int end = message->GetEndColumn();
        for (int i = start; i < end; i++)
            os << "^";
        os << "\n";
    }
    v8::String::Utf8Value stack_trace(try_catch.StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      os << stack_trace_string << "\n";
    }
    return os.str();
}

void printException(v8::TryCatch& try_catch) {
    String eas = exceptionAsString(try_catch);
    JSLOG(error, "Uncaught exception:\n" << eas);
}

/** Note that this return value isn't guaranteed to return anything. If an
 *  exception occurs, it will be left empty. This is due to a quirk in the way
 *  v8 manages exceptions: TryCatch objects only work if there is active
 *  Javascript code on the stack. Therefore, we return the exception object in
 *  exc if the caller has provided a pointer for it.
 */
v8::Handle<v8::Value> ProtectedJSCallbackFull(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> *target, v8::Handle<v8::Function> cb, int argc, v8::Handle<v8::Value> argv[], String* exc = NULL) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    Handle<Value> result;
    if (target != NULL && !(*target)->IsNull() && !(*target)->IsUndefined()) {
        JSLOG(insane,"ProtectedJSCallback with target given.");
        result = cb->Call(*target, argc, argv);
    } else {
        JSLOG(insane,"ProtectedJSCallback without target given.");
        result = cb->Call(ctx->Global(), argc, argv);
    }

    if (try_catch.HasCaught())
    {
        printException(try_catch);
        if (exc != NULL)
            *exc = exceptionAsString(try_catch);
        return v8::Handle<v8::Value>();
    }

    return result;
}

v8::Handle<v8::Value> ProtectedJSCallbackFull(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> *target, v8::Handle<v8::Function> cb, String* exc = NULL) {
    return ProtectedJSCallbackFull(ctx, target, cb, 0, NULL, exc);
}

}

JSObjectScript::EvalContext::EvalContext()
 : currentScriptDir(),
   currentScriptBaseDir(),
   currentOutputStream(&std::cout)
{}

JSObjectScript::EvalContext::EvalContext(const EvalContext& rhs)
 : currentScriptDir(rhs.currentScriptDir),
   currentScriptBaseDir(rhs.currentScriptBaseDir),
   currentOutputStream(rhs.currentOutputStream)
{}

boost::filesystem::path JSObjectScript::EvalContext::getFullRelativeScriptDir() const {
    using namespace boost::filesystem;

    path result;
    // Scan through all matching parts
    path::const_iterator script_it, base_it;
    for(script_it = currentScriptDir.begin(), base_it = currentScriptBaseDir.begin(); base_it != currentScriptBaseDir.end(); script_it++, base_it++) {
        assert(*script_it == *base_it);
    }
    // Get any leftover script parts
    while(script_it != currentScriptDir.end()) {
        result /= *script_it;
        script_it++;
    }
    return result;
}


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
 : contIDTracker(0),
   mResetting(false),
   mParent(ho),
   mCreateEntityPort(NULL),
   mManager(jMan),
   presenceToken(HostedObject::DEFAULT_PRESENCE_TOKEN +1),
   hiddenObjectCount(0)
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

    mHandlingEvent = false;
    SpaceObjectReference sporef = SpaceObjectReference::null();
    mContext = new JSContextStruct(this,NULL,&sporef,true,true,true,true,true,true,true,mManager->mContextGlobalTemplate,contIDTracker);
    mContStructMap[contIDTracker] = mContext;
    ++contIDTracker;


    String script_contents = init_script->as<String>();
    if (script_contents.empty()) {
        JSLOG(info,"Importing default script.");
        import(mManager->defaultScript(),NULL);
        mContext->struct_setScript("system.require('" + mManager->defaultScript() + "');");
    }
    else {
        JSLOG(info,"Have an initial script to execute.  Executing.");
        EvalContext& ctx = mEvalContextStack.top();
        EvalContext new_ctx(ctx);
        v8::ScriptOrigin origin(v8::String::New("(original_import)"));
        protectedEval(script_contents, &origin, new_ctx,mContext);
        mContext->struct_setScript(script_contents);
    }

    // Subscribe for session events
    mParent->addListener((SessionEventListener*)this);
    // And notify the script of existing ones
    HostedObject::SpaceObjRefVec spaceobjrefs;
    mParent->getSpaceObjRefs(spaceobjrefs);
    if (spaceobjrefs.size() > 1)
        JSLOG(fatal,"Error: Connected to more than one space.  Only enabling scripting for one space.");

    //default connections.
    for(HostedObject::SpaceObjRefVec::const_iterator space_it = spaceobjrefs.begin(); space_it != spaceobjrefs.end(); space_it++)
        onConnected(mParent, *space_it, HostedObject::DEFAULT_PRESENCE_TOKEN);


    mParent->getObjectHost()->persistEntityState(String("scene.persist"));

}

void JSObjectScript::printExceptionToScript(JSContextStruct* ctx, const String& exc) {
    v8::Handle<v8::Object> sysobj = ctx->struct_getSystem();
    v8::Handle<v8::Value> printval = sysobj->Get(v8::String::New("print"));
    if (printval.IsEmpty() || !printval->IsFunction()) return;
    v8::Handle<v8::Function> printfunc = v8::Handle<v8::Function>::Cast(printval);

    int argc = 1;
    v8::Handle<v8::Value> argv[1] = { v8::String::New(exc.c_str()) };

    ProtectedJSCallbackFull(ctx->mContext, &sysobj, printfunc, argc, argv, NULL);
}

v8::Handle<v8::Value> JSObjectScript::invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]) {
    String exc;
    v8::Handle<v8::Value> retval = ProtectedJSCallbackFull(ctx->mContext, target, cb, argc, argv, &exc);
    if (retval.IsEmpty())
        printExceptionToScript(ctx, exc);
    return retval;
}

v8::Handle<v8::Value> JSObjectScript::invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]) {
    return invokeCallback(ctx, NULL, cb, argc, argv);
}

v8::Handle<v8::Value> JSObjectScript::invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb) {
    return invokeCallback(ctx, NULL, cb, 0, NULL);
}


//checks to see if the associated space object reference exists in the script.
//if it does, then make the position listener a subscriber to its pos updates.
bool JSObjectScript::registerPosAndMeshListener(SpaceObjectReference* sporef_toListenTo, SpaceObjectReference* ownPres_toListenFrom,PositionListener* pl,MeshListener* ml, TimedMotionVector3f* loc, TimedMotionQuaternion* orient, BoundingSphere3f* bs, String*mesh, String* phy)
{
    ProxyObjectPtr p;
    bool succeeded = false;
    if ((ownPres_toListenFrom ==NULL) || (*ownPres_toListenFrom == SpaceObjectReference::null()))
    {
        //trying to set to one of my own presence's postion listeners
        JSLOG(insane,"attempting to register position listener for one of my own presences with sporef "<<*ownPres_toListenFrom);
        succeeded = mParent->getProxy(sporef_toListenTo,p);
    }
    else
    {
        //trying to get a non-local proxy object
        JSLOG(insane,"attempting to register position listener for a visible object with sporef "<<*sporef_toListenTo);
        succeeded = mParent->getProxyObjectFrom(ownPres_toListenFrom,sporef_toListenTo,p);
    }

    //if actually had associated proxy object, then update loc and orientation.
    if (succeeded)
    {
        p->PositionProvider::addListener(pl);
        if (loc != NULL)
            *loc = p->getTimedMotionVector();
        if (orient != NULL)
            *orient = p->getTimedMotionQuaternion();

        p->MeshProvider::addListener(ml);
        if (bs != NULL)
            *bs = p->getBounds();
        if (mesh != NULL)
            *mesh = p->getMesh().toString();
        if (phy != NULL)
            *phy = p->getPhysics();
    }
    else
        JSLOG(insane,"problem registering to be a position listener. could not find associated object in hosted object.");

    return succeeded;

}

//Here's how resetting works.  system reqeusts a reset.  At this point,
//JSContextStruct calls the below function (requestReset).  If reset was
//requested by root context, then set mResetting to true.  In the check handlers
//function, if mResetting is true, stops comparing event against handlers.
//Then, call resetScript.  resetScript tears down the rest of the script.
v8::Handle<v8::Value> JSObjectScript::requestReset(JSContextStruct* jscont)
{
    if (jscont != mContext)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  Cannot call reset unless within root context.")));

    mResetting = true;
    return v8::Undefined();
}

void JSObjectScript::resetScript()
{
    mResetting = false;
    mPresences.clear();
    mEventHandlers.clear();
    mQueuedHandlerEventsAdd.clear();
    mQueuedHandlerEventsDelete.clear();
    mImportedFiles.clear();
    mContext->struct_rootReset();
    flushQueuedHandlerEvents();
}

/**
   This function adds jspresStruct to mPresences map.  Then, calls the
   onPresence added function.
 */
void JSObjectScript::resetPresence(JSPresenceStruct* jspresStruct)
{
    mPresences[*(jspresStruct->getToListenTo())] = jspresStruct;
}

bool JSObjectScript::isRootContext(JSContextStruct* jscont)
{
    return (jscont == mContext);
}


//deregisters position listening for an arbitrary proxy object visible to
//ownPres and with spaceobject reference sporef.
bool JSObjectScript::deRegisterPosAndMeshListener(SpaceObjectReference* sporef, SpaceObjectReference* ownPres,PositionListener* pl, MeshListener* ml)
{
    ProxyObjectPtr p;
    bool succeeded = false;

    if (sporef == NULL)
    {
        //de-regestering pl from position listening to one of my own presences.
        JSLOG(insane,"attempting to de-register position listener for one of my presences with sporef "<<*ownPres);
        succeeded = mParent->getProxy(ownPres,p);
    }
    else
    {
        //de-registering pl from position listening to an arbitrary proxy object
        JSLOG(insane,"attempting to de-register position listener for visible object with sporef "<<*sporef);
        succeeded =  mParent->getProxyObjectFrom(ownPres,sporef,p);
    }

    if (succeeded)
    {
        p->PositionProvider::removeListener(pl);
        p->MeshProvider::removeListener(ml);
    }
    else
        JSLOG(error,"error de-registering to be a position listener.  could not find associated object in hosted object.");

    return succeeded;

}



//this is the callback that fires when proximateObject no longer receives
//updates from loc (ie the object in the world associated with proximate object
//is outside of querier's standing query registered to pinto).
void  JSObjectScript::notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" went out of query of "<<querier<<".  Mostly just ignoring it.");

    //notifies the underlying struct associated with this object (if any exist)
    //that the proxy object is no longer visible.
    JSVisibleStructMonitor::checkNotifyNowNotVis(proximateObject->getObjectReference(),querier);

    PresenceMapIter iter = mPresences.find(querier);
    if (iter == mPresences.end())
    {
        JSLOG(error,"Error.  Received a notification that a proximate object left query set for querier "<<querier<<".  However, querier has no associated presence in array.  Aborting now");
        return;
    }

    //now see if must issue callback for this object's going out of scope
    if ( !iter->second->mOnProxRemovedEventHandler.IsEmpty() && !iter->second->mOnProxRemovedEventHandler->IsUndefined() && !iter->second->mOnProxRemovedEventHandler->IsNull())
    {
        JSVisibleStruct* jsvis =  checkVisStructExists(proximateObject->getObjectReference(),querier);
        if (jsvis == NULL)
        {
            JSLOG(error, "Error in notifyProximateGone of JSObjectScript.  Before receiving a notification that an object is no longer visible, should have received a notification that it was originally visible.  Error on object: "<<proximateObject->getObjectReference()<<" for querier: "<<querier<<".  Aborting call now.");
            return;
        }


        //jswrap the object
        //should be in context from createVisibleObject call
        v8::Handle<v8::Object> outOfRangeObject = createVisibleObject(jsvis,mContext->mContext);

        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(mContext->mContext);
        TryCatch try_catch;

        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { outOfRangeObject };
        //FIXME: Potential memory leak: when will removedProxObj's
        //SpaceObjectReference field be garbage collected and deleted?
        JSLOG(info,"Issuing user callback for proximate object gone.  Argument passed");
        invokeCallback(mContext, iter->second->mOnProxRemovedEventHandler, argc, argv);
    }
}


//creates a js object associated with the jsvisiblestruct
//will enter and exit the context passed in to make the object before returning
v8::Local<v8::Object> JSObjectScript::createVisibleObject(JSVisibleStruct* jsvis, v8::Handle<v8::Context> ctxToCreateIn)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctxToCreateIn);

    v8::Local<v8::Object> returner = mManager->mVisibleTemplate->GetFunction()->NewInstance();
    returner->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,v8::External::New(jsvis));
    returner->SetInternalField(TYPEID_FIELD,v8::External::New(new String(VISIBLE_TYPEID_STRING)));


    //v8::Persistent<v8::Object> returnerPers = v8::Persistent<v8::Object>::New(returner);

    return returner;
}

//attempts to make a new jsvisible struct...may be returned an existing one.
//then wraps it as v8 object.
v8::Persistent<v8::Object>  JSObjectScript::createVisiblePersistent(const SpaceObjectReference& visibleObj,VisAddParams* addParams, v8::Handle<v8::Context>ctx)
{
    SpaceObjectReference visTo = SpaceObjectReference::null();
    if ((addParams != NULL) && (addParams->mSporefWatchingFrom != NULL))
        visTo = * addParams->mSporefWatchingFrom;

    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this, visibleObj, visTo,addParams);
    return createVisiblePersistent(jsvis,ctx);
}


v8::Persistent<v8::Object> JSObjectScript::createVisiblePersistent(JSVisibleStruct* jsvis, v8::Handle<v8::Context> ctxToCreateIn)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctxToCreateIn);

    v8::Local<v8::Object> returner = mManager->mVisibleTemplate->GetFunction()->NewInstance();
    returner->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,v8::External::New(jsvis));
    returner->SetInternalField(TYPEID_FIELD,v8::External::New(new String(VISIBLE_TYPEID_STRING)));


    v8::Persistent<v8::Object> returnerPers = v8::Persistent<v8::Object>::New(returner);

    return returnerPers;
}



//attempts to make a new jsvisible struct...may be returned an existing one.
//then wraps it as v8 object.
v8::Local<v8::Object> JSObjectScript::createVisibleObject(const SpaceObjectReference& visibleObj,const SpaceObjectReference& visibleTo,VisAddParams* addParams, v8::Handle<v8::Context> ctx)
{
    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this, visibleObj, visibleTo, addParams);
    return createVisibleObject(jsvis,ctx);
}


//if can't find visible, will just return self object
//this is a mess of a function to get things to work again.
//this function will actually need to be super-cleaned up
v8::Handle<v8::Value> JSObjectScript::findVisible(const SpaceObjectReference& proximateObj)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);


    JSVisibleStruct* jsvis = JSVisibleStructMonitor::checkVisStructExists(proximateObj);

    if (jsvis != NULL)
    {
        v8::Persistent<v8::Object> returnerPers =createVisiblePersistent(jsvis, mContext->mContext);
        return returnerPers;
    }


    //otherwise return undefined
    return v8::Undefined();
}

//debugging code to output the sporefs of all the presences that I have in mPresences
void JSObjectScript::printMPresences()
{
    std::cout<<"\n\n";
    std::cout<<"Printing mPresences with size: "<< mPresences.size()<<"\n";
    for (PresenceMapIter iter = mPresences.begin(); iter != mPresences.end(); ++iter)
        std::cout<<"pres: "<<iter->first<<"\n";

    std::cout<<"\n\n";
}

//Gets called by notifier when PINTO states that proximateObject originally
//satisfies the solid angle query registered by querier
void  JSObjectScript::notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" is within query of "<<querier<<".");


    bool isVis = true;
    VisAddParams vap(&isVis);
    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this, proximateObject->getObjectReference(), querier, &vap);

    // Invoke user callback
    PresenceMapIter iter = mPresences.find(querier);
    if (iter == mPresences.end())
    {
        JSLOG(error,"No presence associated with sporef "<<querier<<" exists in presence mapping when getting notifyProximate.  Taking no action.");
        return;
    }

    //check callbacks
    if ( !iter->second->mOnProxAddedEventHandler.IsEmpty() && !iter->second->mOnProxAddedEventHandler->IsUndefined() && !iter->second->mOnProxAddedEventHandler->IsNull())
    {
        v8::Handle<v8::Object> newVisibleObj = createVisiblePersistent(jsvis, mContext->mContext);

        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(mContext->mContext);
        TryCatch try_catch;

        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { newVisibleObj };
        //FIXME: Potential memory leak: when will newAddrObj's
        //SpaceObjectReference field be garbage collected and deleted?
        JSLOG(info,"Issuing user callback for proximate object.");
        invokeCallback(mContext, iter->second->mOnProxAddedEventHandler, argc, argv);
    }
}


JSInvokableObject::JSInvokableObjectInt* JSObjectScript::runSimulation(const SpaceObjectReference& sporef, const String& simname)
{
    TimeSteppedSimulation* sim = mParent->runSimulation(sporef,simname);

    return new JSInvokableObject::JSInvokableObjectInt(sim);
}



void JSObjectScript::onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name, HostedObject::PresenceToken token)
{
    //register for scripting messages from user
    SpaceID space_id = name.space();
    ObjectReference obj_refer = name.object();

    v8::HandleScope handle_scope;

    //register port for messaging
    ODP::Port* msgPort = mParent->bindODPPort(space_id, obj_refer, Services::COMMUNICATION);
    if (msgPort != NULL)
    {
        mMessagingPortMap[SpaceObjectReference(space_id,obj_refer)] = msgPort;
        msgPort->receive( std::tr1::bind(&JSObjectScript::handleCommunicationMessageNewProto, this, _1, _2, _3));
    }

    if (!mCreateEntityPort)
        mCreateEntityPort = mParent->bindODPPort(space_id,obj_refer, Services::CREATE_ENTITY);

    //check for callbacks associated with presence connection

    //means that this is the first presence that has been added to the space
    if (token == HostedObject::DEFAULT_PRESENCE_TOKEN)
    {
        JSPresenceStruct* jspres = addConnectedPresence(name,token);
        mContext->checkContextConnectCallback(jspres);
    }
    else
    {
        //means that we've connected a presence and should finish by calling
        //connection callback
        callbackUnconnected(name,token);
    }
}


void JSObjectScript::callbackUnconnected(const SpaceObjectReference& name, HostedObject::PresenceToken token)
{
    for (PresenceVec::iterator iter = mUnconnectedPresences.begin(); iter != mUnconnectedPresences.end(); ++iter)
    {
        if (token == (*iter)->getPresenceToken())
        {
            mPresences[name] = *iter;
            (*iter)->connect(name);
            mUnconnectedPresences.erase(iter);
            return;
        }
    }
    JSLOG(error,"Error, received a finished connection with token "<<token<<" that we do not have an unconnected presence struct for.");
}


//called by JSPresenceStruct.  requests the parent HostedObject disconnect
//the presence associated with jspres
void JSObjectScript::requestDisconnect(JSPresenceStruct* jspres)
{

    SpaceObjectReference sporef = (*(jspres->getToListenTo()));
    mParent->disconnectFromSpace(sporef.space(), sporef.object());
}

void JSObjectScript::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name)
{
    JSPresenceStruct* jspres = findPresence(name);

    if (jspres != NULL)
        jspres->disconnectCalledFromObjScript();
}


void JSObjectScript::create_entity(EntityCreateInfo& eci)
{
    HostedObjectPtr obj = mParent->getObjectHost()->createObject(UUID::random(), &eci.scriptType, &eci.scriptOpts);

    obj->connect(eci.space,
        eci.loc,
        BoundingSphere3f(Vector3f::nil(), eci.scale),
        eci.mesh,
        eci.physics,
        eci.solid_angle,
        UUID::null(),
        ObjectReference::null()
    );
}


void JSObjectScript::debugPrintString(std::string cStrMsgBody) const
{
    JSLOG(debug,"Is it working: " << cStrMsgBody);
}


JSObjectScript::~JSObjectScript()
{
    for(JSEventHandlerList::iterator handler_it = mEventHandlers.begin(); handler_it != mEventHandlers.end(); handler_it++)
        delete *handler_it;

    mEventHandlers.clear();
}


bool JSObjectScript::valid() const
{
    return (mParent);
}




void JSObjectScript::sendMessageToEntity(SpaceObjectReference* sporef, SpaceObjectReference* from, const std::string& msgBody)
{

    std::map<SpaceObjectReference, ODP::Port*>::iterator iter = mMessagingPortMap.find(*from);
    if (iter == mMessagingPortMap.end())
    {
        JSLOG(error,"Trying to send from a sporef that does not exist");
        return;
    }

    ODP::Endpoint dest (sporef->space(),sporef->object(),Services::COMMUNICATION);
    MemoryReference toSend(msgBody);


    iter->second->send(dest,toSend);
}



Time JSObjectScript::getHostedTime()
{
    return mParent->currentLocalTime();
}

namespace {

class EmersonParserException {
public:
    EmersonParserException(uint32 li, uint32 pos, const String& msg)
     : line(li), position(pos), message(msg)
    {}

    uint32 line;
    uint32 position;
    String message;

    String toString() const {
        std::stringstream err_msg;
        err_msg << "SyntaxError at line " << line << " position " << position << ":\n";
        // FIXME it would be nice to give them the same ^ pointer we do for V8 errors.
        err_msg << message;
        return err_msg.str();
    }
};

void handleEmersonRecognitionError(struct ANTLR3_BASE_RECOGNIZER_struct* recognizer, pANTLR3_UINT8* tokenNames) {
    pANTLR3_EXCEPTION exception = recognizer->state->exception;
    throw EmersonParserException(exception->line, exception->charPositionInLine, (const char*)exception->message);
}
}

//Will compile and run code in the context ctx whose source is em_script_str.
v8::Handle<v8::Value>JSObjectScript::internalEval(v8::Persistent<v8::Context>ctx, const String& em_script_str, v8::ScriptOrigin* em_script_name, bool is_emerson)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    // Special casing emerson compilation
    v8::Handle<v8::String> source;
#ifdef EMERSON_COMPILE
    if (is_emerson) {
        String em_script_str_new = em_script_str;

        if(em_script_str.empty())
            return v8::Undefined();

        if(em_script_str.at(em_script_str.size() -1) != '\n')
        {
            em_script_str_new.push_back('\n');
        }

        emerson_init();

        JSLOG(insane, " Input Emerson script = \n" <<em_script_str_new);
        try {
            int em_compile_err = 0;
            v8::String::Utf8Value parent_script_name(em_script_name->ResourceName());
            const char* js_script_cstr = emerson_compile(String(ToCString(parent_script_name)), em_script_str_new.c_str(), em_compile_err, handleEmersonRecognitionError);
            String js_script_str;
            if (js_script_cstr != NULL) js_script_str = String(js_script_cstr);
            JSLOG(insane, " Compiled JS script = \n" <<js_script_str);

            source = v8::String::New(js_script_str.c_str(), js_script_str.size());
        }
        catch(EmersonParserException e) {
            return v8::ThrowException( v8::Exception::SyntaxError(v8::String::New(e.toString().c_str())) );
        }
    }
    else
#endif
        //assume the input string to be a valid js rather than emerson
        source = v8::String::New(em_script_str.c_str(), em_script_str.size());

    // Compile
    //note, because using compile command, will run in the mContext context
    v8::Handle<v8::Script> script = v8::Script::Compile(source, em_script_name);
    if (try_catch.HasCaught()) {
        v8::String::Utf8Value error(try_catch.Exception());
        String uncaught( *error);
        uncaught = "Uncaught exception " + uncaught + "\nwhen trying to run script: "+ em_script_str;
        JSLOG(error, uncaught);
        printException(try_catch);
        return try_catch.ReThrow();
    }


    if (script.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        std::string msg = std::string("Compile error: ") + std::string(*error);
        JSLOG(error, msg);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(msg.c_str())) );
    }


    TryCatch try_catch2;
    // Execute
    v8::Handle<v8::Value> result = script->Run();

    if (try_catch2.HasCaught()) {
        printException(try_catch2);
        return try_catch2.ReThrow();
    }


    if (!result.IsEmpty() && !result->IsUndefined()) {
        v8::String::AsciiValue ascii(result);
        JSLOG(detailed, "Script result: " << *ascii);
    }

    return result;
}




v8::Handle<v8::Value> JSObjectScript::protectedEval(const String& em_script_str, v8::ScriptOrigin* em_script_name, const EvalContext& new_ctx, JSContextStruct* jscs)
{
    ScopedEvalContext sec(this, new_ctx);
    if (jscs == NULL)
        return internalEval(mContext->mContext, em_script_str, em_script_name, true);

    return internalEval(jscs->mContext, em_script_str, em_script_name, true);
}

//takes the c++ object jspres, creates a new visible object out of it, if we
//don't already have a c++ visible object associated with it (if we do, use
//that one), wraps that c++ object in v8, and returns it as a v8 object to
//user
v8::Persistent<v8::Object> JSObjectScript::presToVis(JSPresenceStruct* jspres, JSContextStruct* jscont)
{
    bool isVis = true;
    VisAddParams vap(&isVis);


    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this,*(jspres->getSporef()),*(jspres->getSporef()),&vap);



    return createVisiblePersistent(jsvis, jscont->mContext);
}





/*
  This function takes the string associated with cb, re-compiles it in the
  passed-in context, and then passes it back out.  (Note, enclose the function
  string in parentheses and semi-colon to get around v8 idiosyncracy associated with
  compiling anonymous functions.  Ie, if didn't add those in, it may not compile
  anonymous functions.)
 */
v8::Handle<v8::Value> JSObjectScript::compileFunctionInContext(v8::Persistent<v8::Context>ctx, v8::Handle<v8::Function>&cb)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    String errorMessage= "Cannot interpret callback function as string while executing in context.  ";
    String v8Source;
    bool stringDecodeSuccessful = decodeString(cb->ToString(),v8Source,errorMessage);
    if (! stringDecodeSuccessful)
    {
        JSLOG(error, errorMessage);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );
    }

    v8Source = "(" + v8Source;
    v8Source += ");";

    ScriptOrigin cb_origin = cb->GetScriptOrigin();
    v8::Handle<v8::Value> compileFuncResult = internalEval(ctx, v8Source, &cb_origin, false);

    if (! compileFuncResult->IsFunction())
    {
        String errorMessage = "Uncaught exception: function passed in did not compile to a function";
        JSLOG(error, errorMessage);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );
    }

    //check if any errors occurred during compile.
    if (try_catch.HasCaught()) {
        JSLOG(error,"Error in compileFunctionInContext");
        printException(try_catch);
        return try_catch.Exception();
    }


    JSLOG(insane, "Successfully compiled function in context.  Passing back function object.");
    return compileFuncResult;
}

/*
  This function grabs the string associated with cb, and recompiles it in the
  context ctx.  It then calls the newly recompiled function from within ctx with
  args specified by argv and argc.
 */
v8::Handle<v8::Value> JSObjectScript::ProtectedJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[])
{

    v8::Handle<v8::Value> compiledFunc = compileFunctionInContext(ctx,cb);
    if (! compiledFunc->IsFunction())
    {
        JSLOG(error, "Callback did not compile to a function in ProtectedJSFunctionInContext.  Aborting execution.");
        //will be error message, or whatever else it compiled to.
        return compiledFunc;
    }

    v8::Handle<v8::Function> funcInCtx = v8::Handle<v8::Function>::Cast(compiledFunc);
    return executeJSFunctionInContext(ctx,funcInCtx,argc,target,argv);
}


/*
  This function takes the result of compileFunctionInContext, and executes it
  within context ctx.
 */
v8::Handle<v8::Value> JSObjectScript::executeJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Function> funcInCtx,int argc, v8::Handle<v8::Object>*target, v8::Handle<v8::Value> argv[])
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    v8::Handle<v8::Value> result;
    bool targetGiven = false;
    if (target!=NULL)
    {
        if (((*target)->IsNull() || (*target)->IsUndefined()))
        {
            JSLOG(insane,"executeJSFunctionInContext with target given.");
            result = funcInCtx->Call(*target, argc, argv);
            targetGiven = true;
        }
    }

    if (!targetGiven)
    {
        JSLOG(insane,"executeJSFunctionInContext without target given.");
        result = funcInCtx->Call(ctx->Global(), argc, argv);
    }

    //check if any errors have occurred.
    if (try_catch.HasCaught()) {
        JSLOG(error,"Error in executeJSFunctionInContext");
        printException(try_catch);
        return try_catch.Exception();
    }

    return result;
}



/*
  executeInSandbox takes in a context, that you want to execute the function
  funcToCall in.  argv are the arguments to funcToCall from the current context,
  and are counted by argc.
 */
v8::Handle<v8::Value>JSObjectScript::executeInSandbox(v8::Persistent<v8::Context> &contExecIn, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    JSLOG(insane, "executing script in alternate context");

    //entering new context associated with
    JSLOG(insane, "entering new context associated with JSContextStruct.");
    v8::Context::Scope context_scope(contExecIn);

    JSLOG(insane, "Evaluating function in context associated with JSContextStruct.");
    ProtectedJSFunctionInContext(contExecIn, NULL,funcToCall, argc, argv);

    JSLOG(insane, "execution in alternate context complete");
    return v8::Undefined();
}




void JSObjectScript::print(const String& str) {
    assert(!mEvalContextStack.empty());
    std::ostream* os = mEvalContextStack.top().currentOutputStream;
    assert(os != NULL);
    (*os) << str;
}


v8::Handle<v8::Value> JSObjectScript::create_timeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared, JSContextStruct* jscont)
{
    Network::IOService* ioserve = mParent->getIOService();
    JSTimerStruct* jstimer = new JSTimerStruct(this,Duration::seconds(period),cb,jscont,ioserve,contID, timeRemaining,isSuspended,isCleared);

    v8::HandleScope handle_scope;

    //create an object
    v8::Handle<v8::Object> returner  = mManager->mTimerTemplate->NewInstance();

    returner->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD,External::New(jstimer));
    returner->SetInternalField(TYPEID_FIELD, External::New(new String("timer")));

    return handle_scope.Close(returner);
}

v8::Handle<v8::Value> JSObjectScript::create_timeout(double period, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont)
{
    return create_timeout(period,cb,jscont->getContextID(),0,false,false,jscont);
}




//third arg may be null to evaluate in global context
void JSObjectScript::handleTimeoutContext(v8::Persistent<v8::Function> cb, JSContextStruct* jscontext)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope(jscontext->mContext);
    TryCatch try_catch;
    invokeCallback( (jscontext == NULL ? mContext : jscontext), cb);
}

//calls funcToCall in jscont, binding jspres bound as first arg.
//mostly used for contexts and presences to execute their callbacks on
//connection and disconnection events
void JSObjectScript::handlePresCallback( v8::Handle<v8::Function> funcToCall,JSContextStruct* jscont, JSPresenceStruct* jspres)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope(jscont->mContext);
    TryCatch try_catch;
    v8::Handle<v8::Value> js_pres =wrapPresence(jspres,&(jscont->mContext));
    invokeCallback(jscont, funcToCall, 1,&js_pres);
}




v8::Handle<v8::Value> JSObjectScript::eval(const String& contents, v8::ScriptOrigin* origin,JSContextStruct* jscs)
{
    EvalContext& ctx = mEvalContextStack.top();
    EvalContext new_ctx(ctx);

    if (jscs == NULL)
        return protectedEval(contents, origin, new_ctx, NULL);

    return protectedEval(contents, origin, new_ctx,jscs);
}


//takes in a name of a file to read from and execute all instructions within.
//also takes in a context to do so in.  If this context is null, just use
//mContext instead.
void JSObjectScript::resolveImport(const String& filename, boost::filesystem::path* full_file_out, boost::filesystem::path* base_path_out,JSContextStruct* contextCtx)
{
    v8::HandleScope handle_scope;

    if (contextCtx == NULL)
        contextCtx = mContext;

    v8::Context::Scope(contextCtx->mContext);

    using namespace boost::filesystem;

    // Search through the import paths to find the file to import, searching the
    // current directory first if it is non-empty.
    path filename_as_path(filename);
    assert(!mEvalContextStack.empty());
    EvalContext& ctx = mEvalContextStack.top();
    if (!ctx.currentScriptDir.empty()) {
        path fq = ctx.currentScriptDir / filename_as_path;
        try {
            if (boost::filesystem::exists(fq)) {
                *full_file_out = fq;
                *base_path_out = ctx.currentScriptBaseDir;
                return;
            }
        } catch (boost::filesystem::filesystem_error) {
            // Ignore, this just means we don't have access to some directory so
            // we can't check for its existence.
        }
    }

    std::list<String> search_paths = mManager->getOptions()->referenceOption("import-paths")->as< std::list<String> >();
    // Always search the current directory as a last resort
    search_paths.push_back(".");
    for (std::list<String>::iterator pit = search_paths.begin(); pit != search_paths.end(); pit++) {
        path base_path(*pit);
        path fq = base_path / filename_as_path;
        try {
            if (boost::filesystem::exists(fq)) {
                *full_file_out = fq;
                *base_path_out = base_path;
                return;
            }
        } catch (boost::filesystem::filesystem_error) {
            // Ignore, this just means we don't have access to some directory so
            // we can't check for its existence.
        }
    }

    *full_file_out = path();
    *base_path_out = path();

    return;
}

v8::Handle<v8::Value> JSObjectScript::absoluteImport(const boost::filesystem::path& full_filename, const boost::filesystem::path& full_base_dir,JSContextStruct* jscont)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(jscont ? jscont->mContext : mContext->mContext);

    JSLOG(detailed, " Performing import on absolute path: " << full_filename.string());

    // Now try to read in and run the file.
    FILE * pFile;
    long lSize;
    char * buffer;
    long result;

    pFile = fopen (full_filename.string().c_str(), "rb" );
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

    EvalContext& ctx = mEvalContextStack.top();
    EvalContext new_ctx(ctx);

    new_ctx.currentScriptDir = full_filename.parent_path();
    new_ctx.currentScriptBaseDir = full_base_dir;
    ScriptOrigin origin( v8::String::New( (new_ctx.getFullRelativeScriptDir() / full_filename.filename()).string().c_str() ) );

    mImportedFiles[jscont->getContextID()].insert( full_filename.string() );

    return  protectedEval(contents, &origin, new_ctx,jscont);
}

std::string* JSObjectScript::extensionize(const String filename)
{
    std::string* fileToFind = new std::string(filename);
    int index = filename.find(".");
    if(index == -1)
    {
      *fileToFind = *fileToFind + ".em";
    }
    else
    {
      if(filename.substr(index) != ".em")
      {
        // found a file with different extenstion
        return NULL;
        //std::string errorMessage("Cannot operate on  file: ");
        //errorMessage+=filename;
        //throw v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
      }
    }

    return fileToFind;

}

v8::Handle<v8::Value> JSObjectScript::import(const String& filename, JSContextStruct* jscont)
{

    JSLOG(detailed, "Importing: " << filename);
    std::string* fileToFind = extensionize(filename);
    if(fileToFind == NULL)
    {
      std::string errMsg = "Cannot import " + filename + ". Illegal file extension.";
      return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str()) ) );;
    }
    boost::filesystem::path full_filename, full_base;
    resolveImport(*fileToFind, &full_filename, &full_base,jscont);
    delete fileToFind;
    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for import named");
        errorMessage+=filename;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
    }
    return absoluteImport(full_filename, full_base,jscont);
}


v8::Handle<v8::Value> JSObjectScript::require(const String& filename,JSContextStruct* jscont)
{
    JSLOG(detailed, "Requiring: " << filename);
    std::string* fileToFind = extensionize(filename);
    if(fileToFind == NULL)
    {
      std::string errMsg = "Cannot import " + filename + ". Illegal file extension.";
      return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str()) ) );;
    }

    boost::filesystem::path full_filename, full_base;
    resolveImport(*fileToFind, &full_filename, &full_base,jscont);
    delete fileToFind;
    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for require named");
        errorMessage += filename;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
    }

    ImportedFileMapIter iter = mImportedFiles.find(jscont->getContextID());
    if (iter != mImportedFiles.end())
    {
        if (iter->second.find(full_filename.string()) != iter->second.end())
        {
        //JSLOG(detailed, " Skipping already imported file: " << filename);
        JSLOG(debug, " Skipping already imported file: " << filename);
        return v8::Undefined();
        }
    }

    return absoluteImport(full_filename, full_base,jscont);
}



//tries to add the handler struct to the list of event handlers.
//if am in the middle of processing an event handler, defers the
//addition to after the event has finished.
void JSObjectScript::registerHandler(JSEventHandlerStruct* jsehs)
{
    if ( mHandlingEvent)
    {
        //means that we're in the process of handling an event, and therefore
        //cannot push onto the event handlers list.  instead, add it to another
        //vector, which are additional changes to make after we've tried to
        //match all events.
        mQueuedHandlerEventsAdd.push_back(jsehs);
    }
    else
        mEventHandlers.push_back(jsehs);

}


//for debugging
void JSObjectScript::printAllHandlers()
{
    std::cout<<"\nDEBUG: printing all handlers\n";
    for (int s=0; s < (int) mEventHandlers.size(); ++s)
        mEventHandlers[s]->printHandler();

    std::cout<<"\n\n\n";
}




/*
 * From the odp::endpoint & src and destination, checks if the corresponding
 * visible object already existed in the visible array.  If it does, return the
 * associated visible object.  If it doesn't, then return a new visible object
 * with stillVisible false associated with this object.
 */
v8::Handle<v8::Object> JSObjectScript::getMessageSender(const ODP::Endpoint& src, const ODP::Endpoint& dst)
{
    v8::HandleScope handle_scope;
    SpaceObjectReference from(src.space(), src.object());
    SpaceObjectReference to  (dst.space(), dst.object());


    bool isVis = false;
    VisAddParams vap(&isVis);

    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this,from,to,&vap);
    v8::Persistent<v8::Object> returner =createVisiblePersistent(jsvis, mContext->mContext);

    return returner;
}


void JSObjectScript::registerFixupSuspendable(JSSuspendable* jssusp, uint32 contID)
{
    toFixup[contID].push_back(jssusp);
}


void JSObjectScript::handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);
    v8::Local<v8::Object> obj = v8::Object::New();


    v8::Handle<v8::Object> msgSender = getMessageSender(src,dst);
    //try deserialization

    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromArray(payload.data(), payload.size());



    if (! parsed)
    {
        JSLOG(error,"Cannot parse the message that I received on this port");
        return;
    }

    bool deserializeWorks = JSSerializer::deserializeObject( this, js_msg,obj);

    if (! deserializeWorks)
    {
        JSLOG(error, "Deserialization Failed!!");
        return;
    }

    // Checks if matches some handler.  Try to dispatch the message
    bool matchesSomeHandler = false;

    SpaceObjectReference to  (dst.space(), dst.object());

    //cannot affect the event handlers when we are executing event handlers.
    mHandlingEvent = true;



    for (int s=0; s < (int) mEventHandlers.size(); ++s)
    {
        if (mResetting)
            break;

        if (mEventHandlers[s]->matches(obj,msgSender,to))
        {
            // Adding support for the knowing the message properties too
            int argc = 3;
            Handle<Value> argv[3] = { obj, msgSender, v8::String::New (to.toString().c_str()) };
            TryCatch try_catch;
            invokeCallback(mContext, NULL, mEventHandlers[s]->cb, argc, argv);

            matchesSomeHandler = true;
        }
    }
    mHandlingEvent = false;
    flushQueuedHandlerEvents();

    //if one of the actions that your handler took was to call reset, then reset
    //the entire script.
    if (mResetting)
        resetScript();

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



void JSObjectScript::removeHandler(JSEventHandlerStruct* toRemove)
{
    JSEventHandlerList::iterator iter = mEventHandlers.begin();
    while (iter != mEventHandlers.end())
    {
        if ((*iter) == toRemove)
        {
            (*iter)->clear();
            iter = mEventHandlers.erase(iter);
        }
        else
            ++iter;
    }
}

//takes in an event handler, if not currently handling an event, removes the
//handler from the vector and deletes it.  Otherwise, adds the handler for
//removal and deletion later.
void JSObjectScript::deleteHandler(JSEventHandlerStruct* toDelete)
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


//This function takes in a jseventhandler, and wraps a javascript object with
//it.  The function is called by registerEventHandler in JSSystem, which returns
//the js object this function creates to the user.
v8::Handle<v8::Object> JSObjectScript::makeEventHandlerObject(JSEventHandlerStruct* evHand, JSContextStruct* jscs)
{
    v8::Handle<v8::Context> ctx = (jscs == NULL) ? mContext->mContext : jscs->mContext;
    v8::Context::Scope context_scope(ctx);
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mHandlerTemplate->NewInstance();

    returner->SetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD, External::New(evHand));
    returner->SetInternalField(JSHANDLER_JSOBJSCRIPT_FIELD, External::New(this));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String (JSHANDLER_TYPEID_STRING)));

    return handle_scope.Close(returner);
}



JSPresenceStruct*  JSObjectScript::addConnectedPresence(const SpaceObjectReference& sporef,HostedObject::PresenceToken token)
{
    JSPresenceStruct* presToAdd = new JSPresenceStruct(this, sporef,mContext,token);
    // Add to our internal map
    mPresences[sporef] = presToAdd;
    return presToAdd;
}


//should be called from something that already has declared a handlescope,
//wraps the presence in a v8 object and returns it.
v8::Local<v8::Object> JSObjectScript::wrapPresence(JSPresenceStruct* presToWrap, v8::Persistent<v8::Context>* ctxToWrapIn)
{
    v8::HandleScope handle_scope;
    v8::Handle<v8::Context> ctx = (ctxToWrapIn == NULL) ? mContext->mContext : *ctxToWrapIn;
    v8::Context::Scope context_scope(ctx);

    Local<Object> js_pres = mManager->mPresenceTemplate->GetFunction()->NewInstance();
    js_pres->SetInternalField(PRESENCE_FIELD_PRESENCE,External::New(presToWrap));
    js_pres->SetInternalField(TYPEID_FIELD,External::New(new String(PRESENCE_TYPEID_STRING)));

    return handle_scope.Close(js_pres);
}



//presAssociatedWith: who the messages that this context's system sends will
//be from
//canMessage: who you can always send messages to.
//sendEveryone creates system that can send messages to everyone besides just
//who created you.
//recvEveryone means that you can receive messages from everyone besides just
//who created you.
//proxQueries means that you can issue proximity queries yourself, and latch on
//callbacks for them.
//canImport means that you can import files/libraries into your code.
//canCreatePres is whether have capability to create presences
//canCreateEnt is whether have capability to create entities
//last field returns the created context struct by reference
v8::Local<v8::Object> JSObjectScript::createContext(JSPresenceStruct* presAssociatedWith,SpaceObjectReference* canMessage,bool sendEveryone, bool recvEveryone, bool proxQueries, bool canImport, bool canCreatePres,bool canCreateEnt,bool canEval,JSContextStruct*& internalContextField)
{
    v8::HandleScope handle_scope;

    v8::Local<v8::Object> returner =mManager->mContextTemplate->NewInstance();
    internalContextField = new JSContextStruct(this,presAssociatedWith,canMessage,sendEveryone,recvEveryone,proxQueries, canImport,canCreatePres,canCreateEnt,canEval,mManager->mContextGlobalTemplate, contIDTracker);
    mContStructMap[contIDTracker] = internalContextField;
    ++contIDTracker;


    returner->SetInternalField(CONTEXT_FIELD_CONTEXT_STRUCT, External::New(internalContextField));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String(CONTEXT_TYPEID_STRING)));

    return returner;
}

//looks through all previously connected presneces (located in mPresences).
//returns the corresponding jspresencestruct that has a spaceobjectreference
//that matches sporef.
JSPresenceStruct* JSObjectScript::findPresence(const SpaceObjectReference& sporef)
{
    PresenceMap::iterator internal_it = mPresences.find(sporef);
    if (internal_it == mPresences.end())
    {
        JSLOG(error, "Got findPresence call for Presence that wasn't being tracked.");
        return NULL;
    }
    return internal_it->second;
}




v8::Handle<v8::Function> JSObjectScript::functionValue(const String& js_script_str)
{
  v8::HandleScope handle_scope;

  static int32_t counter;
  std::stringstream sstream;
  sstream <<  " __emerson_deserialized_function_" << counter << "__ = " << js_script_str << ";";

  //const std::string new_code = std::string("(function () { return ") + js_script_str + "}());";
  // The function name is not required. It is being put in because emerson is not compiling "( function() {} )"; correctly
  const std::string new_code = sstream.str();
  counter++;

  v8::ScriptOrigin origin(v8::String::New("(deserialized)"));
  v8::Local<v8::Value> v = v8::Local<v8::Value>::New(internalEval(mContext->mContext, new_code, &origin, false));
  v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(v);
  v8::Persistent<v8::Function> pf = v8::Persistent<v8::Function>::New(f);
  return pf;
}


v8::Handle<v8::Value> JSObjectScript::restorePresence(PresStructRestoreParams& psrp,JSContextStruct* jsctx)
{

    if (jsctx != mContext)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Can only restore presence from root context.")) );


    v8::Context::Scope context_scope(jsctx->mContext);


    //get location
    Vector3f newPos            = psrp.mTmv3f->extrapolate(mParent->currentLocalTime()).position();
    Quaternion newOrient       = psrp.mTmq->extrapolate(mParent->currentLocalTime()).position();
    Vector3f newVel            = psrp.mTmv3f->velocity();
    Quaternion orientVel       = psrp.mTmq->velocity();

    Vector3f newAngAxis;
    float newAngVel;
    orientVel.toAngleAxis(newAngVel,newAngAxis);

    Vector3d newPosD(newPos.x,newPos.y,newPos.z);
    Location newLoc(newPosD,newOrient,newVel, newAngAxis,newAngVel);


    //get bounding sphere
    BoundingSphere3f bs = BoundingSphere3f(newPos, *psrp.mScale);

    HostedObject::PresenceToken presToke = incrementPresenceToken();
    JSPresenceStruct* jspres = new JSPresenceStruct(this,psrp,newPos,presToke,jsctx);



    if (*psrp.mIsConnected)
    {
        mParent->connect(psrp.mSporef->space(),
            newLoc,
            bs,
            *psrp.mMesh,
            "",
            SolidAngle::Max,
            UUID::null(),
            psrp.mSporef->object(),
            presToke);

        mUnconnectedPresences.push_back(jspres);

        return v8::Null();
    }
    //if is unconnected, return presence now.
    v8::HandleScope handle_scope;
    return handle_scope.Close(wrapPresence(jspres,&(jsctx->mContext)));
}


//takes in a string corresponding to the new presence's mesh and a function
//callback to run when the presence is connected.
v8::Handle<v8::Value> JSObjectScript::create_presence(const String& newMesh, v8::Handle<v8::Function> callback, JSContextStruct* jsctx, const Vector3d& poser, const SpaceID& spaceToCreateIn)
{
    if (jsctx == NULL)
        jsctx = mContext;

    v8::Context::Scope context_scope(jsctx->mContext);

    //presuming that we are connecting to the same space;
    //arbitrarily saying that we'll just be on top of the root object.
    Location startingLoc(poser,Quaternion::identity(),Vector3f(0,0,0),Vector3f(0,1,0),0);

    //Arbitrarily saying that we're just going to use a simple bounding sphere.
    BoundingSphere3f bs = BoundingSphere3f(Vector3f::nil(), 1);

    HostedObject::PresenceToken presToke = incrementPresenceToken();
    mParent->connect(spaceToCreateIn,startingLoc,bs, newMesh, "", SolidAngle::Max,UUID::null(),ObjectReference::null(),presToke);



    //create a presence object associated with this presence and return it;
    JSPresenceStruct* presToAdd = new JSPresenceStruct(this,callback,jsctx,presToke);

    //v8::Persistent<v8::Object>js_pres = jsctx->addToPresencesArray(presToAdd);
    mUnconnectedPresences.push_back(presToAdd);

    return v8::Undefined();
}

//This function returns to you the current value of present token and incrmenets
//presenceToken so that get a unique one each time.  If presenceToken is equal
//to default_presence_token, increments one beyond it so that don't start inadvertently
//returning the DEFAULT_PRESENCE_TOKEN;
HostedObject::PresenceToken JSObjectScript::incrementPresenceToken()
{
    HostedObject::PresenceToken returner = presenceToken++;
    if (returner == HostedObject::DEFAULT_PRESENCE_TOKEN)
        return incrementPresenceToken();

    return returner;
}


void JSObjectScript::setOrientationVelFunction(const SpaceObjectReference* sporef,const Quaternion& quat)
{
    mParent->requestOrientationVelocityUpdate(sporef->space(),sporef->object(),quat);
}




void JSObjectScript::setPositionFunction(const SpaceObjectReference* sporef, const Vector3f& posVec)
{
    mParent->requestPositionUpdate(sporef->space(),sporef->object(),posVec);
}



//velocity
void JSObjectScript::setVelocityFunction(const SpaceObjectReference* sporef, const Vector3f& velVec)
{
    mParent->requestVelocityUpdate(sporef->space(),sporef->object(),velVec);
}



//orientation
void  JSObjectScript::setOrientationFunction(const SpaceObjectReference* sporef, const Quaternion& quat)
{
    mParent->requestOrientationDirectionUpdate(sporef->space(),sporef->object(),quat);
}



//scale
void JSObjectScript::setVisualScaleFunction(const SpaceObjectReference* sporef, float newscale)
{
    BoundingSphere3f bnds = mParent->requestCurrentBounds(sporef->space(),sporef->object());
    bnds = BoundingSphere3f(bnds.center(), newscale);
    mParent->requestBoundsUpdate(sporef->space(),sporef->object(), bnds);
}



//mesh
//FIXME: May want to have an error handler for this function.
void  JSObjectScript::setVisualFunction(const SpaceObjectReference* sporef, const std::string& newMeshString)
{
    //FIXME: need to also pass in the object reference
    mParent->requestMeshUpdate(sporef->space(),sporef->object(),newMeshString);
}

//physics
v8::Handle<v8::Value> JSObjectScript::getPhysicsFunction(const SpaceObjectReference* sporef)
{
    String curphy = mParent->requestCurrentPhysics(sporef->space(),sporef->object());
    return v8::String::New(curphy.c_str(), curphy.size());
}

//FIXME: May want to have an error handler for this function.
void JSObjectScript::setPhysicsFunction(const SpaceObjectReference* sporef, const String& newPhyString)
{
    //FIXME: need to also pass in the object reference
    mParent->requestPhysicsUpdate(sporef->space(), sporef->object(), newPhyString);
}


//just sets the solid angle query for the object.
void JSObjectScript::setQueryAngleFunction(const SpaceObjectReference* sporef, const SolidAngle& sa)
{
    mParent->requestQueryUpdate(sporef->space(), sporef->object(), sa);
}


/*
  This function takes the value passed in as val, and creates a variable in ctx
  corresponding to it.  The variable name that it creates is guaranteed to be
  non-colliding with other variables that already exist.

  The function then returns the name of this new value as a string.
 */
String JSObjectScript::createNewValueInContext(v8::Handle<v8::Value> val, v8::Handle<v8::Context> ctx)
{
    String newName = "$_"+ boost::lexical_cast<String>(hiddenObjectCount);

    v8::HandleScope handle_scope;

    v8::Local<v8::Object> ctx_global_obj = ctx->Global();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    v8::Handle<v8::Object> ctx_global_proto = Handle<Object>::Cast(ctx_global_obj->GetPrototype());

    ctx_global_proto->Set(v8::String::New(newName.c_str(), newName.length()), val);

    ++hiddenObjectCount;
    return newName;
}


} // namespace JS
} // namespace Sirikata
