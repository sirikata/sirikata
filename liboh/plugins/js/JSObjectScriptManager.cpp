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
#include "JSObjects/JSHandler.hpp"
#include "JSObjects/JSTimer.hpp"
#include "JSSerializer.hpp"
#include "JSPattern.hpp"

#include "JS_JSMessage.pbj.hpp"
#include "JSObjects/JSPresence.hpp"
#include "JSObjects/JSGlobal.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/JSInvokableObject.hpp"
#include "JSSystemNames.hpp"
#include "JSObjects/JSContext.hpp"

#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <sirikata/mesh/CompositeFilter.hpp>

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
        mTransferPool = Transfer::TransferMediator::getSingleton().registerClient("JSObjectScriptManager");

        mParsingIOService = Network::IOServiceFactory::makeIOService();
        mParsingWork = new Network::IOWork(*mParsingIOService, "JSObjectScriptManager Mesh Parsing");
        mParsingThread = new Sirikata::Thread(std::tr1::bind(&Network::IOService::runNoReturn, mParsingIOService));

        mModelParser = ModelsSystemFactory::getSingleton ().getConstructor ( "any" ) ( "" );
        try {
            // These have to be consistent with any other simulations -- e.g. the
            // space bullet plugin and scripting plugins that expose mesh data
            std::vector<String> names_and_args;
            names_and_args.push_back("center"); names_and_args.push_back("");
            mModelFilter = new Mesh::CompositeFilter(names_and_args);
        }
        catch(Mesh::CompositeFilter::Exception e) {
            SILOG(js,warning,"Couldn't allocate requested model load filter, will not apply filter to loaded models.");
            mModelFilter = NULL;
        }
    }

    OptionValue* import_paths;
    OptionValue* v8_flags_opt;
    OptionValue* emer_resource_max;
    InitializeClassOptions(
        "jsobjectscriptmanager",this,
        // Default value allows us to use std libs in the build tree, starting
        // from build/cmake
        import_paths = new OptionValue("import-paths","../../liboh/plugins/js/scripts,../share/js/scripts",OptionValueType<std::list<String> >(),"Comma separated list of paths to import files from, searched in order for the requested import."),
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

    createTemplates(); //these templates involve vec, quat, pattern, etc.
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

    mUtilTemplate->Set(JS_STRING(sqrt),v8::FunctionTemplate::New(JSUtilObj::ScriptSqrtFunction));
    mUtilTemplate->Set(JS_STRING(acos),v8::FunctionTemplate::New(JSUtilObj::ScriptAcosFunction));
    mUtilTemplate->Set(JS_STRING(asin),v8::FunctionTemplate::New(JSUtilObj::ScriptAsinFunction));
    mUtilTemplate->Set(JS_STRING(cos),v8::FunctionTemplate::New(JSUtilObj::ScriptCosFunction));
    mUtilTemplate->Set(JS_STRING(sin),v8::FunctionTemplate::New(JSUtilObj::ScriptSinFunction));
    mUtilTemplate->Set(JS_STRING(rand),v8::FunctionTemplate::New(JSUtilObj::ScriptRandFunction));
    mUtilTemplate->Set(JS_STRING(pow),v8::FunctionTemplate::New(JSUtilObj::ScriptPowFunction));
    mUtilTemplate->Set(JS_STRING(exp),v8::FunctionTemplate::New(JSUtilObj::ScriptExpFunction));
    mUtilTemplate->Set(JS_STRING(abs),v8::FunctionTemplate::New(JSUtilObj::ScriptAbsFunction));

    mUtilTemplate->Set(v8::String::New("plus"), v8::FunctionTemplate::New(JSUtilObj::ScriptPlus));
    mUtilTemplate->Set(v8::String::New("minus"), v8::FunctionTemplate::New(JSUtilObj::ScriptMinus));

    mUtilTemplate->Set(v8::String::New("Pattern"), mPatternTemplate);
    mUtilTemplate->Set(v8::String::New("Quaternion"), mQuaternionTemplate);
    mUtilTemplate->Set(v8::String::New("Vec3"), mVec3Template);
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

    createUtilTemplate();


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
    mTimerTemplate->Set(v8::String::New("getAllData"), v8::FunctionTemplate::New(JSTimer::getAllData));
    mTimerTemplate->Set(v8::String::New("__getType"),v8::FunctionTemplate::New(JSTimer::getType));
}


