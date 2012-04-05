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
#include "EmersonScript.hpp"

#include "JSObjects/JSVec3.hpp"
#include "JSObjects/JSQuaternion.hpp"
#include "JSObjects/JSVisible.hpp"
#include "JSObjects/JSSystem.hpp"

#include "JSObjects/JSUtilObj.hpp"
#include "JSObjects/JSTimer.hpp"
#include "JSSerializer.hpp"


#include "JSObjects/JSPresence.hpp"
#include "JSObjects/JSGlobal.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/JSInvokableObject.hpp"
#include "JSSystemNames.hpp"
#include "JSObjects/JSContext.hpp"

#include "JSLogging.hpp"

#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <sirikata/mesh/CompositeFilter.hpp>

#include <sirikata/core/transfer/AggregatedTransferPool.hpp>

#include <sirikata/core/util/Paths.hpp>


namespace Sirikata {
namespace JS {

using namespace Sirikata::Mesh;

ObjectScriptManager* JSObjectScriptManager::createObjectScriptManager(ObjectHostContext* ctx, const Sirikata::String& arguments) {
    return new JSObjectScriptManager(ctx, arguments);
}



JSObjectScriptManager::JSObjectScriptManager(ObjectHostContext* ctx, const Sirikata::String& arguments)
 : mContext(ctx),
   mTransferPool(),
   mParsingIOService(NULL),
   mParsingWork(NULL),
   mParsingThread(NULL),
   mModelParser(NULL),
   mModelFilter(NULL)
{
    // In emheadless we run without an ObjectHostContext
    if (mContext != NULL) {
        mTransferPool = Transfer::TransferMediator::getSingleton().registerClient<Transfer::AggregatedTransferPool>("JSObjectScriptManager");

        // TODO(ewencp) This should just be a strand on the main service
        mParsingIOService = new Network::IOService("JSObjectScriptManager Parsing");
        mParsingWork = new Network::IOWork(*mParsingIOService, "JSObjectScriptManager Mesh Parsing");
        mParsingThread = new Sirikata::Thread("JSObjectScriptManager Model Parsing", std::tr1::bind(&Network::IOService::runNoReturn, mParsingIOService));

        mModelParser = ModelsSystemFactory::getSingleton ().getConstructor ( "any" ) ( "" );
        try {
            // These have to be consistent with any other simulations -- e.g. the
            // space bullet plugin and scripting plugins that expose mesh data
            std::vector<String> names_and_args;
            names_and_args.push_back("triangulate"); names_and_args.push_back("all");
            names_and_args.push_back("compute-normals"); names_and_args.push_back("");
            names_and_args.push_back("center"); names_and_args.push_back("");
            mModelFilter = new Mesh::CompositeFilter(names_and_args);
        }
        catch(Mesh::CompositeFilter::Exception e) {
            SILOG(js,warning,"Couldn't allocate requested model load filter, will not apply filter to loaded models.");
            mModelFilter = NULL;
        }
    }


    OptionValue* default_import_paths;
    OptionValue* import_paths;
    OptionValue* v8_flags_opt;
    OptionValue* emer_resource_max;
    InitializeClassOptions(
        "jsobjectscriptmanager",this,
        // Default value allows us to use std libs in the build tree, starting
        // from build/cmake, bin/, or lib/sirikata/. These defaults
        // are pretty much always sane -- only override if you
        // absolutely know what you're doing
        default_import_paths = new OptionValue("default-import-paths",
            Path::Placeholders::DIR_SYSTEM_CONFIG + "/js/scripts" + "," +
            Path::Placeholders::RESOURCE(JS_PLUGINS_DIR, JS_SCRIPTS_DIR) + "," +
            Path::Placeholders::DIR_CURRENT,
            OptionValueType<std::list<String> >(),"Comma separated list of paths to import files from, searched in order for the requested import."),
        // These are additional import paths. Generally if you need to
        // adjust import paths, you want to simply add to this. We
        // split the default-import-paths and import-paths so it's
        // easier to override the paths by only adding import-paths
        // and not changing default-import-paths
        import_paths = new OptionValue("import-paths","",OptionValueType<std::list<String> >(),"Comma separated list of paths to import files from, searched in order for the requested import."),
        v8_flags_opt = new OptionValue("v8-flags", "", OptionValueType<String>(), "Flags to pass on to v8, e.g. for profiling."),
        emer_resource_max = new OptionValue("emer-resource-max","100000000",OptionValueType<int>(),"int32: how many cycles to allow to run in one pass of event loop before throwing resource error in Emerson."),
        NULL
    );

    mOptions = OptionSet::getOptions("jsobjectscriptmanager",this);
    mOptions->parse(arguments);

    String v8_flags = v8_flags_opt->as<String>();
    if (!v8_flags.empty()) {
        v8::V8::SetFlagsFromString(v8_flags.c_str(), v8_flags.size());
    }
}

/*
  EMERSON!: util
 */

void JSObjectScriptManager::createUtilTemplate(JSCtx* jsctx)
{

    v8::HandleScope handle_scope;
    jsctx->mUtilTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // An internal field holds the JSObjectScript*
    jsctx->mUtilTemplate->SetInternalFieldCount(UTIL_TEMPLATE_FIELD_COUNT);

    jsctx->mUtilTemplate->Set(JS_STRING(sqrt),v8::FunctionTemplate::New(JSUtilObj::ScriptSqrtFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(acos),v8::FunctionTemplate::New(JSUtilObj::ScriptAcosFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(asin),v8::FunctionTemplate::New(JSUtilObj::ScriptAsinFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(cos),v8::FunctionTemplate::New(JSUtilObj::ScriptCosFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(sin),v8::FunctionTemplate::New(JSUtilObj::ScriptSinFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(rand),v8::FunctionTemplate::New(JSUtilObj::ScriptRandFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(pow),v8::FunctionTemplate::New(JSUtilObj::ScriptPowFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(exp),v8::FunctionTemplate::New(JSUtilObj::ScriptExpFunction));
    jsctx->mUtilTemplate->Set(JS_STRING(abs),v8::FunctionTemplate::New(JSUtilObj::ScriptAbsFunction));

    jsctx->mUtilTemplate->Set(v8::String::New("plus"), v8::FunctionTemplate::New(JSUtilObj::ScriptPlus));
    jsctx->mUtilTemplate->Set(v8::String::New("sub"), v8::FunctionTemplate::New(JSUtilObj::ScriptMinus));
    jsctx->mUtilTemplate->Set(v8::String::New("identifier"),v8::FunctionTemplate::New(JSUtilObj::ScriptSporef));

    jsctx->mUtilTemplate->Set(v8::String::New("div"),v8::FunctionTemplate::New(JSUtilObj::ScriptDiv));
    jsctx->mUtilTemplate->Set(v8::String::New("mul"),v8::FunctionTemplate::New(JSUtilObj::ScriptMult));
    jsctx->mUtilTemplate->Set(v8::String::New("mod"),v8::FunctionTemplate::New(JSUtilObj::ScriptMod));
    jsctx->mUtilTemplate->Set(v8::String::New("equal"),v8::FunctionTemplate::New(JSUtilObj::ScriptEqual));
    jsctx->mUtilTemplate->Set(v8::String::New("Quaternion"), jsctx->mQuaternionTemplate);
    jsctx->mUtilTemplate->Set(v8::String::New("Vec3"), jsctx->mVec3Template);

    jsctx->mUtilTemplate->Set(v8::String::New("_base64Encode"), v8::FunctionTemplate::New(JSUtilObj::Base64Encode));
    jsctx->mUtilTemplate->Set(v8::String::New("_base64EncodeURL"), v8::FunctionTemplate::New(JSUtilObj::Base64EncodeURL));
    jsctx->mUtilTemplate->Set(v8::String::New("_base64Decode"), v8::FunctionTemplate::New(JSUtilObj::Base64Decode));
    jsctx->mUtilTemplate->Set(v8::String::New("_base64DecodeURL"), v8::FunctionTemplate::New(JSUtilObj::Base64DecodeURL));
}



//these templates involve vec, quat, pattern, etc.
JSCtx* JSObjectScriptManager::createJSCtx(HostedObjectPtr ho)
{
    JSCtx* jsctx =
        new JSCtx(mContext,
            Network::IOStrandPtr(
                mContext->ioService->createStrand("EmersonScript " + ho->id().toString())),
            Network::IOStrandPtr(
                mContext->ioService->createStrand("VisManager "    + ho->id().toString())),
            v8::Isolate::New());

    v8::Locker locker (jsctx->mIsolate);
    v8::Isolate::Scope iscope(jsctx->mIsolate);
    v8::HandleScope handle_scope;
    jsctx->mVec3Template = v8::Persistent<v8::FunctionTemplate>::New(CreateVec3Template());
    jsctx->mQuaternionTemplate  = v8::Persistent<v8::FunctionTemplate>::New(CreateQuaternionTemplate());

    createUtilTemplate(jsctx);
    createVisibleTemplate(jsctx);
    createTimerTemplate(jsctx);
    createJSInvokableObjectTemplate(jsctx);
    createPresenceTemplate(jsctx);
    createSystemTemplate(jsctx);
    createContextTemplate(jsctx);
    createContextGlobalTemplate(jsctx);

    return jsctx;
}



void JSObjectScriptManager::createTimerTemplate(JSCtx* jsctx)
{
    v8::HandleScope handle_scope;
    jsctx->mTimerTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    jsctx->mTimerTemplate->SetInternalFieldCount(TIMER_JSTIMER_TEMPLATE_FIELD_COUNT);

    jsctx->mTimerTemplate->Set(v8::String::New("resetTimer"),v8::FunctionTemplate::New(JSTimer::resetTimer));
    jsctx->mTimerTemplate->Set(v8::String::New("clear"),v8::FunctionTemplate::New(JSTimer::clear));
    jsctx->mTimerTemplate->Set(v8::String::New("suspend"),v8::FunctionTemplate::New(JSTimer::suspend));
    jsctx->mTimerTemplate->Set(v8::String::New("reset"),v8::FunctionTemplate::New(JSTimer::resume));
    jsctx->mTimerTemplate->Set(v8::String::New("isSuspended"),v8::FunctionTemplate::New(JSTimer::isSuspended));
    jsctx->mTimerTemplate->Set(v8::String::New("getAllData"), v8::FunctionTemplate::New(JSTimer::getAllData));
    jsctx->mTimerTemplate->Set(v8::String::New("__getType"),v8::FunctionTemplate::New(JSTimer::getType));
}



void JSObjectScriptManager::createSystemTemplate(JSCtx* jsctx)
{
    v8::HandleScope handle_scope;
    jsctx->mSystemTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    jsctx->mSystemTemplate->SetInternalFieldCount(SYSTEM_TEMPLATE_FIELD_COUNT);

    jsctx->mSystemTemplate->Set(v8::String::New("registerProxAddedHandler"),v8::FunctionTemplate::New(JSSystem::root_proxAddedHandler));
    jsctx->mSystemTemplate->Set(v8::String::New("registerProxRemovedHandler"),v8::FunctionTemplate::New(JSSystem::root_proxRemovedHandler));


    jsctx->mSystemTemplate->Set(v8::String::New("headless"),v8::FunctionTemplate::New(JSSystem::root_headless));
    jsctx->mSystemTemplate->Set(v8::String::New("__debugFileWrite"),v8::FunctionTemplate::New(JSSystem::debug_fileWrite));
    jsctx->mSystemTemplate->Set(v8::String::New("__debugFileRead"),v8::FunctionTemplate::New(JSSystem::debug_fileRead));
    jsctx->mSystemTemplate->Set(v8::String::New("sendHome"),v8::FunctionTemplate::New(JSSystem::root_sendHome));
    jsctx->mSystemTemplate->Set(v8::String::New("event"), v8::FunctionTemplate::New(JSSystem::root_event));
    jsctx->mSystemTemplate->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(JSSystem::root_timeout));
    jsctx->mSystemTemplate->Set(v8::String::New("print"), v8::FunctionTemplate::New(JSSystem::root_print));

    jsctx->mSystemTemplate->Set(v8::String::New("getAssociatedPresence"), v8::FunctionTemplate::New(JSSystem::getAssociatedPresence));


    jsctx->mSystemTemplate->Set(v8::String::New("__evalInGlobal"), v8::FunctionTemplate::New(JSSystem::evalInGlobal));
    jsctx->mSystemTemplate->Set(v8::String::New("sendSandbox"), v8::FunctionTemplate::New(JSSystem::root_sendSandbox));

    jsctx->mSystemTemplate->Set(v8::String::New("js_import"), v8::FunctionTemplate::New(JSSystem::root_jsimport));
    jsctx->mSystemTemplate->Set(v8::String::New("js_require"), v8::FunctionTemplate::New(JSSystem::root_jsrequire));

    jsctx->mSystemTemplate->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(JSSystem::sendMessageReliable));
    jsctx->mSystemTemplate->Set(v8::String::New("sendMessageUnreliable"),v8::FunctionTemplate::New(JSSystem::sendMessageUnreliable));

    jsctx->mSystemTemplate->Set(v8::String::New("import"), v8::FunctionTemplate::New(JSSystem::root_import));

    jsctx->mSystemTemplate->Set(v8::String::New("http"), v8::FunctionTemplate::New(JSSystem::root_http));

    jsctx->mSystemTemplate->Set(v8::String::New("storageBeginTransaction"),v8::FunctionTemplate::New(JSSystem::storageBeginTransaction));
    jsctx->mSystemTemplate->Set(v8::String::New("storageCommit"),v8::FunctionTemplate::New(JSSystem::storageCommit));
    jsctx->mSystemTemplate->Set(v8::String::New("storageErase"), v8::FunctionTemplate::New(JSSystem::storageErase));
    jsctx->mSystemTemplate->Set(v8::String::New("storageWrite"),v8::FunctionTemplate::New(JSSystem::storageWrite));
    jsctx->mSystemTemplate->Set(v8::String::New("storageRead"),v8::FunctionTemplate::New(JSSystem::storageRead));
    jsctx->mSystemTemplate->Set(v8::String::New("storageRangeRead"),v8::FunctionTemplate::New(JSSystem::storageRangeRead));
    jsctx->mSystemTemplate->Set(v8::String::New("storageRangeErase"),v8::FunctionTemplate::New(JSSystem::storageRangeErase));
    jsctx->mSystemTemplate->Set(v8::String::New("storageCount"),v8::FunctionTemplate::New(JSSystem::storageCount));

    jsctx->mSystemTemplate->Set(v8::String::New("setSandboxMessageCallback"),v8::FunctionTemplate::New(JSSystem::setSandboxMessageCallback));
    jsctx->mSystemTemplate->Set(v8::String::New("setPresenceMessageCallback"),v8::FunctionTemplate::New(JSSystem::setPresenceMessageCallback));

    jsctx->mSystemTemplate->Set(v8::String::New("setRestoreScript"),v8::FunctionTemplate::New(JSSystem::setRestoreScript));
    jsctx->mSystemTemplate->Set(v8::String::New("__emersonCompileString"), v8::FunctionTemplate::New(JSSystem::emersonCompileString));

    jsctx->mSystemTemplate->Set(v8::String::New("__pushEvalContextScopeDirectory"),
        v8::FunctionTemplate::New(JSSystem::pushEvalContextScopeDirectory));
    jsctx->mSystemTemplate->Set(v8::String::New("__popEvalContextScopeDirectory"),
        v8::FunctionTemplate::New(JSSystem::popEvalContextScopeDirectory));

    jsctx->mSystemTemplate->Set(v8::String::New("getUniqueToken"),
        v8::FunctionTemplate::New(JSSystem::getUniqueToken));

    jsctx->mSystemTemplate->Set(v8::String::New("createVisible"),v8::FunctionTemplate::New(JSSystem::root_createVisible));

    //check what permissions fake root is loaded with
    jsctx->mSystemTemplate->Set(v8::String::New("canSendMessage"), v8::FunctionTemplate::New(JSSystem::root_canSendMessage));
    jsctx->mSystemTemplate->Set(v8::String::New("canRecvMessage"), v8::FunctionTemplate::New(JSSystem::root_canRecvMessage));
    jsctx->mSystemTemplate->Set(v8::String::New("canProxCallback"), v8::FunctionTemplate::New(JSSystem::root_canProxCallback));
    jsctx->mSystemTemplate->Set(v8::String::New("canProxChangeQuery"), v8::FunctionTemplate::New(JSSystem::root_canProxChangeQuery));
    jsctx->mSystemTemplate->Set(v8::String::New("canImport"),v8::FunctionTemplate::New(JSSystem::root_canImport));

    jsctx->mSystemTemplate->Set(v8::String::New("canCreatePresence"), v8::FunctionTemplate::New(JSSystem::root_canCreatePres));
    jsctx->mSystemTemplate->Set(v8::String::New("canCreateEntity"), v8::FunctionTemplate::New(JSSystem::root_canCreateEnt));
    jsctx->mSystemTemplate->Set(v8::String::New("canEval"), v8::FunctionTemplate::New(JSSystem::root_canEval));

    jsctx->mSystemTemplate->Set(v8::String::New("serialize"), v8::FunctionTemplate::New(JSSystem::root_serialize));
    jsctx->mSystemTemplate->Set(v8::String::New("deserialize"), v8::FunctionTemplate::New(JSSystem::root_deserialize));

    jsctx->mSystemTemplate->Set(v8::String::New("restorePresence"), v8::FunctionTemplate::New(JSSystem::root_restorePresence));

    jsctx->mSystemTemplate->Set(v8::String::New("getVersion"),v8::FunctionTemplate::New(JSSystem::root_getVersion));

    jsctx->mSystemTemplate->Set(v8::String::New("killEntity"), v8::FunctionTemplate::New(JSSystem::root_killEntity));

    //this doesn't work now.
    jsctx->mSystemTemplate->Set(v8::String::New("create_context"),v8::FunctionTemplate::New(JSSystem::root_createContext));


    jsctx->mSystemTemplate->Set(v8::String::New("create_entity_no_space"), v8::FunctionTemplate::New(JSSystem::root_createEntityNoSpace));

    jsctx->mSystemTemplate->Set(v8::String::New("create_entity"), v8::FunctionTemplate::New(JSSystem::root_createEntity));


    jsctx->mSystemTemplate->Set(v8::String::New("onPresenceConnected"),v8::FunctionTemplate::New(JSSystem::root_onPresenceConnected));
    jsctx->mSystemTemplate->Set(v8::String::New("onPresenceDisconnected"),v8::FunctionTemplate::New(JSSystem::root_onPresenceDisconnected));


    jsctx->mSystemTemplate->Set(JS_STRING(__presence_constructor__), jsctx->mPresenceTemplate);
    jsctx->mSystemTemplate->Set(JS_STRING(__visible_constructor__), jsctx->mVisibleTemplate);

    jsctx->mSystemTemplate->Set(v8::String::New("require"), v8::FunctionTemplate::New(JSSystem::root_require));
    jsctx->mSystemTemplate->Set(v8::String::New("reset"),v8::FunctionTemplate::New(JSSystem::root_reset));
    jsctx->mSystemTemplate->Set(v8::String::New("set_script"),v8::FunctionTemplate::New(JSSystem::root_setScript));
    jsctx->mSystemTemplate->Set(v8::String::New("getScript"),v8::FunctionTemplate::New(JSSystem::root_getScript));

}


void JSObjectScriptManager::createContextTemplate(JSCtx* jsctx)
{
    v8::HandleScope handle_scope;
    // And we expose some functionality directly
    jsctx->mContextTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    // An internal field holds the JSObjectScript*
    jsctx->mContextTemplate->SetInternalFieldCount(CONTEXT_TEMPLATE_FIELD_COUNT);

    // Functions / types
    //suspend,kill,resume,execute
    jsctx->mContextTemplate->Set(v8::String::New("execute"), v8::FunctionTemplate::New(JSContext::ScriptExecute));
    jsctx->mContextTemplate->Set(v8::String::New("suspend"), v8::FunctionTemplate::New(JSContext::ScriptSuspend));
    jsctx->mContextTemplate->Set(v8::String::New("resume"), v8::FunctionTemplate::New(JSContext::ScriptResume));
    jsctx->mContextTemplate->Set(v8::String::New("clear"), v8::FunctionTemplate::New(JSContext::ScriptClear));

}


void JSObjectScriptManager::createContextGlobalTemplate(JSCtx* jsctx)
{
    v8::HandleScope handle_scope;
    // And we expose some functionality directly
    jsctx->mContextGlobalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    jsctx->mContextGlobalTemplate->SetInternalFieldCount(CONTEXT_GLOBAL_TEMPLATE_FIELD_COUNT);

    jsctx->mContextGlobalTemplate->Set(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME),jsctx->mSystemTemplate);
    jsctx->mContextGlobalTemplate->Set(v8::String::New(JSSystemNames::UTIL_OBJECT_NAME), jsctx->mUtilTemplate);

    jsctx->mContextGlobalTemplate->Set(v8::String::New("__checkResources8_8_3_1__"), v8::FunctionTemplate::New(JSGlobal::checkResources));
}



void JSObjectScriptManager::createJSInvokableObjectTemplate(JSCtx* jsctx)
{
  v8::HandleScope handle_scope;

  jsctx->mInvokableObjectTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
  jsctx->mInvokableObjectTemplate->SetInternalFieldCount(JSSIMOBJECT_TEMPLATE_FIELD_COUNT);
  jsctx->mInvokableObjectTemplate->Set(v8::String::New("invoke"), v8::FunctionTemplate::New(JSInvokableObject::invoke));
}



void JSObjectScriptManager::createVisibleTemplate(JSCtx* jsctx)
{
    v8::HandleScope handle_scope;

    jsctx->mVisibleTemplate = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());

    v8::Local<v8::Template> proto_t = jsctx->mVisibleTemplate->PrototypeTemplate();
    //these function calls are defined in JSObjects/JSVisible.hpp

    proto_t->Set(v8::String::New("__debugRef"),v8::FunctionTemplate::New(JSVisible::__debugRef));
    proto_t->Set(v8::String::New("__toString"),v8::FunctionTemplate::New(JSVisible::toString));

    proto_t->Set(v8::String::New("getPosition"),v8::FunctionTemplate::New(JSVisible::getPosition));
    proto_t->Set(v8::String::New("getVelocity"),v8::FunctionTemplate::New(JSVisible::getVelocity));
    proto_t->Set(v8::String::New("getOrientation"),v8::FunctionTemplate::New(JSVisible::getOrientation));
    proto_t->Set(v8::String::New("getOrientationVel"),v8::FunctionTemplate::New(JSVisible::getOrientationVel));
    proto_t->Set(v8::String::New("getScale"),v8::FunctionTemplate::New(JSVisible::getScale));
    proto_t->Set(v8::String::New("getMesh"),v8::FunctionTemplate::New(JSVisible::getMesh));
    proto_t->Set(v8::String::New("getPhysics"),v8::FunctionTemplate::New(JSVisible::getPhysics));
    proto_t->Set(v8::String::New("getSpaceID"),v8::FunctionTemplate::New(JSVisible::getSpace));
    proto_t->Set(v8::String::New("getVisibleID"),v8::FunctionTemplate::New(JSVisible::getOref));
    proto_t->Set(v8::String::New("getStillVisible"),v8::FunctionTemplate::New(JSVisible::getStillVisible));
    proto_t->Set(v8::String::New("checkEqual"),v8::FunctionTemplate::New(JSVisible::checkEqual));
    proto_t->Set(v8::String::New("dist"),v8::FunctionTemplate::New(JSVisible::dist));

    proto_t->Set(v8::String::New("loadMesh"),v8::FunctionTemplate::New(JSVisible::loadMesh));
    proto_t->Set(v8::String::New("meshBounds"),v8::FunctionTemplate::New(JSVisible::meshBounds));
    proto_t->Set(v8::String::New("untransformedMeshBounds"),v8::FunctionTemplate::New(JSVisible::untransformedMeshBounds));
    proto_t->Set(v8::String::New("raytrace"),v8::FunctionTemplate::New(JSVisible::raytrace));
    proto_t->Set(v8::String::New("unloadMesh"),v8::FunctionTemplate::New(JSVisible::unloadMesh));

    //animations
    proto_t->Set(v8::String::New("getAnimationList"),v8::FunctionTemplate::New(JSVisible::getAnimationList));


    proto_t->Set(v8::String::New("getAllData"), v8::FunctionTemplate::New(JSVisible::getAllData));
    proto_t->Set(v8::String::New("__getType"),v8::FunctionTemplate::New(JSVisible::getType));


    // For instance templates
    v8::Local<v8::ObjectTemplate> instance_t = jsctx->mVisibleTemplate->InstanceTemplate();
    instance_t->SetInternalFieldCount(VISIBLE_FIELD_COUNT);

}


void JSObjectScriptManager::createPresenceTemplate(JSCtx* jsctx)
{
  v8::HandleScope handle_scope;

  jsctx->mPresenceTemplate = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
  //mPresenceTemplate->SetInternalFieldCount(PRESENCE_FIELD_COUNT);

  v8::Local<v8::Template> proto_t = jsctx->mPresenceTemplate->PrototypeTemplate();

  //These are not just accessors because we need to ensure that we can deal with
  //their failure conditions.  (Have callbacks).
  proto_t->Set(v8::String::New("__toString"), v8::FunctionTemplate::New(JSPresence::toString));

  proto_t->Set(v8::String::New("getSpaceID"),v8::FunctionTemplate::New(JSPresence::getSpace));
  proto_t->Set(v8::String::New("getPresenceID"),v8::FunctionTemplate::New(JSPresence::getOref));


  //meshes
  proto_t->Set(v8::String::New("getMesh"),v8::FunctionTemplate::New(JSPresence::getMesh));
  proto_t->Set(v8::String::New("setMesh"),v8::FunctionTemplate::New(JSPresence::setMesh));

  //physics
  proto_t->Set(v8::String::New("getPhysics"),v8::FunctionTemplate::New(JSPresence::getPhysics));
  proto_t->Set(v8::String::New("setPhysics"),v8::FunctionTemplate::New(JSPresence::setPhysics));

  //positions
  proto_t->Set(v8::String::New("getPosition"),v8::FunctionTemplate::New(JSPresence::getPosition));
  proto_t->Set(v8::String::New("setPosition"),v8::FunctionTemplate::New(JSPresence::setPosition));

  proto_t->Set(v8::String::New("getIsConnected"), v8::FunctionTemplate::New(JSPresence::getIsConnected));

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



  //for restore-ability.
  proto_t->Set(v8::String::New("getAllData"),v8::FunctionTemplate::New(JSPresence::getAllData));


  // Query angle
  proto_t->Set(v8::String::New("setQuery"),v8::FunctionTemplate::New(JSPresence::setQuery));
  proto_t->Set(v8::String::New("getQuery"), v8::FunctionTemplate::New(JSPresence::getQuery));

  //set up graphics
  proto_t->Set(v8::String::New("_runSimulation"),v8::FunctionTemplate::New(JSPresence::runSimulation));

  //convert this presence object into a visible object
  proto_t->Set(v8::String::New("toVisible"),v8::FunctionTemplate::New(JSPresence::toVisible));

  proto_t->Set(v8::String::New("suspend"), v8::FunctionTemplate::New(JSPresence::pres_suspend));
  proto_t->Set(v8::String::New("resume"), v8::FunctionTemplate::New(JSPresence::pres_resume));
  proto_t->Set(v8::String::New("disconnect"), v8::FunctionTemplate::New(JSPresence::pres_disconnect));


  proto_t->Set(v8::String::New("loadMesh"),v8::FunctionTemplate::New(JSPresence::loadMesh));
  proto_t->Set(v8::String::New("meshBounds"),v8::FunctionTemplate::New(JSPresence::meshBounds));
  proto_t->Set(v8::String::New("untransformedMeshBounds"),v8::FunctionTemplate::New(JSPresence::untransformedMeshBounds));
  proto_t->Set(v8::String::New("raytrace"),v8::FunctionTemplate::New(JSPresence::raytrace));
  proto_t->Set(v8::String::New("unloadMesh"),v8::FunctionTemplate::New(JSPresence::unloadMesh));

  //animations
  proto_t->Set(v8::String::New("getAnimationList"),v8::FunctionTemplate::New(JSPresence::getAnimationList));

  // For instance templates
  v8::Local<v8::ObjectTemplate> instance_t = jsctx->mPresenceTemplate->InstanceTemplate();
  instance_t->SetInternalFieldCount(PRESENCE_FIELD_COUNT);
}


void JSObjectScriptManager::loadMesh(const Transfer::URI& uri, MeshLoadCallback cb) {
    // First try to grab out of cache
    MeshCache::iterator it = mMeshCache.find(uri);
    if (it != mMeshCache.end()) {
        VisualWPtr w_mesh = it->second;
        VisualPtr mesh = w_mesh.lock();
        if (mesh) {
            // Ideally we shouldn't get here if we already shutdown, but check
            // just in case
            if (mContext->stopped()) {
                JSLOG(warn, "Load mesh called after shutdown request, ignoring successful callback...");
                return;
            }
            mContext->mainStrand->post(std::tr1::bind(cb, mesh), "JSObjectScriptManager::loadMesh");
            return;
        }
    }

    // Always add the callback to the list to be invoked, whether
    // we're the first requester or not
    mMeshCallbacks[uri].push_back(cb);

    // Even if we don't have the VisualPtr, the load might be in progress. In
    // that case, we just queue up the callback.
    if (mMeshDownloads.find(uri) != mMeshDownloads.end())
        return;

    // Finally, if none of that worked, we need to download it.
    Transfer::ResourceDownloadTaskPtr dl = Transfer::ResourceDownloadTask::construct(
        uri,
        mTransferPool,
        1.0,
        std::tr1::bind(&JSObjectScriptManager::meshDownloaded, this, _1, _2, _3)
    );
    mMeshDownloads[uri] = dl;
    dl->start();
}

void JSObjectScriptManager::meshDownloaded(Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request, Transfer::DenseDataPtr data) {
    Transfer::ChunkRequestPtr chunkreq = std::tr1::static_pointer_cast<Transfer::ChunkRequest>(request);
    mParsingIOService->post(
        std::tr1::bind(&JSObjectScriptManager::parseMeshWork, this, chunkreq->getMetadata(), chunkreq->getMetadata().getFingerprint(), data),
        "JSObjectScriptManager::parseMeshWork"
    );
}

void JSObjectScriptManager::parseMeshWork(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data) {
    Mesh::VisualPtr parsed = mModelParser->load(metadata, fp, data);
    if (parsed && mModelFilter) {
        Mesh::MutableFilterDataPtr input_data(new Mesh::FilterData);
        input_data->push_back(parsed);
        Mesh::FilterDataPtr output_data = mModelFilter->apply(input_data);
        assert(output_data->single());
        parsed = output_data->get();
    }

    mContext->mainStrand->post(
        std::tr1::bind(&JSObjectScriptManager::finishMeshDownload, this, metadata.getURI(), parsed),
        "JSObjectScriptManager::finishMeshDownload"
    );
}

void JSObjectScriptManager::finishMeshDownload(const Transfer::URI& uri, VisualPtr mesh) {
    // We need to clean up and invoke callbacks. Make sure we're fully cleaned
    // up (out of member data) before making callbacks in case they do any
    // re-requests.
    mMeshCache[uri] = mesh;
    mMeshDownloads.erase(uri);
    MeshLoadCallbackList cbs = mMeshCallbacks[uri];
    mMeshCallbacks.erase(uri);

    for(MeshLoadCallbackList::iterator it = cbs.begin(); it != cbs.end(); it++)
        mContext->mainStrand->post(
            std::tr1::bind(*it, mesh),
            "JSObjectScriptManager::finishMeshDownload"
        );
}

JSObjectScriptManager::~JSObjectScriptManager()
{
    if (mContext != NULL) {
        // These only allocated if we're not headless.

        mParsingThread->join();
        delete mParsingThread;
        delete mParsingIOService;

        delete mModelFilter;
        delete mModelParser;
    }
}



JSObjectScript* JSObjectScriptManager::createHeadless(const String& args, const String& script,int32 maxres)
{
    JSLOG(error, "Cannot run emheadless without providing a context from which to get strand.");
    assert(false);
    JSObjectScript* new_script =
        new JSObjectScript(this, NULL, NULL, UUID::random(),NULL);

    new_script->initialize(args, script, maxres);
    return new_script;
}

ObjectScript* JSObjectScriptManager::createObjectScript(
    HostedObjectPtr ho, const String& args, const String& script)
{
    JSCtx* jsctx =createJSCtx(ho);


    EmersonScript* new_script =new EmersonScript(
        ho, args, script, this,jsctx);


    if (!new_script->valid()) {
        delete new_script;
        return NULL;
    }
    return new_script;
}

void JSObjectScriptManager::destroyObjectScript(ObjectScript*toDestroy){
    delete toDestroy;
}


} // namespace JS
} // namespace JS
