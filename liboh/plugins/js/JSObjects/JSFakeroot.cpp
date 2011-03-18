#include "JSFakeroot.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"
#include "../JSObjectStructs/JSContextStruct.hpp"
#include "JSFields.hpp"
#include "JSSystem.hpp"
#include "JSObjectsUtils.hpp"
#include "../JSSystemNames.hpp"
#include "../JSObjectStructs/JSFakerootStruct.hpp"

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

    return jsfake->struct_canSendMessage();
}


v8::Handle<v8::Value> root_canRecvMessage(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_canRecvMessage.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canRecvMessage();    
}

v8::Handle<v8::Value> root_canImport(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_canImport.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canImport();
}


v8::Handle<v8::Value> root_canProx(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_canProx.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canProx();
}


v8::Handle<v8::Value> root_import(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Import only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];



    //decode the filename to import from.
    String strDecodeErrorMessage = "Error decoding string as first argument of root_import of jsfakeroot.  ";
    String native_filename; //string to decode to.
    bool decodeStrSuccessful = decodeString(args[0],native_filename,strDecodeErrorMessage);
    if (! decodeStrSuccessful)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(strDecodeErrorMessage.c_str(), strDecodeErrorMessage.length())) );



    //decode the fakeroot object
    String errorMessage = "Error decoding the fakeroot object from root_import.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_import(native_filename);
}

v8::Handle<v8::Value> root_getVersion(const v8::Arguments& args)
{
    return v8::String::New( JSSystemNames::EMERSON_VERSION);
}

v8::Handle<v8::Value> root_getPosition(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the fakeroot object from root_getPosition.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_getPosition();
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
    
    return jsfake->struct_print(toPrint);    
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
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));

    return jsfake->struct_sendHome(serialized_message);
}



v8::Handle<v8::Value> root_getPresence(const v8::Arguments& args)
{
    //check 0 args (shouldn't need any)
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in root_getPresence.  Requires exactly 0 arguments to get a presence from a fakeroot")));

    //decode root
    String errorMessage = "Error decoding the fakeroot object from root_getPresence.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));
    
    
    return jsfake->struct_getPresence();
}


//fake root in context can already send messages to who instantiated it and
//receive messages from who instantiated it.
//messages sent out of it get stamped with a port number automatically

//argument 0: the presence that the context is associated with.  (will use
//this as sender of messages).  If this arg is null, then just passes through
//the parent context's presence
//argument 1: a visible object that can always send messages to.  if null, will
//use same spaceobjectreference as one passed in for arg0.
//argument 2: true/false.  can I send messages to everyone?
//argument 3: true/false.  can I receive messages from everyone?
//argument 4: true/false.  can I make my own prox queries
//argument 5: true/false.  can I import
v8::Handle<v8::Value> root_createContext(const v8::Arguments& args)
{
    if (args.Length() != 6)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: must have three arguments: <presence to send/recv messages from (null if want to push through parent's presence)>, <JSVisible or JSPresence object that can always send messages to><bool can I send to everyone?>, <bool can I receive from everyone?> , <bool, can I make my own proximity queries>, <bool, can I import code>")) );


    bool sendEveryone,recvEveryone,proxQueries,canImport;
    String errorMessageBase = "In ScriptCreateContext.  Trying to decode argument ";
    String errorMessageWhichArg,errorMessage;

    //jsfakeroot decode
    String errorMsgFakeroot  = "Error decoding fakeroot when creating new context.  ";
    JSFakerootStruct* jsfake = JSFakerootStruct::decodeRootStruct(args.This(),errorMsgFakeroot);
    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMsgFakeroot.c_str())));

    
    //jspresstruct decode
    JSPresenceStruct* jsPresStruct = NULL;
    if (args[0]->IsNull())
        jsPresStruct = jsfake->struct_getPresenceCPP();
    else
    {
        errorMessageWhichArg= " 1.  ";
        errorMessage= errorMessageBase + errorMessageWhichArg;
        jsPresStruct = JSPresenceStruct::decodePresenceStruct(args[0],errorMessage);
        if (jsPresStruct == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );
    }


    //getting who can sendTo
    SpaceObjectReference* canSendTo = NULL;
    if (args[1]->IsNull())
        canSendTo = jsPresStruct->getSporef();
    else
    {
        //should try to decode as a jspositionListener.  if decoding fails, throw error
        errorMessageWhichArg= " 2.  ";
        errorMessage= errorMessageBase + errorMessageWhichArg;

        JSPositionListener* jsposlist = decodeJSPosListener(args[1],errorMessage);

        if (jsposlist == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );

        canSendTo = jsposlist->getToListenTo();
    }


    //send everyone decode
    errorMessageWhichArg= " 3.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[2],sendEveryone, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );

    //recv everyone decode
    errorMessageWhichArg= " 4.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[3],recvEveryone, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );


    //recv everyone decode
    errorMessageWhichArg= " 5.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[4],proxQueries, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );

    //import decode
    errorMessageWhichArg= " 6.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[5],canImport, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );



    return jsfake->struct_createContext(canSendTo,sendEveryone,recvEveryone,proxQueries,canImport,jsPresStruct);
}



