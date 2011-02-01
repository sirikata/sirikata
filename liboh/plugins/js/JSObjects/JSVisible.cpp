#include "JSVisible.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"
#include "../JSObjectStructs/JSVisibleStruct.hpp"
#include "JSFields.hpp"

#include <sirikata/core/util/SpaceObjectReference.hpp>


namespace Sirikata {
namespace JS {
namespace JSVisible {




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
    std::cout << "\n\nTrying to serialize visible\n\n";
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
//returns true if the object is a valid sender object
//utility function for working with visible objects.  pass in args for a
//call, get back the jsobjectscript that called it, and the space object
//reference that was visible
//bool decodeVisible(v8::Handle<v8::Value> sender_val, JSObjectScript*&
//jsObjScript, SpaceObjectReference*& sporef)
bool decodeVisible(v8::Handle<v8::Value> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef, SpaceObjectReference*& p)
{
    if ((!senderVal->IsObject()) || (senderVal->IsUndefined()))
    {
        jsObjScript = NULL;
        sporef = NULL;
        SILOG(js, debug, "\n\nReturning false from decodeVisible 1. senderVal->IsObject() = " << senderVal->IsObject() << ", senderVal->IsUndefined() = " << senderVal->IsUndefined() << "\n\n");
        //std::cout<<"\n\nReturning false from decodeVisible 1\n\n";
        return false;
    }
    
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
/*
    v8::Local<v8::Value> typeidVal = senderVal->GetInternalField(TYPEID_FIELD);
    if(typeidVal->IsUndefined())
    {
      SILOG(js, debug, "\n\nJSVisible: Returning false from decodeVisible 2. typeidVal->IsUndefined() = " << typeidVal->IsUndefined() << "\n\n");
      jsObjScript = NULL;
      sporef = NULL;
      sporefVisTo = NULL;
      return false;
    }
    
    v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
    void* ptr = wrapped->Value();
    std::string* typeId = static_cast<std::string*>(ptr);
    std::string typeIdString = *typeId;    

    SILOG(js, debug, "\n\nJSVisible: The typeIdString is " << typeIdString << " \n");
    
    if(typeIdString != VISIBLE_TYPEID_STRING)
    {
      SILOG(js, error, "\n\nJSVisible: Returning false from decodeVisible 2: The typeId field is not visible\n\n");

      jsObjScript = NULL;
      sporef = NULL;
      sporefVisTo = NULL;
      return false;
    }

    //if (senderVal->InternalFieldCount() == VISIBLE_FIELD_COUNT)
    {
        //decode the jsobject script field
        v8::Local<v8::External> wrapJSObj;
        wrapJSObj = v8::Local<v8::External>::Cast(senderVal->GetInternalField(VISIBLE_JSOBJSCRIPT_FIELD));
        void* ptr = wrapJSObj->Value();

        jsObjScript = static_cast<JSObjectScript*>(ptr);
        
        */
        /* bug 187 : visible objects shipped across on the network will not have a jsobjectscript attached .*/
      
      /*
        if (jsObjScript == NULL)
        {

            SILOG(js, info, "\n\nVisible: Decoded Visible with NULL JSObjectScript..Continuing.. \n\n");
        }

        //decode the spaceobjectreference field
        v8::Local<v8::External> wrapSpaceObjRef;
        wrapSpaceObjRef = v8::Local<v8::External>::Cast(senderVal->GetInternalField(VISIBLE_SPACEOBJREF_FIELD));
        
        void* ptr2 = wrapSpaceObjRef->Value();
        sporef = static_cast<SpaceObjectReference*>(ptr2);
        if (sporef == NULL)
        {
            jsObjScript = NULL;
            sporefVisTo = NULL;
            std::cout<<"\n\nReturning false from decodeVisible 2 sporef\n\n";
            return false;
        }


        v8::Local<v8::External> wrapSPVisTo;
        wrapSPVisTo = v8::Local<v8::External>::Cast(senderVal->GetInternalField(VISIBLE_TO_SPACEOBJREF_FIELD));
        
        void* ptr3 = wrapSPVisTo->Value();
        sporefVisTo = static_cast<SpaceObjectReference*>(ptr3);
*/
        /*
        if (sporefVisTo == NULL)
        {
            jsObjScript = NULL;
            sporef =  NULL;
            std::cout<<"\n\nReturning false from decodeVisible 3 proxy object\n\n";
            return false;
        }

        */
/*
        if(sporefVisTo == NULL)
        {
          SILOG(js, warn, "JSVisible: got a visible with no sporefVisTo field. Continuing...");
        }


        return true;
    }

    //std::cout<<"\n\nReturning false from decodeVisible 2 incorrect field count: "<<senderVal->InternalFieldCount()<<"\n\n";
    SILOG(js, debug, "\n\nReturning false from decodeVisible 2 incorrect field count: "<<senderVal->InternalFieldCount()<<"\n\n");
    jsObjScript = NULL;
    sporef = NULL;
    sporefVisTo = NULL;
    return false;
    */
}


}//end jsvisible namespace
}//end js namespace
}//end sirikata


