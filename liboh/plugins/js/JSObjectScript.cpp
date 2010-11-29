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

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>

#include <sirikata/core/odp/Defs.hpp>
#include <vector>
#include <set>
#include "JSObjects/JSFields.hpp"
#include "JS_JSMessage.pbj.hpp"
#include "emerson/EmersonUtil.h"
#include "JSSystemNames.hpp"
#include "JSPresenceStruct.hpp"

//#define __EMERSON_COMPILE_ON__


#define FIXME_GET_SPACE_OREF() \
    HostedObject::SpaceObjRefSet spaceobjrefs;              \
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
        result = cb->Call(ctx->Global(), argc, argv);
    else
        result = cb->Call(target, argc, argv);

    if (result.IsEmpty()) {
        // FIXME what should we do with this exception?
        v8::String::Utf8Value error(try_catch.Exception());
        const char* cMsg = ToCString(error);
        std::cout << cMsg << "\n";
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
    Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::ROOT_OBJECT_NAME)));
    populateSystemObject(system_obj);
    
    mHandlingEvent = false;

    // If we have a script to load, load it.
    String script_name = init_script->as<String>();
    if (!script_name.empty())
        import(script_name);

    // Subscribe for session events
    mParent->addListener((SessionEventListener*)this);
    // And notify the script of existing ones
    HostedObject::SpaceObjRefSet spaceobjrefs;
    mParent->getSpaceObjRefs(spaceobjrefs);
    if (spaceobjrefs.size() > 1)
        JSLOG(fatal,"Error: Connected to more than one space.  Only enabling scripting for one space.");
    for(HostedObject::SpaceObjRefSet::const_iterator space_it = spaceobjrefs.begin(); space_it != spaceobjrefs.end(); space_it++)
        onConnected(mParent, *space_it);

    mParent->getObjectHost()->persistEntityState(String("scene.persist"));
}

void JSObjectScript::onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name) {
    //register for scripting messages from user
    SpaceID space_id = name.space();
    ObjectReference obj_refer = name.object();

    mScriptingPort = mParent->bindODPPort(space_id, obj_refer, Services::SCRIPTING);
    if (mScriptingPort)
        mScriptingPort->receive( std::tr1::bind(&JSObjectScript::handleScriptingMessageNewProto, this, _1, _2, _3));

    //register port for messaging
    mMessagingPort = mParent->bindODPPort(space_id, obj_refer, Services::COMMUNICATION);
    if (mMessagingPort)
        mMessagingPort->receive( std::tr1::bind(&JSObjectScript::handleCommunicationMessageNewProto, this, _1, _2, _3));

    mCreateEntityPort = mParent->bindODPPort(space_id,obj_refer, Services::CREATE_ENTITY);

    // Add to system.presences array
    v8::Handle<v8::Object> new_pres = addPresence(name);

    // Invoke user callback
    if ( !mOnPresenceConnectedHandler.IsEmpty() && !mOnPresenceConnectedHandler->IsUndefined() && !mOnPresenceConnectedHandler->IsNull() ) {
        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { new_pres };
        ProtectedJSCallback(mContext, v8::Handle<Object>::Cast(v8::Undefined()), mOnPresenceConnectedHandler, argc, argv);
    }
}

void JSObjectScript::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name) {
    // Remove from system.presences array
    removePresence(name);

    // FIXME this should get the presence but its already been deleted
    if ( !mOnPresenceConnectedHandler.IsEmpty() && !mOnPresenceDisconnectedHandler->IsUndefined() && !mOnPresenceDisconnectedHandler->IsNull() )
        ProtectedJSCallback(mContext, v8::Handle<Object>::Cast(v8::Undefined()), mOnPresenceDisconnectedHandler);
}


void JSObjectScript::runSimulation(const SpaceObjectReference& sporef, const String& simname)
{
    mParent->runSimulation(sporef,simname);
}

void JSObjectScript::create_entity(Vector3d& vec, String& script_name, String& mesh_name)
{

  //float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

  // get the script type
  // String script_type = "js";
  // Sirikata::Protocol::CreateObject creator;
  // Sirikata::Protocol::IConnectToSpace spacer = creator.add_space_properties();
  // Sirikata::Protocol::IObjLoc loc = spacer.mutable_requested_object_loc();

  // loc.set_position(vec);
  // loc.set_velocity(Vector3f(0,0,0));
  // loc.set_angular_speed(0);
  // loc.set_rotational_axis(Vector3f(1,0,0));

  // creator.set_mesh(mesh_name);
  // creator.set_scale(Vector3f(1,1,1));
  // creator.set_script(script_type);
  // creator.set_script_name(script_name);

  // std::string serializedCreate;
  // creator.SerializeToString(&serializedCreate);

  // FIXME_GET_SPACE_OREF();

  // ODP::Endpoint dest (space,oref,Services::CREATE_ENTITY);
  // //mCreateEntityPort->send(dest, MemoryReference(serialized.data(),
  // //serialized.length()));
  // mCreateEntityPort->send(dest, MemoryReference(serializedCreate));

    assert(false);
}


