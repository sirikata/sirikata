
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
void readORef(const v8::Arguments& args, JSObjectScript*& caller, ObjectReference*& oref);
v8::Handle<v8::Value> __addressableSendMessage (const v8::Arguments& args);


}}}

#endif
