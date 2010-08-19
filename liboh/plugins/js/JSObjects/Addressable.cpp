#include "Addressable.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"

#include "JSFields.hpp"

#include <sirikata/core/util/SpaceObjectReference.hpp>


namespace Sirikata {
namespace JS {
namespace JSAddressable {



v8::Handle<v8::Value> toString(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to toString method of Addressable")));
    
    JSObjectScript*         caller;
    SpaceObjectReference*   sporef;
  
  //readORef(args,caller,oref);

  if ( ! decodeAddressable(args[0], caller,sporef))
      return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through an addressable object")) );


  //std::string s = oref->toString();
  std::string s = sporef->toString();
  v8::Local<v8::String> ret = v8::String::New(s.c_str(), s.length());
  v8::Persistent<v8::String> pret = v8::Persistent<v8::String>::New(ret); 

  return pret;
}


v8::Handle<v8::Value> __debugRef(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to __debugRef")) );
    
    JSObjectScript* caller;
    SpaceObjectReference* sporef;
    //readORef(args,caller,oref);
    if (! decodeAddressable(args[0],caller,sporef))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through an addressable object")) );


    std::cout << "Printing Object Reference :" << sporef->toString() << "\n";
    return v8::Undefined();
}



v8::Handle<v8::Value> __addressableSendMessage (const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sendMessage(<object>)")) );

    //first need to extract out the sending jsobjectscript and oref
    JSObjectScript* caller;
    SpaceObjectReference* sporef;
    //lkjs: may need to actually pass in args.This.
    if (! decodeAddressable(args[0],caller,sporef))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through an addressable object")) );
        

        
    //then need to read the object
    v8::Handle<v8::Value> messageBody = args[0];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    std::string serialized_message = JSSerializer::serializeObject(v8Object);


    //actually send the message to the entity
    caller->sendMessageToEntity(sporef,serialized_message);
    
    return v8::Undefined();
}


//utility function for working with addressable objects.  pass in args for a
//call, get back the jsobjectscript that called it, and the object reference
//that was the addressable
void readORef(const v8::Arguments& args, JSObjectScript*& caller, ObjectReference*& oref)
{
   // v8::Local<v8::Object> mRef = args.This();

   // //grabs the caller
   // v8::Local<v8::External> wrapJSObj;
   // if (mRef->InternalFieldCount() > 0)
   //     wrapJSObj = v8::Local<v8::External>::Cast(
   //         mRef->GetInternalField(OREF_JSOBJSCRIPT_FIELD)
   //     );
   // else
   //     wrapJSObj = v8::Local<v8::External>::Cast(
   //         v8::Handle<v8::Object>::Cast(mRef->GetPrototype())->GetInternalField(OREF_JSOBJSCRIPT_FIELD)
   //     );
   // void* ptr = wrapJSObj->Value();
   // caller = static_cast<JSObjectScript*>(ptr);


   // //grabs the oref
   //  v8::Local<v8::External> wrapORef;
   //  if (mRef->InternalFieldCount() > 0)
   //      wrapORef = v8::Local<v8::External>::Cast(
   //          mRef->GetInternalField(OREF_OREF_FIELD)
   //      );
   //  else
   //      wrapORef = v8::Local<v8::External>::Cast(
   //          v8::Handle<v8::Object>::Cast(mRef->GetPrototype())->GetInternalField(OREF_OREF_FIELD)
   //      );
   //  void* ptr2 = wrapORef->Value();
   //  oref = static_cast<ObjectReference*>(ptr2);
}


//returns true if the object is a valid sender object
//utility function for working with addressable objects.  pass in args for a
//call, get back the jsobjectscript that called it, and the space object
//reference that was addressable
//bool decodeAddressable(v8::Handle<v8::Value> sender_val, JSObjectScript*&
//jsObjScript, SpaceObjectReference*& sporef)
bool decodeAddressable(v8::Handle<v8::Value> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef)
{
    if (!senderVal->IsObject() && !senderVal->IsNull() && !senderVal->IsUndefined())
    {
        jsObjScript = NULL;
        sporef = NULL;
        return false;
    }

    v8::Handle<v8::Object>senderer = v8::Handle<v8::Object>::Cast(senderVal);
    return decodeAddressable(senderer,jsObjScript,sporef);
}


bool decodeAddressable(v8::Handle<v8::Object> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef)
{
    //   if (!sender_val->IsObject() && !sender_val->IsNull() &&
    //   !sender_val->IsUndefined())
    // if (!senderVal->IsObject() && !senderVal->IsNull() && !senderVal->IsUndefined())
    // {
    //     jsObjScript = NULL;
    //     sporef = NULL;
    //     return false;
    // }
    
    
    if (senderVal->InternalFieldCount() == ADDRESSABLE_FIELD_COUNT)
    {
        //decode the jsobject script field
        v8::Local<v8::External> wrapJSObj;
        wrapJSObj = v8::Local<v8::External>::Cast(senderVal->GetInternalField(ADDRESSABLE_JSOBJSCRIPT_FIELD));
        void* ptr = wrapJSObj->Value();

        jsObjScript = static_cast<JSObjectScript*>(ptr);
        if (jsObjScript == NULL)
        {
            sporef = NULL;
            return false;
        }

        //decode the spaceobjectreference field
        v8::Local<v8::External> wrapSpaceObjRef;
        wrapSpaceObjRef = v8::Local<v8::External>::Cast(senderVal->GetInternalField(ADDRESSABLE_SPACEOBJREF_FIELD));
        
        void* ptr2 = wrapSpaceObjRef->Value();

        sporef = static_cast<SpaceObjectReference*>(ptr2);
        if (jsObjScript == NULL)
        {
            jsObjScript = NULL;
            return false;
        }

        return true;
    }


    jsObjScript = NULL;
    sporef = NULL;
    return false;
}


}//end jsaddressable namespace
}//end js namespace
}//end sirikata


