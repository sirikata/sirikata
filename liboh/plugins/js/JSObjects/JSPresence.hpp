#ifndef __SIRIKATA_JS_PRESENCE_HPP__
#define __SIRIKATA_JS_PRESENCE_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"




namespace Sirikata 
{
  namespace JS 
  {
    namespace JSPresence
    {
	  
    v8::Handle<v8::Value> setMesh(const v8::Arguments& args);   
    v8::Handle<v8::Value> toString(const v8::Arguments& args);   
    v8::Handle<v8::Value> ScriptGetPosition(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    void ScriptSetPosition(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
    v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);

    template<typename WithHolderType>
    JSPresenceStruct* GetTargetPresenceStruct(const WithHolderType&);
    
//     template<typename WithHolderType>
//     JSObjectScript* GetTargetJSObjectScript(const WithHolderType&);
//     template<typename WithHolderType>
//     SpaceID* GetTargetSpaceID(const WithHolderType&);
    }
  }
}  
#endif   
