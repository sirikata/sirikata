
#ifndef __SIRIKATA_JS_SYSTEM_HPP__
#define __SIRIKATA_JS_SYSTEM_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"



namespace Sirikata {
namespace JS {
namespace JSSystem {

v8::Handle<v8::Value> sendMessage (const v8::Arguments& args);


v8::Handle<v8::Value> root_canSendMessage(const v8::Arguments& args);
v8::Handle<v8::Value> root_canRecvMessage(const v8::Arguments& args);
v8::Handle<v8::Value> root_canProx(const v8::Arguments& args);
v8::Handle<v8::Value> root_canImport(const v8::Arguments& args);

v8::Handle<v8::Value> root_canCreatePres(const v8::Arguments& args);
v8::Handle<v8::Value> root_canCreateEnt(const v8::Arguments& args);
v8::Handle<v8::Value> root_canEval(const v8::Arguments& args);

v8::Handle<v8::Value> root_createVisible(const v8::Arguments& args);
v8::Handle<v8::Value> createVisFromAddress(const v8::Handle<v8::Value>& addressFor);
v8::Handle<v8::Value> createVisFromFull(const v8::Arguments& args);

v8::Handle<v8::Value> root_headless(const v8::Arguments& args);

v8::Handle<v8::Value> debug_fileWrite(const v8::Arguments& args);
v8::Handle<v8::Value> debug_fileRead(const v8::Arguments& args);

//backend functions
v8::Handle<v8::Value> backendCreateEntry(const v8::Arguments& args);
v8::Handle<v8::Value> backendWrite(const v8::Arguments& args);
v8::Handle<v8::Value> backendFlush(const v8::Arguments& args);
v8::Handle<v8::Value> backendRead(const v8::Arguments& args);
v8::Handle<v8::Value> backendClearEntry(const v8::Arguments& args);


//end backend functions


v8::Handle<v8::Value> root_import(const v8::Arguments& args);
v8::Handle<v8::Value> root_require(const v8::Arguments& args);
v8::Handle<v8::Value> root_getPosition(const v8::Arguments& args);

v8::Handle<v8::Value> root_registerHandler(const v8::Arguments& args);
v8::Handle<v8::Value> root_timeout(const v8::Arguments& args);
v8::Handle<v8::Value> root_sendHome(const v8::Arguments& args);
v8::Handle<v8::Value> root_print(const v8::Arguments& args);

v8::Handle<v8::Value> root_getVersion(const v8::Arguments& args);

v8::Handle<v8::Value> root_createPresence(const v8::Arguments& args);
v8::Handle<v8::Value> root_restorePresence(const v8::Arguments& args);

v8::Handle<v8::Value> root_scriptEval(const v8::Arguments& args);
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




}//end jssystem namespace
}//end js namespace
}//end sirikata

#endif
