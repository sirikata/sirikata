#include "Addressable.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"




namespace Sirikata {
namespace JS {
namespace JSAddressable {


v8::Handle<v8::Value> __debugRef(const v8::Arguments& args)
{
    std::cout<<"\n\n\n\nDEBUG: WAKA WAKA\n\n";
	JSObjectScript* caller;
    ObjectReference* oref;
    readORef(args,caller,oref);

	std::cout << "Printing Object Reference :" << oref->toString() << "\n";
    return v8::Undefined();
}



v8::Handle<v8::Value> __addressableSendMessage (const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sendMessage(<object>)")) );

    //first need to extract out the sending jsobjectscript and oref
    JSObjectScript* caller;
    ObjectReference* oref;
    readORef(args,caller,oref);

        
    //then need to read the object
    v8::Handle<v8::Value> messageBody = args[0];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    std::string serialized_message = JSSerializer::serializeObject(v8Object);


    //actually send the message to the entity
    caller->sendMessageToEntity(oref,serialized_message);
    
    return v8::Undefined();
}


//utility function for working with addressable objects.  pass in args for a
//call, get back the jsobjectscript that called it, and the object reference
//that was the addressable
void readORef(const v8::Arguments& args, JSObjectScript*& caller, ObjectReference*& oref)
{
   v8::Local<v8::Object> mRef = args.This();

   //grabs the caller
   v8::Local<v8::External> wrapJSObj;
   if (mRef->InternalFieldCount() > 0)
       wrapJSObj = v8::Local<v8::External>::Cast(
           mRef->GetInternalField(OREF_JSOBJSCRIPT_FIELD)
       );
   else
       wrapJSObj = v8::Local<v8::External>::Cast(
           v8::Handle<v8::Object>::Cast(mRef->GetPrototype())->GetInternalField(OREF_JSOBJSCRIPT_FIELD)
       );
   void* ptr = wrapJSObj->Value();
   caller = static_cast<JSObjectScript*>(ptr);


   //grabs the oref
    v8::Local<v8::External> wrapORef;
    if (mRef->InternalFieldCount() > 0)
        wrapORef = v8::Local<v8::External>::Cast(
            mRef->GetInternalField(OREF_OREF_FIELD)
        );
    else
        wrapORef = v8::Local<v8::External>::Cast(
            v8::Handle<v8::Object>::Cast(mRef->GetPrototype())->GetInternalField(OREF_OREF_FIELD)
        );
    void* ptr2 = wrapORef->Value();
    oref = static_cast<ObjectReference*>(ptr2);
}



}}}//end namespaces