/*
  EMERSON!: system
 */
void JSObjectScriptManager::createSystemTemplate()
{
    v8::HandleScope handle_scope;
    mSystemTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());

    mSystemTemplate->SetInternalFieldCount(SYSTEM_TEMPLATE_FIELD_COUNT);

    mSystemTemplate->Set(v8::String::New("registerProxAddedHandler"),v8::FunctionTemplate::New(JSSystem::root_proxAddedHandler));
    mSystemTemplate->Set(v8::String::New("registerProxRemovedHandler"),v8::FunctionTemplate::New(JSSystem::root_proxRemovedHandler));


    mSystemTemplate->Set(v8::String::New("headless"),v8::FunctionTemplate::New(JSSystem::root_headless));
    mSystemTemplate->Set(v8::String::New("__debugFileWrite"),v8::FunctionTemplate::New(JSSystem::debug_fileWrite));
    mSystemTemplate->Set(v8::String::New("__debugFileRead"),v8::FunctionTemplate::New(JSSystem::debug_fileRead));
    mSystemTemplate->Set(v8::String::New("sendHome"),v8::FunctionTemplate::New(JSSystem::root_sendHome));
    mSystemTemplate->Set(v8::String::New("registerHandler"),v8::FunctionTemplate::New(JSSystem::root_registerHandler));
    mSystemTemplate->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(JSSystem::root_timeout));
    mSystemTemplate->Set(v8::String::New("print"), v8::FunctionTemplate::New(JSSystem::root_print));

    mSystemTemplate->Set(v8::String::New("import"), v8::FunctionTemplate::New(JSSystem::root_import));

    mSystemTemplate->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(JSSystem::sendMessage));

    mSystemTemplate->Set(v8::String::New("import"), v8::FunctionTemplate::New(JSSystem::root_import));


    mSystemTemplate->Set(v8::String::New("storageBeginTransaction"),v8::FunctionTemplate::New(JSSystem::storageBeginTransaction));
    mSystemTemplate->Set(v8::String::New("storageCommit"),v8::FunctionTemplate::New(JSSystem::storageCommit));
    mSystemTemplate->Set(v8::String::New("storageErase"), v8::FunctionTemplate::New(JSSystem::storageErase));
    mSystemTemplate->Set(v8::String::New("storageWrite"),v8::FunctionTemplate::New(JSSystem::storageWrite));
    mSystemTemplate->Set(v8::String::New("storageRead"),v8::FunctionTemplate::New(JSSystem::storageRead));


    mSystemTemplate->Set(v8::String::New("setRestoreScript"),v8::FunctionTemplate::New(JSSystem::setRestoreScript));



    mSystemTemplate->Set(v8::String::New("createVisible"),v8::FunctionTemplate::New(JSSystem::root_createVisible));

    //check what permissions fake root is loaded with
    mSystemTemplate->Set(v8::String::New("canSendMessage"), v8::FunctionTemplate::New(JSSystem::root_canSendMessage));
    mSystemTemplate->Set(v8::String::New("canRecvMessage"), v8::FunctionTemplate::New(JSSystem::root_canRecvMessage));
    mSystemTemplate->Set(v8::String::New("canProx"), v8::FunctionTemplate::New(JSSystem::root_canProx));
    mSystemTemplate->Set(v8::String::New("canImport"),v8::FunctionTemplate::New(JSSystem::root_canImport));

    mSystemTemplate->Set(v8::String::New("canCreatePresence"), v8::FunctionTemplate::New(JSSystem::root_canCreatePres));
    mSystemTemplate->Set(v8::String::New("canCreateEntity"), v8::FunctionTemplate::New(JSSystem::root_canCreateEnt));
    mSystemTemplate->Set(v8::String::New("canEval"), v8::FunctionTemplate::New(JSSystem::root_canEval));

    mSystemTemplate->Set(v8::String::New("serialize"), v8::FunctionTemplate::New(JSSystem::root_serialize));
    mSystemTemplate->Set(v8::String::New("deserialize"), v8::FunctionTemplate::New(JSSystem::root_deserialize));

    mSystemTemplate->Set(v8::String::New("restorePresence"), v8::FunctionTemplate::New(JSSystem::root_restorePresence));


    mSystemTemplate->Set(v8::String::New("getPosition"), v8::FunctionTemplate::New(JSSystem::root_getPosition));
    mSystemTemplate->Set(v8::String::New("getVersion"),v8::FunctionTemplate::New(JSSystem::root_getVersion));

    mSystemTemplate->Set(v8::String::New("killEntity"), v8::FunctionTemplate::New(JSSystem::root_killEntity));

    //this doesn't work now.
    mSystemTemplate->Set(v8::String::New("eval"), v8::FunctionTemplate::New(JSSystem::root_scriptEval));
    mSystemTemplate->Set(v8::String::New("create_context"),v8::FunctionTemplate::New(JSSystem::root_createContext));
    mSystemTemplate->Set(v8::String::New("create_presence"), v8::FunctionTemplate::New(JSSystem::root_createPresence));


    mSystemTemplate->Set(v8::String::New("create_entity_no_space"), v8::FunctionTemplate::New(JSSystem::root_createEntityNoSpace));

    mSystemTemplate->Set(v8::String::New("create_entity"), v8::FunctionTemplate::New(JSSystem::root_createEntity));


    mSystemTemplate->Set(v8::String::New("onPresenceConnected"),v8::FunctionTemplate::New(JSSystem::root_onPresenceConnected));
    mSystemTemplate->Set(v8::String::New("onPresenceDisconnected"),v8::FunctionTemplate::New(JSSystem::root_onPresenceDisconnected));


    mSystemTemplate->Set(JS_STRING(__presence_constructor__), mPresenceTemplate);
    mSystemTemplate->Set(JS_STRING(__visible_constructor__), mVisibleTemplate);

    mSystemTemplate->Set(v8::String::New("require"), v8::FunctionTemplate::New(JSSystem::root_require));
    mSystemTemplate->Set(v8::String::New("reset"),v8::FunctionTemplate::New(JSSystem::root_reset));
    mSystemTemplate->Set(v8::String::New("set_script"),v8::FunctionTemplate::New(JSSystem::root_setScript));
    mSystemTemplate->Set(v8::String::New("getScript"),v8::FunctionTemplate::New(JSSystem::root_getScript));

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
    mContextGlobalTemplate->SetInternalFieldCount(CONTEXT_GLOBAL_TEMPLATE_FIELD_COUNT);

    mContextGlobalTemplate->Set(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME),mSystemTemplate);
    mContextGlobalTemplate->Set(v8::String::New(JSSystemNames::UTIL_OBJECT_NAME), mUtilTemplate);

    mContextGlobalTemplate->Set(v8::String::New("__checkResources8_8_3_1__"), v8::FunctionTemplate::New(JSGlobal::checkResources));
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

    mVisibleTemplate = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());

    v8::Local<v8::Template> proto_t = mVisibleTemplate->PrototypeTemplate();
    //these function calls are defined in JSObjects/JSVisible.hpp

    proto_t->Set(v8::String::New("__debugRef"),v8::FunctionTemplate::New(JSVisible::__debugRef));
    proto_t->Set(v8::String::New("toString"),v8::FunctionTemplate::New(JSVisible::toString));

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

    proto_t->Set(v8::String::New("getAllData"), v8::FunctionTemplate::New(JSVisible::getAllData));
    proto_t->Set(v8::String::New("__getType"),v8::FunctionTemplate::New(JSVisible::getType));


    // For instance templates
    v8::Local<v8::ObjectTemplate> instance_t = mVisibleTemplate->InstanceTemplate();
    instance_t->SetInternalFieldCount(VISIBLE_FIELD_COUNT);

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
  proto_t->Set(v8::String::New("setQueryAngle"),v8::FunctionTemplate::New(JSPresence::setQueryAngle));
  proto_t->Set(v8::String::New("getQueryAngle"), v8::FunctionTemplate::New(JSPresence::getQueryAngle));

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
    mHandlerTemplate->Set(v8::String::New("getAllData"),v8::FunctionTemplate::New(JSHandler::getAllData));
}


