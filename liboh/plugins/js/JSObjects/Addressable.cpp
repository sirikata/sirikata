#include "Addressable.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"

#include "JSFields.hpp"
#include "JSObjectsUtils.hpp"
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include "../JSPresenceStruct.hpp"

namespace Sirikata {
namespace JS {
namespace JSAddressable {



v8::Handle<v8::Value> toString(const v8::Arguments& args)
{
    if (args.Length() != 0)
    {
        std::cout<<"\n\n\n";
        std::cout<<"This is args.Length(): "<<args.Length()<<"\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to toString method of Addressable")));
    }
    
    JSObjectScript*         caller;
    SpaceObjectReference*   sporef;

    if ( ! decodeAddressable(args.This(), caller,sporef))
    {
        std::cout<<"\n\nInside of toString function\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through an addressable object")) );
    }

    

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
        
    if (! decodeAddressable(args.This(),caller,sporef))
    {
        std::cout<<"\n\nInside of addressableSendMessageFunction\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through an addressable object")) );
    }
        
    //then need to read the object
    v8::Handle<v8::Value> messageBody = args[0];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    std::string serialized_message = JSSerializer::serializeObject(v8Object);
    JSPresenceStruct* jsps = getPresStructFromArgs(args);
    if (jsps == NULL)
    {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message not being sent from a valid presence in addressableSendMessage.")) );
    }
    
    //actually send the message to the entity
    caller->sendMessageToEntity(sporef,jsps->sporef,serialized_message);
    
    return v8::Undefined();
}


//returns true if the object is a valid sender object
//utility function for working with addressable objects.  pass in args for a
//call, get back the jsobjectscript that called it, and the space object
//reference that was addressable
//bool decodeAddressable(v8::Handle<v8::Value> sender_val, JSObjectScript*&
//jsObjScript, SpaceObjectReference*& sporef)
bool decodeAddressable(v8::Handle<v8::Value> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef)
{

    if ((!senderVal->IsObject()) || (senderVal->IsUndefined()))
    {
        jsObjScript = NULL;
        sporef = NULL;
        std::cout<<"\n\nReturning false from decodeAddressable 1\n\n";
        return false;
    }
    
    v8::Handle<v8::Object>senderer = v8::Handle<v8::Object>::Cast(senderVal);
    return decodeAddressable(senderer,jsObjScript,sporef);
}


bool decodeAddressable(v8::Handle<v8::Object> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef)
{
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
            std::cout<<"\n\nReturning false from decodeAddressable 2: jsobject script\n\n";
            return false;
        }

        //decode the spaceobjectreference field
        v8::Local<v8::External> wrapSpaceObjRef;
        wrapSpaceObjRef = v8::Local<v8::External>::Cast(senderVal->GetInternalField(ADDRESSABLE_SPACEOBJREF_FIELD));
        
        void* ptr2 = wrapSpaceObjRef->Value();

        sporef = static_cast<SpaceObjectReference*>(ptr2);
        if (sporef == NULL)
        {
            jsObjScript = NULL;
            std::cout<<"\n\nReturning false from decodeAddressable 2 sporef\n\n";
            return false;
        }

        return true;
    }


    std::cout<<"\n\nReturning false from decodeAddressable 2 incorrect field count: "<<senderVal->InternalFieldCount()<<"\n\n";
    jsObjScript = NULL;
    sporef = NULL;
    return false;
}



JSPresenceStruct* getPresStructFromArgs(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    v8::Local<v8::Object> v8Object = args.This();
    v8::Local<v8::External> wrapJSPresStructObj;
    if (v8Object->InternalFieldCount() > 0)
    {
        wrapJSPresStructObj = v8::Local<v8::External>::Cast(
            v8Object->GetInternalField(PRESENCE_FIELD_PRESENCE));
    }
    else
    {
        wrapJSPresStructObj = v8::Local<v8::External>::Cast(
            v8::Handle<v8::Object>::Cast(v8Object->GetPrototype())->GetInternalField(PRESENCE_FIELD_PRESENCE));
    }
    void* ptr = wrapJSPresStructObj->Value();
    JSPresenceStruct* jspres_struct = static_cast<JSPresenceStruct*>(ptr);
    
    if (jspres_struct == NULL)
        assert(false);
        
        return jspres_struct;
    return NULL;
}



}//end jsaddressable namespace
}//end js namespace
}//end sirikata


