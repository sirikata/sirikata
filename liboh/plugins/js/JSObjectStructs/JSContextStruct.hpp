#ifndef __SIRIKATA_JS_CONTEXT_STRUCT_HPP__
#define __SIRIKATA_JS_CONTEXT_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>
#include "../JSSystemNames.hpp"
#include "../JSObjects/JSFields.hpp"
#include "JSSuspendable.hpp"
#include "JSEventHandlerStruct.hpp"
#include "../JSPattern.hpp"
#include "../JSEntityCreateInfo.hpp"
#include "../JSUtil.hpp"
#include <sirikata/core/util/Vector3.hpp>
#include <sirikata/oh/Storage.hpp>


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class EmersonScript;
class JSPresenceStruct;
class JSTimerStruct;
class JSUtilStruct;
class JSPositionListener;
class JSSystemStruct;
struct PresStructRestoreParams;
class JSProxyData;

struct JSContextStruct : public JSSuspendable
{
    JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference home, bool sendEveryone, bool recvEveryone, bool proxQueries, bool canImport,bool canCreatePres,bool canCreateEnt,bool canEval,v8::Handle<v8::ObjectTemplate> contGlobTempl, uint32 contextID);
    ~JSContextStruct();

    //looks in current context and returns the current context as pointer to
    //user.  if unsuccessful, return null.
    static JSContextStruct* decodeContextStruct(v8::Handle<v8::Value> toDecode, String& errorMsg);

    uint32 getContextID();


    //contexts can be suspended (suspends all suspendables within the context)
    //resumed (takes all suspendables in context and calls resume on them)
    //or cleared (marks all objects native to this context and its chilren as
    //ready for garbage collection)  (also clears all suspendables in the context).
    virtual v8::Handle<v8::Value> suspend();
    virtual v8::Handle<v8::Value> resume();
    virtual v8::Handle<v8::Value> clear();

    v8::Handle<v8::Value>  struct_suspendContext();
    v8::Handle<v8::Value>  struct_resumeContext();

    v8::Handle<v8::Value>  checkHeadless();

    //returns an object that contains the system/system object associated with
    //this context
    v8::Handle<v8::Object> struct_getSystem();

    v8::Handle<v8::Value> struct_create_vis(const SpaceObjectReference& sporefWathcing,JSProxyData* addParams);

    v8::Handle<v8::Value> killEntity();


    v8::Handle<v8::Value> storageBeginTransaction();
    v8::Handle<v8::Value> storageCommit(v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb);


    v8::Handle<v8::Value> setRestoreScript(const String& key, v8::Handle<v8::Function> cb);


    //creates a new jseventhandlerstruct and wraps it in a js object
    //registers the jseventhandlerstruct both with this context and
    //jsobjectscript
    v8::Handle<v8::Value>  struct_makeEventHandlerObject(const PatternList& native_patterns, v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist,bool issusp);

    //create presence with mesh associated with string newMesh, and initFunction
    //to be called when presence is connected
    v8::Handle<v8::Value> struct_createPresence(const String& newMesh, v8::Handle<v8::Function> initFunc,const Vector3d& poser, const SpaceID& spaceToCreateIn);

    //create presence in the place, and with the script specified in eci
    v8::Handle<v8::Value> struct_createEntity(EntityCreateInfo& eci);

    v8::Handle<v8::Value> struct_setReset();
    v8::Handle<v8::Value> struct_setScript(const String& script);
    v8::Handle<v8::Value> struct_getScript();
    v8::Handle<v8::Value> debug_fileWrite(const String& strToWrite,const String& filename);
    v8::Handle<v8::Value> debug_fileRead(const String& filename);


    v8::Handle<v8::Value> deserializeObject(const String& toDeserialize);

    v8::Handle<v8::Value> struct_rootReset();

    v8::Handle<v8::Value> restorePresence(PresStructRestoreParams& psrp);

    //when add a handler, timer, when inside of context, want to register them.
    //That way, when call suspend on context and resume on context, can
    //suspend/resume them.
    void struct_registerSuspendable   (JSSuspendable* toRegister);
    void struct_deregisterSuspendable (JSSuspendable* toDeregister);

    //creates a vec3 emerson object out of the vec3d cpp object passed in.
    v8::Handle<v8::Value> struct_createVec3(Vector3d& toCreate);

    //if receiver is one of my presences, or it is the system presence that I
    //was created from return true.  Otherwise, return false.
    bool canReceiveMessagesFor(const SpaceObjectReference& receiver);



    void jsscript_print(const String& msg);
    void presenceDied();

    v8::Handle<v8::Value>  struct_executeScript(v8::Handle<v8::Function> funcToCall,const v8::Arguments& args);
    v8::Handle<v8::Value>  struct_getAssociatedPresPosition();
    v8::Handle<v8::Value>  struct_sendHome(const String& toSend);
    //string argument is the filename that we're trying to open and execute
    //contents of.
    v8::Handle<v8::Value>  struct_import(const String& toImportFrom);
    //string argument is the filename that we're trying to open and execute
    //contents of.
    v8::Handle<v8::Value>  struct_require(const String& toRequireFrom);

    //requests jsobjscript to create an event handler in the context associated
    //wth jscontextstruct.  registers this handler as well through struct_registerSuspendable
    v8::Handle<v8::Value>  struct_makeEventHandlerObject(JSEventHandlerStruct* jsehs);


    //presStruct: who the messages that this context's system sends will
    //be from canMessage: who you can always send messages to.  sendEveryone creates
    //system that can send messages to everyone besides just who created you.
    //recvEveryone means that you can receive messages from everyone besides just
    //who created you.  proxQueries means that you can issue proximity queries
    //yourself, and latch on callbacks for them.  canImport means that you can
    //import files/libraries into your code.
    //canCreatePres is whether have capability to create presences
    //canCreateEnt is whether have capability to create entities
    //canEval is whether have capability to call system.eval directly in context
    //creates a new context, and hangs the child into suspendables map.
    v8::Handle<v8::Value> struct_createContext(SpaceObjectReference canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport, bool canCreatePres, bool canCreateEnt, bool canEval, JSPresenceStruct* presStruct);

    //create a timer that will fire cb in dur seconds from now,
    v8::Handle<v8::Value> struct_createTimeout(double period, v8::Persistent<v8::Function>& cb);

    v8::Handle<v8::Value> struct_createTimeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared);

    //Tries to eval the emerson code in native_contents that came from origin
    //sOrigin inside of this context.
    v8::Handle<v8::Value> struct_eval(const String& native_contents, ScriptOrigin* sOrigin);

    v8::Handle<v8::Value> sendMessageNoErrorHandler(JSPresenceStruct* jspres,const String& serialized_message,JSPositionListener* jspl);


    //register cb_persist as the default handler that gets thrown
    v8::Handle<v8::Value> struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist);
    v8::Handle<v8::Value> struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist);
    void checkContextConnectCallback(JSPresenceStruct* jspres);
    void checkContextDisconnectCallback(JSPresenceStruct* jspres);


    v8::Handle<v8::Value> clearConservePres(std::vector<JSPresenceStruct*>& jspresVec);

    //********data
    JSObjectScript* jsObjScript;

    //this is the context that any and all objects will be run in.
    v8::Persistent<v8::Context> mContext;

    String getScript();
    v8::Handle<v8::Value> proxAddedHandlerCallallback(v8::Handle<v8::Function>cb);
    v8::Handle<v8::Value> proxRemovedHandlerCallallback(v8::Handle<v8::Function>cb);



    JSPresenceStruct* getAssociatedPresenceStruct()
    {
        return associatedPresence;
    }

    v8::Persistent<v8::Function>proxAddedFunc;
    v8::Persistent<v8::Function>proxRemovedFunc;


