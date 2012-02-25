#ifndef __SIRIKATA_JS_CONTEXT_STRUCT_HPP__
#define __SIRIKATA_JS_CONTEXT_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>
#include "../JSSystemNames.hpp"
#include "../JSObjects/JSFields.hpp"
#include "JSSuspendable.hpp"
#include "../JSEntityCreateInfo.hpp"
#include "../JSUtil.hpp"
#include <sirikata/core/util/Vector3.hpp>
#include <sirikata/oh/Storage.hpp>
#include <sirikata/core/util/Liveness.hpp>
#include "../EmersonHttpManager.hpp"
#include "../JSVisibleManager.hpp"
#include "JSCapabilitiesConsts.hpp"
#include "../JSCtx.hpp"

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

struct JSContextStruct : public JSSuspendable, public Liveness
{
    JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence,
        SpaceObjectReference home,Capabilities::CapNum capNum,
        v8::Handle<v8::ObjectTemplate> contGlobTempl, uint32 contextID,
        JSContextStruct* parentContext, JSCtx* jsctx);

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

    //clear requests the jsobjectscript to begin to clear the context.
    //JSObjectScript calls finishClear, where all other suspendables are cleared
    //and the internal state of this object is removed.
    virtual v8::Handle<v8::Value> clear();
    void finishClear();

    v8::Handle<v8::Value>  struct_suspendContext();
    v8::Handle<v8::Value>  struct_resumeContext();

    v8::Handle<v8::Value>  checkHeadless();

    //returns an object that contains the system/system object associated with
    //this context
    v8::Handle<v8::Object> struct_getSystem();

    v8::Handle<v8::Value> struct_create_vis(const SpaceObjectReference& sporefWathcing, JSVisibleDataPtr addParams);

    v8::Handle<v8::Value> killEntity();

    /**
       @param {sporef} goneFrom The id of the local presence that the presence
       represented by jsvis is no longer within query set of.

       @param {JSVisibleStruct*} The struct associated with the external
       presence that is no longer visible to goneFrom.

       @param {bool} isGone True if the event is that a visible moved *out* of a
       presence's result set.  False if the event is that a visible moved *into*
       a presence's result set.

       Checks to see if this notification is applicable to fire.  Ie, if the
       sandbox isn't suspended and if the notification is for its root presence
       (and we have capability to fire for the root presence) or if the
       notification is for one of the presences created in this sandbox.

       If the above conditions are met, actually fires proximate event.
     */
    void proximateEvent(const SpaceObjectReference& goneFrom,
        JSVisibleStruct* jsvis,bool isGone);


    v8::Handle<v8::Value> pushEvalContextScopeDirectory(const String& newDir);
    v8::Handle<v8::Value> popEvalContextScopeDirectory();
    
    v8::Handle<v8::Value> storageBeginTransaction();
    v8::Handle<v8::Value> storageCommit(v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageRangeRead(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageRangeErase(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageCount(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb);

    /**
       @param {string} serialized message to send
       @param {JSContextStruct*} destination.  (If null, means send to parent).

       Sends a message from this sandbox to the sandbox associated with
       destination.  If destination is null, sends to parent.
     */
    v8::Handle<v8::Value> sendSandbox(const String& msgToSend, JSContextStruct* destination);


    v8::Handle<v8::Value> setRestoreScript(const String& key, v8::Handle<v8::Function> cb);


    v8::Handle<v8::Value> struct_evalInGlobal(const String& native_contents, ScriptOrigin* sOrigin);


    //create presence in the place, and with the script specified in eci
    v8::Handle<v8::Value> struct_createEntity(EntityCreateInfo& eci);

    v8::Handle<v8::Value> struct_setReset(const std::map<SpaceObjectReference, std::vector<SpaceObjectReference> > & proxResSet);
    v8::Handle<v8::Value> struct_setScript(const String& script);
    v8::Handle<v8::Value> struct_getScript();
    v8::Handle<v8::Value> debug_fileWrite(String& strToWrite,String& filename);
    v8::Handle<v8::Value> debug_fileRead(String& filename);


    v8::Handle<v8::Value> deserialize(const String& toDeserialize);

    v8::Handle<v8::Value> struct_rootReset();

    v8::Handle<v8::Value> restorePresence(PresStructRestoreParams& psrp);

    //when add a handler, timer, when inside of context, want to register them.
    //That way, when call suspend on context and resume on context, can
    //suspend/resume them.
    void struct_registerSuspendable   (JSSuspendable* toRegister);
    void struct_deregisterSuspendable (JSSuspendable* toDeregister);

    //if calling deregister through a post.
    void struct_asyncDeregisterSuspendable (
        JSSuspendable* toDeregister,Liveness::Token contAlive,
        Liveness::Token suspAlive);

    //creates a vec3 emerson object out of the vec3d cpp object passed in.
    v8::Handle<v8::Value> struct_createVec3(Vector3d& toCreate);
    v8::Handle<v8::Value> struct_createQuaternion(Quaternion& toCreate);

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
    v8::Handle<v8::Value>  struct_import(const String& toImportFrom,bool isJS);
    //string argument is the filename that we're trying to open and execute
    //contents of.
    v8::Handle<v8::Value>  struct_require(const String& toRequireFrom,bool isJS);

    /**
       msgObj is the JS object message that is being delivered from sender to
       me.  Dispatches the onSandboxMessage handler that is bound to newly
       created contexts.
     */
    void receiveSandboxMessage(v8::Local<v8::Object> msgObj, JSContextStruct* sender);


    /**
       @param {JSPresenceStruct} jspres Each context (other than the root) is
       associated with a single presence.  Depending on the capabilities granted
       to a sandbox, code within the sandbox can act on that presence, for
       instance, sending messages from it, receiving its proximity callbacks, etc.

       @param {SpaceObjectReference} canSendTo The sporef associated with the
       May be SpaceObjectReference::null.  Used for the sendHome call.

       @param {Capabilities::CapNum} capNum Capabilities granted to knew sandbox.

     */
    v8::Handle<v8::Value> struct_createContext(JSPresenceStruct* jspres,const SpaceObjectReference& canSendTo,Capabilities::CapNum capNum);

    // Trigger an event handler
    v8::Handle<v8::Value> struct_event(v8::Persistent<v8::Function>& cb);

    //create a timer that will fire cb in dur seconds from now,
    v8::Handle<v8::Value> struct_createTimeout(double period, v8::Persistent<v8::Function>& cb);
    v8::Handle<v8::Value> struct_createTimeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared);


    v8::Handle<v8::Value> sendMessageNoErrorHandler(JSPresenceStruct* jspres,const String& serialized_message,JSPositionListener* jspl,bool reliable);


    //register cb_persist as the default handler that gets thrown
    v8::Handle<v8::Value> struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist);
    v8::Handle<v8::Value> struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist);
    void checkContextConnectCallback(JSPresenceStruct* jspres);
    void checkContextDisconnectCallback(JSPresenceStruct* jspres);



    //********data
    JSObjectScript* jsObjScript;

    //this is the context that any and all objects will be run in.
    v8::Persistent<v8::Context> mContext;
    JSCtx* mCtx;

    String getScript();
    //sets proxAddedFunc and proxRemovedFunc, respectively
    v8::Handle<v8::Value> proxAddedHandlerCallallback(v8::Handle<v8::Function>cb);
    v8::Handle<v8::Value> proxRemovedHandlerCallallback(v8::Handle<v8::Function>cb);


    v8::Handle<v8::Value> getAssociatedPresence();

    JSPresenceStruct* getAssociatedPresenceStruct()
    {
        return associatedPresence;
    }

    v8::Persistent<v8::Function>proxAddedFunc;
    v8::Persistent<v8::Function>proxRemovedFunc;


   /**
      The http request that was associated with this request failed. Execute
      callback with two arguments: false, and a string with the reason
      (potential reasons: UNKNOWN_ERROR, REQUEST_PARSING_ERROR,
      RESPONSE_PARSING_ERROR.  Caller is in charge of calling dispose on cb.
   */
    void httpFail(v8::Persistent<v8::Function> cb,const String& failureReason );
    /**
       The http request that was associated with cb passed, and its associated
       data are in httpResp.  Execute callback with first argument true, and
       second argument containing data.
       Caller is in charge of calling dispose on cb.
     */
    void httpSuccess(v8::Persistent<v8::Function> cb,EmersonHttpManager::HttpRespPtr httpResp);
    v8::Handle<v8::Value> httpRequest(Sirikata::Network::Address addr, Transfer::HttpManager::HTTP_METHOD method, String request, v8::Persistent<v8::Function> cb);


    v8::Handle<v8::Value> setSandboxMessageCallback(v8::Persistent<v8::Function> callback);
    v8::Handle<v8::Value> setPresenceMessageCallback(v8::Persistent<v8::Function> callback);

    v8::Handle<v8::Value> emersonCompileString(const String& toCompile);

    //Each entity consists of a sandbox tree.  mParentContext points to the
    //parent of the current sandbox.  (Can be null for root sandbox.)  Can
    //access children sandboxes through associatedSuspendables map.
    JSContextStruct* mParentContext;

    //Should always check if empty before using.  Contains function to dispatch
    //if we ever receive a sandbox message.  Takes two arguments: 1: decoded
    //sandbox message (should just be an object); 2: sandbox object for sender
    //of message; if parent sent message, then the second field is null (which
    //should agree with sandbox.PARENT set in std/shim/sandbox.em
    v8::Persistent<v8::Function> sandboxMessageCallback;
    v8::Persistent<v8::Function> presenceMessageCallback;

    //returns associated capabilities number
    Capabilities::CapNum getCapNum();