v8::Handle<v8::Value> root_scriptEval(const v8::Arguments& args)
{
    String errorMessage       = "Error calling eval in context.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct( args.This(), errorMessage);
    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));
    
    return JSSystem::ScriptEvalContext(args, jsfake->getContext());
}


v8::Handle<v8::Value> root_timeout(const v8::Arguments& args)
{
    //just returns the ScriptTimeout function
    String errorMessage      =  "Error decoding fakeroot in root_timeout of JSFakeroot.cpp.  ";
    JSFakerootStruct* jsfake = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));
    
    return JSSystem::ScriptTimeoutContext(args, jsfake->associatedContext);
}



/** Registers a handler to be invoked for events that match the
 *  specified pattern, where the pattern is a list of individual
 *  rules.
 *  Arguments:
 *   pattern[] pattterns: Array of Pattern rules to match
 *   object target: target of callback (this pointer when invoked), or null for
 *                  the global (root) object
 *   function cb: callback to invoke, with event as parameter
 */
v8::Handle<v8::Value> root_registerHandler(const v8::Arguments& args)
{
    if (args.Length() != 4)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().  Need exactly 4 args.  <function, callback to execute when event associated with handler fires>, <target_val (this pointer when invoked)>, <pattern: array of pattern rules to match or null if can match all>, <a sender to match even to>")) );

    // Changing the sequence of the arguments so as to get the same
    // as is generated in emerson

    v8::Handle<v8::Value> cb_val     = args[0];
    v8::Handle<v8::Value> target_val = args[1];
    v8::Handle<v8::Value> pattern    = args[2];
    v8::Handle<v8::Value> sender_val = args[3];


    // Pattern
    PatternList native_patterns;
    if (! pattern->IsNull())
    {
        if (PatternValidate(pattern))
        {
            Pattern single_pattern = PatternExtract(pattern);
            native_patterns.push_back(single_pattern);
        }
        else if (pattern->IsArray())
        {
            v8::Handle<v8::Array> pattern_array( v8::Handle<v8::Array>::Cast(pattern) );
            if (pattern_array->Length() == 0)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Pattern array must contain at least one element.")) );
            for(uint32 pat_idx = 0; pat_idx < pattern_array->Length(); pat_idx++)
            {
            Local<Value> pattern_element = pattern_array->Get(pat_idx);
            if (!PatternValidate(pattern_element))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Found non-pattern element in array of patterns.")) );
            Pattern single_pattern = PatternExtract(pattern_element);
            native_patterns.push_back(single_pattern);
            }
        }
    }
    else
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Pattern argument must be pattern or array of patterns.")) );


    // Target
    if (!target_val->IsObject() && !target_val->IsNull() && !target_val->IsUndefined())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Target is not object or null.")) );

    v8::Handle<v8::Object> target = v8::Handle<v8::Object>::Cast(target_val);
    v8::Persistent<v8::Object> target_persist = v8::Persistent<v8::Object>::New(target);

    // Sender
    if (! sender_val->IsNull())  //means that it's a valid sender
    {
        String errorMessage = "[JS] Error in ScriptRegisterHandler of JSSystem.cpp.  Having trouble decoding sender.  ";
        JSPositionListener* jsposlist = decodeJSPosListener(sender_val,errorMessage);
        
        if (jsposlist == NULL)
            return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));
    }


    v8::Handle<v8::Object> sender = v8::Handle<v8::Object>::Cast(sender_val);
    v8::Persistent<v8::Object> sender_persist = v8::Persistent<v8::Object>::New(sender);


    // Function
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().  Must contain callback function.")) );


    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);

    String errorMessage = "Error decoding JSObjectScript from system object in ScriptRegisterHandler of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    
    //now decode fakeroot
    String errorMessageDecodeRoot = "Error decoding the fakeroot object from root_registerHandler.  ";
    JSFakerootStruct* jsfake  = JSFakerootStruct::decodeRootStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageDecodeRoot.c_str())));

    return jsfake->struct_makeEventHandlerObject(native_patterns, target_persist, cb_persist, sender_persist);
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