void JSObjectScriptManager::loadMesh(const Transfer::URI& uri, MeshLoadCallback cb) {
    // First try to grab out of cache
    MeshCache::iterator it = mMeshCache.find(uri);
    if (it != mMeshCache.end()) {
        MeshdataWPtr w_mesh = it->second;
        MeshdataPtr mesh = w_mesh.lock();
        if (mesh) {
            mContext->mainStrand->post(std::tr1::bind(cb, mesh));
            return;
        }
    }

    // Always add the callback to the list to be invoked, whether
    // we're the first requester or not
    mMeshCallbacks[uri].push_back(cb);

    // Even if we don't have the MeshdataPtr, the load might be in progress. In
    // that case, we just queue up the callback.
    if (mMeshDownloads.find(uri) != mMeshDownloads.end())
        return;

    // Finally, if none of that worked, we need to download it.
    Transfer::ResourceDownloadTaskPtr dl = Transfer::ResourceDownloadTask::construct(
        uri,
        mTransferPool,
        1.0,
        std::tr1::bind(&JSObjectScriptManager::meshDownloaded, this, _1, _2)
    );
    mMeshDownloads[uri] = dl;
    dl->start();
}

void JSObjectScriptManager::meshDownloaded(Transfer::ChunkRequestPtr request, Transfer::DenseDataPtr data) {
    mParsingIOService->post(
        std::tr1::bind(&JSObjectScriptManager::parseMeshWork, this, request->getMetadata().getURI(), request->getMetadata().getFingerprint(), data)
    );
}

