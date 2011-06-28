#ifndef __SIRIKATA_JS_PRESENCE_HPP__
#define __SIRIKATA_JS_PRESENCE_HPP__

#include <sirikata/oh/Platform.hpp>
#include "../JSObjectStructs/JSPresenceStruct.hpp"
#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"
#include <sirikata/core/transfer/URI.hpp>



namespace Sirikata
{
namespace JS
{
namespace JSPresence
{

bool isPresence(v8::Handle<v8::Value> v8Val);

v8::Handle<v8::Value>  setMesh(const v8::Arguments& args);
v8::Handle<v8::Value>  getMesh(const v8::Arguments& args);

v8::Handle<v8::Value>  setPosition(const v8::Arguments& args);
v8::Handle<v8::Value>  getPosition(const v8::Arguments& args);

v8::Handle<v8::Value>  getSpace(const v8::Arguments& args);
v8::Handle<v8::Value>  getOref(const v8::Arguments& args);

v8::Handle<v8::Value>  getAllData(const v8::Arguments& args);
v8::Handle<v8::Value>  pres_disconnect(const v8::Arguments& args);

v8::Handle<v8::Value>  getIsConnected(const v8::Arguments& args);

v8::Handle<v8::Value>  getVelocity(const v8::Arguments& args);
v8::Handle<v8::Value>  setVelocity(const v8::Arguments& args);


v8::Handle<v8::Value>  getOrientation(const v8::Arguments& args);
v8::Handle<v8::Value>  setOrientation(const v8::Arguments& args);

v8::Handle<v8::Value>  setOrientationVel(const v8::Arguments& args);
v8::Handle<v8::Value>  getOrientationVel(const v8::Arguments& args);

v8::Handle<v8::Value>  setScale(const v8::Arguments& args);
v8::Handle<v8::Value>  getScale(const v8::Arguments& args);

v8::Handle<v8::Value>  setQueryAngle(const v8::Arguments& args);
v8::Handle<v8::Value>  getQueryAngle(const v8::Arguments& args);

v8::Handle<v8::Value>  setPhysics(const v8::Arguments& args);
v8::Handle<v8::Value>  getPhysics(const v8::Arguments& args);

v8::Handle<v8::Value> toString(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);


bool getURI(const v8::Arguments& args,std::string& returner);
v8::Handle<v8::Value>runSimulation(const v8::Arguments& args);

v8::Handle<v8::Value>isConnectedGetter(v8::Local<v8::String> property, const AccessorInfo& info);
void isConnectedSetter(v8::Local<v8::String> property, v8::Local<v8::Value> toSetTo,const AccessorInfo& info);


v8::Handle<v8::Value>distance(const v8::Arguments& args);

v8::Handle<v8::Value> toVisible(const v8::Arguments& args);

v8::Handle<v8::Value> pres_suspend(const v8::Arguments& args);
v8::Handle<v8::Value> pres_resume(const v8::Arguments& args);

v8::Handle<v8::Value> loadMesh(const v8::Arguments& args);
v8::Handle<v8::Value> unloadMesh(const v8::Arguments& args);

void setNullPresence(const v8::Arguments& args);

} //end namespace JSPresence
} //end namespace JS
} //end namespace Sirikata
#endif
