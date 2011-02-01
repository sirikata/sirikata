
#ifndef __SIRIKATA_JS_VISIBLE_HPP__
#define __SIRIKATA_JS_VISIBLE_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"


#include "../JSSerializer.hpp"



namespace Sirikata {
namespace JS {
namespace JSVisible {

v8::Handle<v8::Value> toString(const v8::Arguments& args);
v8::Handle<v8::Value> __debugRef(const v8::Arguments& args);
v8::Handle<v8::Value> __visibleSendMessage (const v8::Arguments& args);
v8::Handle<v8::Value> getPosition(const v8::Arguments& args);
v8::Handle<v8::Value> getStillVisible(const v8::Arguments& args);
v8::Handle<v8::Value> checkEqual(const v8::Arguments& args);
bool decodeVisible(v8::Handle<v8::Value>, JSObjectScript*&, SpaceObjectReference*& , SpaceObjectReference*&);


}//end jsvisible namespace
}//end js namespace
}//end sirikata

#endif
