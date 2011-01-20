#include "JSFakeroot.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"
#include "../JSObjectStructs/JSContextStruct.hpp"
#include "JSFields.hpp"

#include <sirikata/core/util/SpaceObjectReference.hpp>


namespace Sirikata {
namespace JS {
namespace JSFakeroot {

v8::Handle<v8::Value> root_canSendMessage(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_canSendMessage.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->script_canSendMessage();
}


v8::Handle<v8::Value> root_canRecvMessage(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_canRecvMessage.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->script_canRecvMessage();    
}


v8::Handle<v8::Value> root_canProx(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_canProx.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->script_canProx();
}


v8::Handle<v8::Value> root_getPosition(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_getPosition.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->script_getPosition();
}

v8::Handle<v8::Value> root_print(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in root_print.  Requires exactly one argument: a string to print.")));
    
    String errorMessage = "Error decoding the fakeroot object from root_print.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


    v8::String::Utf8Value str(args[0]);
    String toPrint( ToCString(str));
    
    return jsfake->script_print(toPrint);    
}


v8::Handle<v8::Value> root_sendHome(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in root_sendHome.  Requires exactly one argument: an object to send.")));

    if (! args[0]->IsObject())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in root_sendHome.  Requires argument to be an object.")));

    //decode string argument
    v8::Handle<v8::Value> messageBody = args[0];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object in root_sendHome.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    String serialized_message = JSSerializer::serializeObject(v8Object);
    
    
    String errorMessage = "Error decoding the fakeroot object from root_print.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->script_sendHome(serialized_message);
}


v8::Handle<v8::Value> root_registerHandler(const v8::Arguments& args)
{
    std::cout<<"\n\nIn JSFakeroot.cpp.  Haven't finished root_registerHandler.\n\n";
    assert(false);
    return v8::Undefined();
}
v8::Handle<v8::Value> root_timeout(const v8::Arguments& args)
{
    std::cout<<"\n\nIn JSFakeroot.cpp.  Haven't finished root_timeout.\n\n";
    assert(false);
    return v8::Undefined();
}
v8::Handle<v8::Value> root_toString(const v8::Arguments& args)
{
    //note to string probably should not serialize fakeroot object, but instead
    //should just be a keyword for you should do this on your end
    std::cout<<"\n\nIn JSFakeroot.cpp.  Haven't finished root_toString.\n\n";
    assert(false);
    return v8::Undefined();
}






}//end jsfakeroot namespace
}//end js namespace
}//end sirikata


