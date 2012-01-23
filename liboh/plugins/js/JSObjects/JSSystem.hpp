
#ifndef __SIRIKATA_JS_SYSTEM_HPP__
#define __SIRIKATA_JS_SYSTEM_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"



namespace Sirikata {
namespace JS {
namespace JSSystem {

v8::Handle<v8::Value> sendMessageReliable (const v8::Arguments& args);
v8::Handle<v8::Value> sendMessageUnreliable(const v8::Arguments& args);
v8::Handle<v8::Value> sendMessage(const v8::Arguments&args, bool reliable);

v8::Handle<v8::Value> getAssociatedPresence(const v8::Arguments& args);
v8::Handle<v8::Value> root_canSendMessage(const v8::Arguments& args);
v8::Handle<v8::Value> root_canRecvMessage(const v8::Arguments& args);
v8::Handle<v8::Value> root_canProxCallback(const v8::Arguments& args);
v8::Handle<v8::Value> root_canProxChangeQuery(const v8::Arguments& args);
v8::Handle<v8::Value> root_canImport(const v8::Arguments& args);

v8::Handle<v8::Value> root_canCreatePres(const v8::Arguments& args);
v8::Handle<v8::Value> root_canCreateEnt(const v8::Arguments& args);
v8::Handle<v8::Value> root_canEval(const v8::Arguments& args);

v8::Handle<v8::Value> root_createVisible(const v8::Arguments& args);
v8::Handle<v8::Value> createVisFromAddress(const v8::Handle<v8::Value>& addressFor);
v8::Handle<v8::Value> createVisFromFull(const v8::Arguments& args);

v8::Handle<v8::Value> root_headless(const v8::Arguments& args);
v8::Handle<v8::Value> root_sendSandbox(const v8::Arguments& args);

v8::Handle<v8::Value> debug_fileWrite(const v8::Arguments& args);
v8::Handle<v8::Value> debug_fileRead(const v8::Arguments& args);

v8::Handle<v8::Value> root_killEntity(const v8::Arguments& args);

v8::Handle<v8::Value> root_http(const v8::Arguments& args);

v8::Handle<v8::Value> emersonCompileString(const v8::Arguments& args);
v8::Handle<v8::Value> evalInGlobal(const v8::Arguments& args);
v8::Handle<v8::Value> getUniqueToken(const v8::Arguments& args);

v8::Handle<v8::Value> setSandboxMessageCallback(const v8::Arguments& args);
v8::Handle<v8::Value> setPresenceMessageCallback(const v8::Arguments& args);

v8::Handle<v8::Value> pushEvalContextScopeDirectory(const v8::Arguments& args);
v8::Handle<v8::Value> popEvalContextScopeDirectory(const v8::Arguments& args);

//storage functions
v8::Handle<v8::Value> storageBeginTransaction(const v8::Arguments& args);
v8::Handle<v8::Value> storageCommit(const v8::Arguments& args);
v8::Handle<v8::Value> storageWrite(const v8::Arguments& args);
v8::Handle<v8::Value> storageRead(const v8::Arguments& args);
v8::Handle<v8::Value> storageErase(const v8::Arguments& args);
v8::Handle<v8::Value> storageRangeRead(const v8::Arguments& args);
v8::Handle<v8::Value> storageRangeErase(const v8::Arguments& args);
v8::Handle<v8::Value> storageCount(const v8::Arguments& args);
//end storage functions

v8::Handle<v8::Value> setRestoreScript(const v8::Arguments& args);

v8::Handle<v8::Value> root_import(const v8::Arguments& args);
v8::Handle<v8::Value> root_require(const v8::Arguments& args);
v8::Handle<v8::Value> root_jsimport(const v8::Arguments& args);
v8::Handle<v8::Value> commonImport(const v8::Arguments& args, bool isJS);
v8::Handle<v8::Value> root_jsrequire(const v8::Arguments& args);
v8::Handle<v8::Value> commonRequire(const v8::Arguments& args,bool isJS);


v8::Handle<v8::Value> root_event(const v8::Arguments& args);
v8::Handle<v8::Value> root_timeout(const v8::Arguments& args);
v8::Handle<v8::Value> root_sendHome(const v8::Arguments& args);
v8::Handle<v8::Value> root_print(const v8::Arguments& args);

v8::Handle<v8::Value> root_getVersion(const v8::Arguments& args);

v8::Handle<v8::Value> root_createPresence(const v8::Arguments& args);
v8::Handle<v8::Value> root_restorePresence(const v8::Arguments& args);

v8::Handle<v8::Value> root_createContext(const v8::Arguments& args);
v8::Handle<v8::Value> root_createEntityNoSpace(const v8::Arguments& args);
v8::Handle<v8::Value> root_createEntity(const v8::Arguments& args);


v8::Handle<v8::Value> root_onPresenceConnected(const v8::Arguments& args);
v8::Handle<v8::Value> root_onPresenceDisconnected(const v8::Arguments& args);


v8::Handle<v8::Value> root_reset(const v8::Arguments& args);
v8::Handle<v8::Value> root_setScript(const v8::Arguments& args);
v8::Handle<v8::Value> root_getScript(const v8::Arguments& args);

v8::Handle<v8::Value> root_serialize(const v8::Arguments& args);
v8::Handle<v8::Value> root_deserialize(const v8::Arguments& args);


v8::Handle<v8::Value> root_proxAddedHandler(const v8::Arguments& args);
v8::Handle<v8::Value> root_proxRemovedHandler(const v8::Arguments& args);

bool decodeResetArg(v8::Handle<v8::Object> arg, std::map<SpaceObjectReference, std::vector<SpaceObjectReference> > & cppRes);


}//end jssystem namespace
}//end js namespace
}//end sirikata

#endif