private:

    uint32 mContextID;

    //runs through suspendable map to check if have a presence in this sandbox
    //matching sporef
    bool hasPresence(const SpaceObjectReference& sporef);

    //performs the initialization and population of util object, system object,
    //and system object's presences array.
    void createContextObjects(String* scriptToEval=NULL);

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


//working with presence wrappers: check if associatedPresence is null and throw
//exception if is.
#define NullPresenceCheck(funcName)                                     \
    String fname (funcName);                                            \
    if (associatedPresence == NULL)                                     \
    {                                                                   \
        String errorMessage = "Error in " + fname +                     \
            " of JSContextStruct.  " +                                  \
            "Have no default presence to perform action with.";         \
                                                                        \
        return v8::ThrowException( v8::Exception::Error(                \
                v8::String::New(errorMessage.c_str())));                \
    }

#define CHECK_EMERSON_SCRIPT_ERROR(emerScriptName,errorIn,whatToCast)   \
    EmersonScript* emerScriptName =                                     \
        dynamic_cast<EmersonScript*> (whatToCast);                      \
    if (emerScriptName == NULL)                                         \
    {                                                                   \
        return v8::ThrowException(                                      \
            v8::Exception::Error(v8::String::New(                       \
                    "Error.  Must not be in headless mode to run "      \
                    #errorIn                                            \
                )));                                                    \
    }


#define CHECK_EMERSON_SCRIPT_RETURN(emerScriptName,errorIn,whatToCast)  \
    EmersonScript* emerScriptName =                                     \
        dynamic_cast<EmersonScript*> (whatToCast);                      \
    if (emerScriptName == NULL)                                         \
    {                                                                   \
        JSLOG(error,"Error in " #errorIn ".  "  <<                      \
            "Must not be in headless mode to run " #errorIn );          \
                                                                        \
        return;                                                         \
    }




}; //end class


#define INLINE_CONTEXT_CONV_ERROR(toConvert,whereError,whichArg,whereWriteTo) \
    JSContextStruct* whereWriteTo;                                      \
    {                                                                   \
        String _errMsg = "In " #whereError "cannot convert " #whichArg " to sandbox struct"; \
        whereWriteTo = JSContextStruct::decodeContextStruct(toConvert,_errMsg); \
        if (whereWriteTo == NULL)                                       \
            V8_EXCEPTION_STRING(_errMsg);                               \
    }



typedef std::vector<JSContextStruct*> ContextVector;
typedef ContextVector::iterator ContextVecIter;



}//end namespace js
}//end namespace sirikata

#endif
