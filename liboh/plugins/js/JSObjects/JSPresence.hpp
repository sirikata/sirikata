#ifndef __SIRIKATA_JS_PRESENCE_HPP__
#define __SIRIKATA_JS_PRESENCE_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"
#include <sirikata/core/transfer/URI.hpp>



namespace Sirikata
{
  namespace JS
  {
    namespace JSPresence
    {

    v8::Handle<v8::Value>  setMesh(const v8::Arguments& args);
    Handle<v8::Value>      getMesh(const v8::Arguments& args);

    v8::Handle<v8::Value>  setPosition(const v8::Arguments& args);
    Handle<v8::Value>      getPosition(const v8::Arguments& args);


    Handle<v8::Value>      getVelocity(const v8::Arguments& args);
    v8::Handle<v8::Value>  setVelocity(const v8::Arguments& args);


    Handle<v8::Value>      getOrientation(const v8::Arguments& args);
    v8::Handle<v8::Value>  setOrientation(const v8::Arguments& args);

    v8::Handle<v8::Value>  setOrientationVel(const v8::Arguments& args);
    Handle<v8::Value>      getOrientationVel(const v8::Arguments& args);

    v8::Handle<v8::Value>  setScale(const v8::Arguments& args);
    Handle<v8::Value>      getScale(const v8::Arguments& args);

    v8::Handle<v8::Value> setQueryAngle(const v8::Arguments& args);

    v8::Handle<v8::Value> toString(const v8::Arguments& args);
    v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);

    template<typename WithHolderType>
    JSPresenceStruct* GetTargetPresenceStruct(const WithHolderType&);

    JSPresenceStruct* getPresStructFromArgs(const v8::Arguments& args);
    //Transfer::URI* getURI(const v8::Arguments& args);
    bool getURI(const v8::Arguments& args,std::string& returner);

    v8::Handle<v8::Value>runSimulation(const v8::Arguments& args);


    }
  }
}
#endif
