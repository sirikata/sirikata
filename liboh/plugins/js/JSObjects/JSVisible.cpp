#include "JSVisible.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"

#include "JSFields.hpp"

#include <sirikata/core/util/SpaceObjectReference.hpp>


namespace Sirikata {
namespace JS {
namespace JSVisible {


v8::Handle<v8::Value> getPosition(const v8::Arguments& args)
{
    if (args.Length() != 0)
    {
        std::cout<<"\n\n\n";
        std::cout<<"This is args.Length(): "<<args.Length()<<"\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to getPosition method of Visible")));
    }
    
    JSObjectScript*         caller;
    SpaceObjectReference*   sporef;
    SpaceObjectReference*   spVisTo;
    
    if ( ! decodeVisible(args.This(), caller,sporef,spVisTo))
    {
        std::cout<<"\n\nInside of getPosition function\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through a visible object")) );
    }

    return caller->returnProxyPosition(sporef,spVisTo);

}


v8::Handle<v8::Value> toString(const v8::Arguments& args)
{
    if (args.Length() != 0)
    {
        std::cout<<"\n\n\n";
        std::cout<<"This is args.Length(): "<<args.Length()<<"\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to toString method of Visible")));
    }
    
    JSObjectScript*         caller;
    SpaceObjectReference*   sporef;
    ProxyObjectPtr               p;
    
    if ( ! decodeVisible(args.This(), caller,sporef,p))
    {
        std::cout<<"\n\nInside of toString function\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through a visible object")) );
    }

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
    SpaceObjectReference* spVisTo;
    if (! decodeVisible(args[0],caller,sporef,spVisTo))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through a visible object")) );


    std::cout << "Printing Object Reference :" << sporef->toString() << "\n";
    caller->printPosition(sporef,spVisTo);

    lkjs;    
    return v8::Undefined();
}



v8::Handle<v8::Value> __visibleSendMessage (const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sendMessage(<object>)")) );

    //first need to extract out the sending jsobjectscript and oref
    JSObjectScript* caller;
    SpaceObjectReference* sporef;
    SpaceObjectReference* spVisTo;
    
    if (! decodeVisible(args.This(),caller,sporef,spVisTo))
    {
        std::cout<<"\n\nInside of visibleSendMessageFunction\n\n";
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters: require you to send through an visible object")) );
    }
        
    //then need to read the object
    v8::Handle<v8::Value> messageBody = args[0];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    std::string serialized_message = JSSerializer::serializeObject(v8Object);


    //actually send the message to the entity
    caller->sendMessageToEntity(sporef,spVisTo,serialized_message);
    
    return v8::Undefined();
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
        std::cout<<"\n\nReturning false from decodeVisible 1\n\n";
        return false;
    }
    
    v8::Handle<v8::Object>senderer = v8::Handle<v8::Object>::Cast(senderVal);
    return decodeVisible(senderer,jsObjScript,sporef,p);
}


bool decodeVisible(v8::Handle<v8::Object> senderVal, JSObjectScript*& jsObjScript, SpaceObjectReference*& sporef, SpaceObjectReference*&  sporefVisTo )
{
    if (senderVal->InternalFieldCount() == VISIBLE_FIELD_COUNT)
    {
        //decode the jsobject script field
        v8::Local<v8::External> wrapJSObj;
        wrapJSObj = v8::Local<v8::External>::Cast(senderVal->GetInternalField(VISIBLE_JSOBJSCRIPT_FIELD));
        void* ptr = wrapJSObj->Value();

        jsObjScript = static_cast<JSObjectScript*>(ptr);
        if (jsObjScript == NULL)
        {
            sporef = NULL;
            sporefVisTo = NULL;
            std::cout<<"\n\nReturning false from decodeVisible 2: jsobject script\n\n";
            return false;
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
        if (sporefVisTo == NULL)
        {
            jsObjScript = NULL;
            sporef =  NULL;
            std::cout<<"\n\nReturning false from decodeVisible 3 proxy object\n\n";
            return false;
        }


        return true;
    }

    std::cout<<"\n\nReturning false from decodeVisible 2 incorrect field count: "<<senderVal->InternalFieldCount()<<"\n\n";
    jsObjScript = NULL;
    sporef = NULL;
    sporefVisTo = NULL;
    return false;
}


}//end jsvisible namespace
}//end js namespace
}//end sirikata