void JSObjectScript::reboot()
{
  // Need to delete the existing context? v8 garbage collects?

  v8::HandleScope handle_scope;
  mContext = v8::Context::New(NULL, mManager->mGlobalTemplate);
  Local<Object> global_obj = mContext->Global();
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  global_proto->SetInternalField(0, External::New(this));
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::ROOT_OBJECT_NAME)));

  populateSystemObject(system_obj);

  //delete all handlers
  for (int s=0; s < (int) mEventHandlers.size(); ++s)
      delete mEventHandlers[s];

  mEventHandlers.clear();

}

void JSObjectScript::debugPrintString(std::string cStrMsgBody) const
{
    std::cout<<"\n\n\n\nIs it working:  "<<cStrMsgBody<<"\n\n";
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



void JSObjectScript::test() const
{
    testSendMessageBroadcast("default message");
}


//bftm
//populates the internal addressable object references vector
// void JSObjectScript::populateAddressable(Handle<Object>& system_obj )
// {
//     //loading the vector
//     mAddressableList.clear();
//     getAllMessageable(mAddressableList);

//     v8::Context::Scope context_scope(mContext);
//     v8::Local<v8::Array> arrayObj= v8::Array::New();
//     system_obj->Set(v8::String::New(JSSystemNames::ADDRESSABLE_ARRAY_NAME),arrayObj);

//     // No work to do if we have no presences.
//     if (mPresences.empty()) return;

// //     v8::Context::Scope context_scope(mContext);
// //     v8::Local<v8::Array> arrayObj= v8::Array::New();


// //     Right now, we have multiple presences, but only designate one as "self"
// //     should we have multiple selves as well?
// //     FIXME_GET_SPACE_OREF();
// //     SpaceObjectReference mSporef = SpaceObjectReference(space,oref);

// //     for (int s=0;s < (int)mAddressableList.size(); ++s)
// //     {
// //         Local<Object> tmpObj = mManager->mAddressableTemplate->NewInstance();

// //         tmpObj->SetInternalField(ADDRESSABLE_JSOBJSCRIPT_FIELD,External::New(this));
// //         tmpObj->SetInternalField(ADDRESSABLE_SPACEOBJREF_FIELD,External::New(mAddressableList[s]));
// //         arrayObj->Set(v8::Number::New(s),tmpObj);

// //         if((*mAddressableList[s]) == mSporef)
// //             system_obj->Set(v8::String::New(JSSystemNames::ADDRESSABLE_SELF_NAME), tmpObj);
// //     }
// //     system_obj->Set(v8::String::New(JSSystemNames::ADDRESSABLE_ARRAY_NAME),arrayObj);
// // }
// }



int JSObjectScript::getAddressableSize()
{
    return (int)mAddressableList.size();
}



//this function sends a message to all functions that proxyObjectManager has an
//ObjectReference for
void JSObjectScript::testSendMessageBroadcast(const std::string& msgToBCast) const
{
    Sirikata::JS::Protocol::JSMessage js_object_msg;
    Sirikata::JS::Protocol::IJSField js_object_field = js_object_msg.add_fields();

    for (int s=0; s < (int)mAddressableList.size(); ++s)
        sendMessageToEntity(mAddressableList[s],msgToBCast);
}



//takes in the index into the mAddressableList (for the object reference) and
//a string that forms the message body.
void JSObjectScript::sendMessageToEntity(int numIndex, const std::string& msgBody) const
{
    sendMessageToEntity(mAddressableList[numIndex],msgBody);
}

//void JSObjectScript::sendMessageToEntity(ObjectReference* reffer, const
//std::string& msgBody) const
void JSObjectScript::sendMessageToEntity(SpaceObjectReference* sporef, const std::string& msgBody) const
{
    ODP::Endpoint dest (sporef->space(),sporef->object(),Services::COMMUNICATION);
    MemoryReference toSend(msgBody);

    mMessagingPort->send(dest,toSend);
}


v8::Handle<v8::Value> JSObjectScript::protectedEval(const String& em_script_str, const EvalContext& new_ctx)
{
    ScopedEvalContext sec(this, new_ctx);

    v8::Context::Scope context_scope(mContext);
    v8::HandleScope handle_scope;
    TryCatch try_catch;


    // Special casing emerson compilation


    #ifdef __EMERSON_COMPILE_ON__

    String em_script_str_new = em_script_str;

    if(em_script_str.at(em_script_str.size() -1) != '\n')
    {
        em_script_str_new.push_back('\n');
    }

    emerson_init();
    String js_script_str = string(emerson_compile(em_script_str_new.c_str()));
    cout << " js script = \n" <<js_script_str << "\n";

    v8::Handle<v8::String> source = v8::String::New(js_script_str.c_str(), js_script_str.size());
    #else

    // assume the input string to be a valid js rather than emerson
    v8::Handle<v8::String> source = v8::String::New(em_script_str.c_str(), em_script_str.size());

    #endif



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
        JSLOG(info, "Script result: " << *ascii);
    }

    return result;
}


