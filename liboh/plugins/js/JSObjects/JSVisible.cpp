#include "JSVisible.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"
#include "../JSObjectStructs/JSVisibleStruct.hpp"
#include "JSFields.hpp"
#include "JSVec3.hpp"

#include <sirikata/core/util/SpaceObjectReference.hpp>


namespace Sirikata {
namespace JS {
namespace JSVisible {


v8::Handle<v8::Value> dist(const v8::Arguments& args)
{
    if ((args.Length() != 0) && (args.Length() != 1))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly zero or one argument to dist method of Visible")));

    String errorMessage = "Error in dist method of JSVisible.cpp.  Cannot decode visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);

    if (jsvis == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

        
    if (args.Length() == 0)
        return jsvis->dist(NULL);

    if (! args[0]->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in dist of JSVisible.cpp.  Argument should be an objet.")));

    v8::Handle<v8::Object> argObj = args[0]->ToObject();
    
    bool isVec3 = Vec3Validate(argObj);
    if (! isVec3)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: argument to dist method of Visible needs to be a vec3")));

    Vector3d vec3 = Vec3Extract(argObj);
    
    return jsvis->dist(&vec3);
}



v8::Handle<v8::Value> getPosition(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to getPosition method of Visible")));

    std::string errorMessage = "In getPosition function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->returnProxyPosition();
}


v8::Handle<v8::Value> toString(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to toString method of Visible")));

    std::string errorMessage = "In toString function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->toString();
}


v8::Handle<v8::Value> __debugRef(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to __debugRef.  Requires 0 arguments.")) );

    std::string errorMessage = "In __debugRef function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->printData();
}



v8::Handle<v8::Value> __visibleSendMessage (const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sendMessage(<object>)")) );

    //decode string argument
    v8::Handle<v8::Value> messageBody = args[0];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    std::string serialized_message = JSSerializer::serializeObject(v8Object);

    //decode the visible struct associated with this object
    std::string errorMessage = "In __visibleSendMessage function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));


    return jsvis->visibleSendMessage(serialized_message);
}

bool isVisibleObject(v8::Local<v8::Value> v8Val)
{
  if( v8Val->IsNull() || v8Val->IsUndefined() || !v8Val->IsObject())
  {
    return false;
  }

  // This is an object

  v8::Handle<v8::Object>v8Obj = v8::Handle<v8::Object>::Cast(v8Val);
  v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
  if(typeidVal->IsNull() || typeidVal->IsUndefined())
  {
      return false;
  }
    
  v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
  void* ptr = wrapped->Value();
  std::string* typeId = static_cast<std::string*>(ptr);
  if(typeId == NULL) return false;
    
  std::string typeIdString = *typeId;

  if(typeIdString == VISIBLE_TYPEID_STRING)
  {
    return true;
  }

  return false;

}

v8::Handle<v8::Value> getStillVisible(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to getStillVisible.  Requires 0 arguments.")) );

    std::string errorMessage = "In getStillVisible function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->getStillVisible();

}

v8::Handle<v8::Value> checkEqual(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to checkEqual.  Requires 1 argument: another JSVisibleStruct.")) );

    
    std::string errorMessage = "In checkEqual function of visible.  Decoding caller.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));


    errorMessage = "In checkEqual function of visible.  Decoding argument.  ";
    JSVisibleStruct* jsvis2 = JSVisibleStruct::decodeVisible(args[0],errorMessage);
    if (jsvis2 == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    
    return jsvis->checkEqual(jsvis2);
}


}//end jsvisible namespace
}//end js namespace
}//end sirikata


