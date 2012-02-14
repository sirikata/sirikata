#ifndef __SIRIKATA_JS_SYSTEM_STRUCT_HPP__
#define __SIRIKATA_JS_SYSTEM_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include "../JSEntityCreateInfo.hpp"
#include <sirikata/oh/Storage.hpp>
#include "JSPresenceStruct.hpp"
#include "JSContextStruct.hpp"
#include "JSCapabilitiesConsts.hpp"


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSEventHandlerStruct;


//Most calls in this class just go straight through into associated context to
//make a sibling call.  Split system into intermediate layer between v8-bound
//calls and jscontextstruct to make tracking of capabilities explicit, and easy
//to check without having to dig through a lot of other code.
struct JSSystemStruct
{
    JSSystemStruct(JSContextStruct* jscont, Capabilities::CapNum capNum);
    ~JSSystemStruct();

    static JSSystemStruct* decodeSystemStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

    v8::Handle<v8::Value> proxAddedHandlerCallallback(v8::Handle<v8::Function>cb);
    v8::Handle<v8::Value> proxRemovedHandlerCallallback(v8::Handle<v8::Function>cb);

    //regular members
    v8::Handle<v8::Value> struct_canSendMessage();
    v8::Handle<v8::Value> struct_canRecvMessage();
    v8::Handle<v8::Value> struct_canProxCallback();
    v8::Handle<v8::Value> struct_canProxChangeQuery();
    v8::Handle<v8::Value> struct_canImport();

    v8::Handle<v8::Value> pushEvalContextScopeDirectory(const String& newDir);
    v8::Handle<v8::Value> popEvalContextScopeDirectory();
    
    v8::Handle<v8::Value> checkResources();
    v8::Handle<v8::Value> struct_evalInGlobal(const String& native_contents, ScriptOrigin* sOrigin);
    v8::Handle<v8::Value> checkHeadless();

    v8::Handle<v8::Value> getAssociatedPresence();

    v8::Handle<v8::Value> storageBeginTransaction();
    v8::Handle<v8::Value> storageCommit(v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageRangeRead(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageRangeErase(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageCount(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb);


    v8::Handle<v8::Value> setRestoreScript(const String& key, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> killEntity();

    v8::Handle<v8::Value> struct_canCreatePres();
    v8::Handle<v8::Value> struct_canCreateEnt();
    v8::Handle<v8::Value> struct_canEval();

    /**
       @param {v8::Object} msgToSend
       @param {JSContextStruct*} destination.  (If null, means send to parent).

       Sends a message from the sandbox associated with this system object to
       the sandbox associated with destination.  If destination is null, sends
       to parent.

     */
    v8::Handle<v8::Value> sendSandbox(const String& msgToSend, JSContextStruct* destination);

    v8::Handle<v8::Value> setSandboxMessageCallback(v8::Persistent<v8::Function> callback);
    v8::Handle<v8::Value> setPresenceMessageCallback(v8::Persistent<v8::Function> callback);

    v8::Handle<v8::Value> emersonCompileString(const String& toCompile);

    v8::Handle<v8::Value> struct_create_vis(const SpaceObjectReference& sporefWatching, JSVisibleDataPtr addParams)
    {
        return associatedContext->struct_create_vis(sporefWatching,addParams);
    }


    v8::Handle<v8::Value> restorePresence(PresStructRestoreParams& psrp);


    v8::Handle<v8::Value> debug_fileWrite(String& strToWrite,String& filename);
    v8::Handle<v8::Value> debug_fileRead(String& filename);

    v8::Handle<v8::Value> httpRequest(Sirikata::Network::Address addr, Transfer::HttpManager::HTTP_METHOD method, String request, v8::Persistent<v8::Function> cb);


    v8::Handle<v8::Value> struct_print(const String& msg);
    v8::Handle<v8::Value> struct_sendHome(const String& toSend);
    v8::Handle<v8::Value> struct_import(const String& toImportFrom,bool isJS);
    v8::Handle<v8::Value> struct_require(const String& toRequireFrom,bool isJS);


    //if have the capability to create presences, create a new presence with
    //mesh newMesh and executes initFunc, which gets executed onConnected.
    //if do not have the capability, throws an error.
    v8::Handle<v8::Value> struct_createEntity(EntityCreateInfo& eci);

    v8::Handle<v8::Value> struct_createContext(JSPresenceStruct* jspres,const SpaceObjectReference& canSendTo, Capabilities::CapNum permNum);

    JSContextStruct* getContext();

    v8::Handle<v8::Value> struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist);
    v8::Handle<v8::Value> struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist);

    //last bool indicates whether to send message reliably or unreliably.
    v8::Handle<v8::Value> sendMessageNoErrorHandler(JSPresenceStruct* jspres, const String& serialized_message,JSPositionListener* jspl,bool reliable);


    v8::Handle<v8::Value> deserialize(const String& toDeserialize);

    // Trigger an event handler
    v8::Handle<v8::Value> struct_event(v8::Persistent<v8::Function>& cb);

    //create a timer that will fire in dur seconds from now, that will bind the
    //this parameter to target and that will fire the callback cb.
    v8::Handle<v8::Value> struct_createTimeout(double period, v8::Persistent<v8::Function>& cb);

    v8::Handle<v8::Value> struct_createTimeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared);

    v8::Handle<v8::Value> struct_setScript(const String& script);
    v8::Handle<v8::Value> struct_getScript();
    v8::Handle<v8::Value> struct_reset(const std::map<SpaceObjectReference, std::vector<SpaceObjectReference> > & proxResSet);


    Capabilities::CapNum getCapNum();

private:

   /**
      @param {Capabilities::CapNum} The requested amount of capabilities.
      @param {Capabilities::Caps} capRequesting Capability that scripter is
      requesting to imbue into new sandbox.

      @param {JSPresenceStruct} jspres Default presence for new sandbox.

      If scripter is trying to request capabilities that the initial sandbox he/she
      is creating does not have, strips those capabilities.
   */
    void stripCapEscalation(Capabilities::CapNum& permNum, Capabilities::Caps capRequesting, JSPresenceStruct* jspres, const String& capRequestingName);


    //returns true if you have capability to perform the operation associated with
    //capRequesting on jspres, false otherwise.  Note: pass null to jspres if
    //requesting a capability not associated with a presence.  (See list of
    //these in JSCapabilitiesConsts.hpp.)
    bool checkCurCtxtHasCapability(JSPresenceStruct* jspres, Capabilities::Caps capRequesting);

    //associated data
    JSContextStruct* associatedContext;
    uint32 mCapNum;
};




#define INLINE_SYSTEM_CONV_ERROR(toConvert,whereError,whichArg,whereWriteTo)   \
    JSSystemStruct* whereWriteTo;                                                   \
    {                                                                      \
        String _errMsg = "In " #whereError "cannot convert " #whichArg " to system struct";     \
        whereWriteTo = JSSystemStruct::decodeSystemStruct(toConvert,_errMsg); \
        if (whereWriteTo == NULL) \
            return v8::ThrowException(v8::Exception::Error(v8::String::New(_errMsg.c_str(), _errMsg.length()))); \
    }


}//end namespace js
}//end namespace sirikata

#endif
