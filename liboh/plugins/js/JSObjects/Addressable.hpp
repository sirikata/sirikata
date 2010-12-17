
#ifndef __SIRIKATA_JS_ADDRESSABLE_HPP__
#define __SIRIKATA_JS_ADDRESSABLE_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"



namespace Sirikata {
namespace JS {
namespace JSAddressable {

v8::Handle<v8::Value> toString(const v8::Arguments& args);
v8::Handle<v8::Value> __debugRef(const v8::Arguments& args);
v8::Handle<v8::Value> __addressableSendMessage (const v8::Arguments& args);
bool decodeAddressable(v8::Handle<v8::Object> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef);
bool decodeAddressable(v8::Handle<v8::Value> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef);
JSPresenceStruct* getPresStructFromArgs(const v8::Arguments& args);

}//end jsaddressable namespace
}//end js namespace
}//end sirikata

#endif