//this function gets a list of all objects that are messageable
void JSObjectScript::getAllMessageable(AddressableList&allAvailableObjectReferences) const
{
    allAvailableObjectReferences.clear();

    HostedObject::SpaceObjRefSet allSporefs;
    mParent->getSpaceObjRefs(allSporefs);


    for (HostedObject::SpaceObjRefSet::iterator sporefIt = allSporefs.begin(); sporefIt != allSporefs.end(); ++ sporefIt)
    {
        ProxyManagerPtr proxManagerPtr = mParent->getProxyManager(sporefIt->space(),sporefIt->object());
        proxManagerPtr->getAllObjectReferences(allAvailableObjectReferences);
    }


    if (allAvailableObjectReferences.empty())
        printf("\n\nBFTM: No object references available for sending messages");
}


//this function runs through 
void JSObjectScript::createMessageableArray()
{
    AddressableList allAvailableObjectReferences;

    HostedObject::SpaceObjRefSet allSporefs;
    mParent->getSpaceObjRefs(allSporefs);

    for (HostedObject::SpaceObjRefSet::iterator sporefIt = allSporefs.begin(); sporefIt != allSporefs.end(); ++ sporefIt)
    {
        ProxyManagerPtr proxManagerPtr = mParent->getProxyManager(sporefIt->space(),sporefIt->object());
        proxManagerPtr->getAllObjectReferences(mAddressableList);
        proxManagerPtr->addListener(this);
    }
}


//bftm
//populates the js addressable array
//createMessageableArray should have been called before this;
void JSObjectScript::populateAddressable(Handle<Object>& system_obj )
{
    //loading the vector

    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Array> arrayObj= v8::Array::New();

    //Right now, we have multiple presences, but only designate one as "self"
    //should we have multiple selves as well?
    FIXME_GET_SPACE_OREF();
    SpaceObjectReference mSporef = SpaceObjectReference(space,oref);

    for (int s=0;s < (int)mAddressableList.size(); ++s)
    {
        Local<Object> tmpObj = mManager->mAddressableTemplate->NewInstance();

        tmpObj->SetInternalField(ADDRESSABLE_JSOBJSCRIPT_FIELD,External::New(this));
        tmpObj->SetInternalField(ADDRESSABLE_SPACEOBJREF_FIELD,External::New(mAddressableList[s]));

        arrayObj->Set(v8::Number::New(s),tmpObj);

        if((*mAddressableList[s]) == mSporef)
            system_obj->Set(v8::String::New(JSSystemNames::ADDRESSABLE_SELF_NAME), tmpObj);

    }
    system_obj->Set(v8::String::New(JSSystemNames::ADDRESSABLE_ARRAY_NAME),arrayObj);
}


void JSObjectScript::onCreateProxy(ProxyObjectPtr p)
{
    std::cout<<"\n\nIssue: likely incorrect behavior in onCreateProxy: need to keep track of actual system object rather than create a new one.\n\n";
    mAddressableList.push_back(new SpaceObjectReference(p->getObjectReference()));
    HandleScope handle_scope;
    Handle<Object> system_obj = getSystemObject();
    populateAddressable(system_obj);
}

void JSObjectScript::onDestroyProxy(ProxyObjectPtr p)
{
    JSLOG(info,"Ignoring destruction of proxy.");
}