private:
    uint32 mContextID;

    //runs through suspendable map to check if have a presence in this sandbox
    //matching sporef
    bool hasPresence(const SpaceObjectReference& sporef);

    //performs the initialization and population of util object, system object,
    //and system object's presences array.
    void createContextObjects();

    //a function to call within this context for when a presence that was
    //created from within this context gets connected.
    bool hasOnConnectedCallback;
    v8::Persistent<v8::Function> cbOnConnected;
    bool hasOnDisconnectedCallback;
    v8::Persistent<v8::Function> cbOnDisconnected;

    //script associated with this context.
    String mScript;

    //a pointer to the local presence that is associated with this context.  for
    //instance, when you call getPosition on the system object, you actually
    //end up returning the position of this presence.  you send messages from
    //this presence, etc.  Can be null.
    JSPresenceStruct* associatedPresence;

    //homeObject is the spaceobjectreference of an object that you can always
    //send messages to regardless of permissions
    SpaceObjectReference mHomeObject;

    //a pointer to the system struct that will be used as a system-like object
    //inside of the context.
    JSSystemStruct* mSystem;
    //mSystem in a v8 wrapper.  also, its persistent!
    v8::Persistent<v8::Object> systemObj;

    v8::Handle<v8::ObjectTemplate> mContGlobTempl;

    //struct associated with the Emerson util object that is associated with this
    //context.
    JSUtilStruct* mUtil;

    //flag that is true if in the midst of a clear command.  False otherwise
    //(prevents messing up iterators in suspendable map).
    bool inClear;

    //all associated objects that will need to be suspended/resumed if context
    //is suspended/resumed
    SuspendableMap associatedSuspendables;

    //If mInSuspendableLoop is true, doesn't actually delete any suspendable
    //objects.  Just cues them to be deleted after we end suspendable loop.
    bool mInSuspendableLoop;
    SuspendableVec suspendablesToDelete;
    SuspendableVec suspendablesToAdd;

    void flushQueuedSuspendablesToChange();


    //working with presence wrappers: check if associatedPresence is null and throw exception if is.
#define NullPresenceCheck(funcName)        \
    String fname (funcName);               \
    if (associatedPresence == NULL)        \
    {                                      \
        String errorMessage = "Error in " + fname + " of JSContextStruct.  Have no default presence to perform action with."; \
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()))); \
    }

#define CHECK_EMERSON_SCRIPT_ERROR(emerScriptName,errorIn,whatToCast)             \
    EmersonScript* emerScriptName = dynamic_cast<EmersonScript*> (whatToCast); \
    if (emerScriptName == NULL)\
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Must not be in headless mode to run " #errorIn  )));


}; //end class

typedef std::vector<JSContextStruct*> ContextVector;
typedef ContextVector::iterator ContextVecIter;



}//end namespace js
}//end namespace sirikata

#endif