void JSObjectScriptManager::parseMeshWork(const Transfer::URI& uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data) {
    Mesh::MeshdataPtr parsed = mModelParser->load(uri, fp, data);
    if (parsed && mModelFilter) {
        Mesh::MutableFilterDataPtr input_data(new Mesh::FilterData);
        input_data->push_back(parsed);
        Mesh::FilterDataPtr output_data = mModelFilter->apply(input_data);
        assert(output_data->single());
        parsed = output_data->get();
    }

    mContext->mainStrand->post(std::tr1::bind(&JSObjectScriptManager::finishMeshDownload, this, uri, parsed));
}

void JSObjectScriptManager::finishMeshDownload(const Transfer::URI& uri, MeshdataPtr mesh) {
    // We need to clean up and invoke callbacks. Make sure we're fully cleaned
    // up (out of member data) before making callbacks in case they do any
    // re-requests.
    mMeshCache[uri] = mesh;
    mMeshDownloads.erase(uri);
    MeshLoadCallbackList cbs = mMeshCallbacks[uri];
    mMeshCallbacks.erase(uri);

    for(MeshLoadCallbackList::iterator it = cbs.begin(); it != cbs.end(); it++)
        mContext->mainStrand->post(std::tr1::bind(*it, mesh));
}

JSObjectScriptManager::~JSObjectScriptManager()
{
    mParsingThread->join();
    delete mParsingThread;
    Network::IOServiceFactory::destroyIOService(mParsingIOService);

    delete mModelFilter;
    delete mModelParser;
}



JSObjectScript* JSObjectScriptManager::createHeadless(const String& args, const String& script,int32 maxres)
{
    JSObjectScript* new_script = new JSObjectScript(this, NULL, NULL, UUID::random());
    new_script->initialize(args, script, maxres);
    return new_script;
}

ObjectScript* JSObjectScriptManager::createObjectScript(HostedObjectPtr ho, const String& args, const String& script)
{
    EmersonScript* new_script = new EmersonScript(ho, args, script, this);
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