void JSObjectScript::timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb)
{
    // FIXME using the raw pointer isn't safe
    FIXME_GET_SPACE_OREF();

    Network::IOService* ioserve = mParent->getIOService();

    ioserve->post(
        dur,
        std::tr1::bind(&JSObjectScript::handleTimeout,
            this,
            target,
            cb
        ));


}

void JSObjectScript::handleTimeout(v8::Persistent<v8::Object> target, v8::Persistent<v8::Function> cb) {
    ProtectedJSCallback(mContext, target, cb);
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
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Couldn't find file for import.")) );

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

    EvalContext new_ctx;
    new_ctx.currentScriptDir = full_filename.parent_path().string();
    return protectedEval(contents, new_ctx);
}



//position


// need to ensure that the sender object is an addressable of type spaceobject reference rather than just having an object reference;
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


/*
 * Populates the message properties
 */




v8::Local<v8::Object> JSObjectScript::getMessageSender(const ODP::Endpoint& src)
{

    SpaceObjectReference* sporef = new SpaceObjectReference(src.space(),src.object());

    Local<Object> tmpObj = mManager->mAddressableTemplate->NewInstance();
    tmpObj->SetInternalField(ADDRESSABLE_JSOBJSCRIPT_FIELD,External::New(this));
    tmpObj->SetInternalField(ADDRESSABLE_SPACEOBJREF_FIELD,External::New(sporef));
    return tmpObj;
}


void JSObjectScript::handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Object> obj = v8::Object::New();


    std::cout<<"\n\nComm: dst space: "<<dst.space()<<"\n\n";

    v8::Local<v8::Object> msgSender = getMessageSender(src);
    //try deserialization

    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromArray(payload.data(), payload.size());

//bftm:clean all this up later

    if (! parsed)
    {
        std::cout<<"\n\nCannot parse the message that I received on this port\n\n";
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
    if (!matchesSomeHandler)
        std::cout<<"\n\nMessage did not match any files\n\n";
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
    if (!parsed)
    {
        JSLOG(fatal, "Parsing failed.");
    }
    else
    {
        // Handle all requests
        for(int32 ii = 0; ii < scripting_msg.requests_size(); ii++)
        {
            Sirikata::JS::Protocol::ScriptingRequest req = scripting_msg.requests(ii);
            String script_str = req.body();

            assert(!mEvalContextStack.empty());
            // No new context info currently, just copy the previous one
            protectedEval(script_str, mEvalContextStack.top());
        }
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
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::ROOT_OBJECT_NAME)));

  Persistent<Object> ret_obj = Persistent<Object>::New(system_obj);
  return ret_obj;
}


void JSObjectScript::updateAddressable()
{

}

// void JSObjectScript::updateAddressable()
// {
//   HandleScope handle_scope;
//   Handle<Object> system_obj = getSystemObject();
//   populateAddressable(system_obj);
// }


//called to build the presences array as well as to build the presence keyword
void JSObjectScript::initializePresences(Handle<Object>& system_obj)
{
    v8::Context::Scope context_scope(mContext);
    // Create the space for the presences, they get filled in by
    // onConnected/onDisconnected calls
    v8::Local<v8::Array> arrayObj = v8::Array::New();
    system_obj->Set(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME), arrayObj);
}

v8::Handle<v8::Object> JSObjectScript::addPresence(const SpaceObjectReference& sporef) {
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

void JSObjectScript::removePresence(const SpaceObjectReference& sporef) {
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
   HandleScope handle_scope;
   //takes care of the addressable array in sys.

   system_obj->SetInternalField(SYSTEM_TEMPLATE_JSOBJSCRIPT_FIELD, External::New(this));

   initializePresences(system_obj);

   //FIXME: May need an initialize addressable

   createMessageableArray();
   populateAddressable(system_obj);
   initializePresences(system_obj);
   populateMath(system_obj);
}


void JSObjectScript::populateMath(Handle<Object>& system_obj)
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

  std::cout<<"\n\nERROR: Must fix create_presence to use new connect interface\n\n";
  assert(false);

  //FIXME: will need to add this presence to the presences vector.
  //but only want to do so when the function has succeeded.

}


//FIXME: Hard coded default mesh below
void JSObjectScript::create_presence(const SpaceID& new_space)
{
    std::cout<<"\n\nThis function does not exist yet.\n\n";
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


void JSObjectScript::setQueryAngleFunction(const SpaceObjectReference* sporef, const SolidAngle& sa)
{
    mParent->requestQueryUpdate(sporef->space(), sporef->object(), sa);
}




} // namespace JS
} // namespace Sirikata
