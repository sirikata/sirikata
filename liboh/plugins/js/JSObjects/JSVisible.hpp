
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
v8::Handle<v8::Value> getPosition(const v8::Arguments& args);
v8::Handle<v8::Value> getVelocity(const v8::Arguments& args);
v8::Handle<v8::Value> getOrientation(const v8::Arguments& args);
v8::Handle<v8::Value> getOrientationVel(const v8::Arguments& args);
v8::Handle<v8::Value> getSpace(const v8::Arguments& args);
v8::Handle<v8::Value> getOref(const v8::Arguments& args);
v8::Handle<v8::Value> getScale(const v8::Arguments& args);
v8::Handle<v8::Value> getStillVisible(const v8::Arguments& args);
v8::Handle<v8::Value> checkEqual(const v8::Arguments& args);
v8::Handle<v8::Value> dist(const v8::Arguments& args);
bool isVisibleObject(v8::Handle<v8::Value> v8Val);
Handle<v8::Value> getMesh(const v8::Arguments& args);

}//end jsvisible namespace
}//end js namespace
}//end sirikata

#endif
