
#ifndef __SIRIKATA_JS_FAKEROOT_HPP__
#define __SIRIKATA_JS_FAKEROOT_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"



namespace Sirikata {
namespace JS {
namespace JSFakeroot {


v8::Handle<v8::Value> root_canSendMessage(const v8::Arguments& args);
v8::Handle<v8::Value> root_canRecvMessage(const v8::Arguments& args);
v8::Handle<v8::Value> root_canProx(const v8::Arguments& args);
v8::Handle<v8::Value> root_canImport(const v8::Arguments& args);

v8::Handle<v8::Value> root_import(const v8::Arguments& args);
v8::Handle<v8::Value> root_require(const v8::Arguments& args);
v8::Handle<v8::Value> root_getPosition(const v8::Arguments& args);

v8::Handle<v8::Value> root_registerHandler(const v8::Arguments& args);
v8::Handle<v8::Value> root_timeout(const v8::Arguments& args);
v8::Handle<v8::Value> root_sendHome(const v8::Arguments& args);
v8::Handle<v8::Value> root_print(const v8::Arguments& args);

v8::Handle<v8::Value> root_getVersion(const v8::Arguments& args);

v8::Handle<v8::Value> root_createPresence(const v8::Arguments& args);

v8::Handle<v8::Value> root_scriptEval(const v8::Arguments& args);
v8::Handle<v8::Value> root_createContext(const v8::Arguments& args);
v8::Handle<v8::Value> root_createEntity(const v8::Arguments& args);


v8::Handle<v8::Value> root_onPresenceConnected(const v8::Arguments& args);
v8::Handle<v8::Value> root_onPresenceDisconnected(const v8::Arguments& args);




}//end jsfakeroot namespace
}//end js namespace
}//end sirikata

#endif
