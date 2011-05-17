
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSHandler.hpp"
#include "../JSObjectStructs/JSEventHandlerStruct.hpp"
#include "JSFields.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSHandler{

/**
   For debugging mostly.  Prints the pattern that this message handler matches
 */
v8::Handle<v8::Value> _printContents(const v8::Arguments& args)
{
    //get the target object whose context owns it.
    //and the pattern and the callback associated with this
    JSObjectScript* caller;
    JSEventHandlerStruct* handler;
    readHandler(args,caller,handler);

    if (handler == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Cannot print contents: handler has already been cleared.")));

    
    //print all handler stuff
    handler->printHandler();
    
    return v8::Undefined();
}

v8::Handle<v8::Value> getAllData(const v8::Arguments& args)
{
    JSObjectScript* caller;
    JSEventHandlerStruct* handler;
    readHandler(args,caller,handler);

    if (handler == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Cannot getAllData: cannot decode handler.")));


    return handler->getAllData();
}


/**
   Calling suspend prevents this handler from being triggered until it's resumed.
 */
v8::Handle<v8::Value> _suspend(const v8::Arguments& args)
{
    JSObjectScript* caller;
    JSEventHandlerStruct* handler;
    readHandler(args,caller,handler);

    if (handler == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Cannot suspend: handler has already been cleared.")));

    handler->suspend();
    
    return v8::Undefined();
}

/**
   Resuming a suspended handler allows it to be triggered in response to message events.
 */
v8::Handle<v8::Value> _resume(const v8::Arguments& args)
{
    JSObjectScript* caller;
    JSEventHandlerStruct* handler;
    readHandler(args,caller,handler);

    if (handler == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Cannot resume: handler has already been cleared.")));
    
    handler->resume();
    
    return v8::Undefined();
}

/**
   @return Returns a boolean for whether the handler is currently suspended or not.
 */
v8::Handle<v8::Value> _isSuspended(const v8::Arguments& args)
{
    JSObjectScript* caller;
    JSEventHandlerStruct* handler;
    readHandler(args,caller,handler);

    if (handler == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Cannot suspend: handler has already been cleared.")));
    
    return v8::Boolean::New(handler->getIsSuspended());
}

/**
   When clear a handler, handler will never be triggered again (even if call
   resume on it).
 */
v8::Handle<v8::Value> _clear(const v8::Arguments& args)
{
    JSObjectScript* caller;
    JSEventHandlerStruct* handler;
    readHandler(args,caller,handler);

    if (handler == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Cannot clear: handler has already been cleared.")));
 
    //handler has not been deleted, and we need to do so now inside of
    //JSObjectScript (so can also remove from event handler vector).
    caller->deleteHandler(handler);
    
    //set the internal field of the vector to be null
    setNullHandler(args);
    
    return v8::Undefined();
}

void setNullHandler(const v8::Arguments& args)
{
    v8::Handle<v8::Object> mHand = args.This();

   //grabs the internal pattern
   //(which has been saved as a pointer to JSEventHandler
   v8::Local<v8::External> wrapEventHand;
   if (mHand->InternalFieldCount() > 0)
       mHand->SetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD,External::New(NULL));
   else
       v8::Handle<v8::Object>::Cast(mHand->GetPrototype())->SetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD, External::New(NULL));
}


//utility function for working with handler objects.  pass in args for a
//call, get back the jsobjectscript that called it, and the matching pattern and
//callback function that are associated with the handler objects.
void readHandler(const v8::Arguments& args, JSObjectScript*& caller, JSEventHandlerStruct*& hand)
{
   v8::Local<v8::Object> mHand = args.This();

   //grabs the caller
   v8::Local<v8::External> wrapJSObj;
   if (mHand->InternalFieldCount() > 0)
       wrapJSObj = v8::Local<v8::External>::Cast(
           mHand->GetInternalField(JSHANDLER_JSOBJSCRIPT_FIELD)
       );
   else
       wrapJSObj = v8::Local<v8::External>::Cast(
           v8::Handle<v8::Object>::Cast(mHand->GetPrototype())->GetInternalField(JSHANDLER_JSOBJSCRIPT_FIELD)
       );
   void* ptr = wrapJSObj->Value();
   caller = static_cast<JSObjectScript*>(ptr);


   //grabs the internal pattern
   //(which has been saved as a pointer to JSEventHandler
   v8::Local<v8::External> wrapEventHand;
   if (mHand->InternalFieldCount() > 0)
       wrapEventHand = v8::Local<v8::External>::Cast(
           mHand->GetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD)
       );
   else
       wrapEventHand = v8::Local<v8::External>::Cast(
           v8::Handle<v8::Object>::Cast(mHand->GetPrototype())->GetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD)
       );
   void* ptr2 = wrapEventHand->Value();
   hand = static_cast<JSEventHandlerStruct*>(ptr2);
}



}}}//end namespaces
