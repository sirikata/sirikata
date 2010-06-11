
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSHandler.hpp"
#include "../JSEventHandler.hpp"
#include "JSFields.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSHandler{

v8::Handle<v8::Value> __printContents(const v8::Arguments& args)
{
    //get the target object whose context owns it.
    //and the pattern and the callback associated with this
    JSObjectScript* caller;
    JSEventHandler* handler;
    readHandler(args,caller,handler);

    std::cout<<"\n\n\nPRINTING BOTH\n\n\n";
    //print both
    /*
      FIXME: Do we want to have both print functions within the pattern and
      callback themselves?  Probably not.
     */
    
    return v8::Undefined();
}


//utility function for working with handler objects.  pass in args for a
//call, get back the jsobjectscript that called it, and the matching pattern and
//callback function that are associated with the handler objects.
void readHandler(const v8::Arguments& args, JSObjectScript*& caller, JSEventHandler*& hand)
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
   hand = static_cast<JSEventHandler*>(ptr);
}



}}}//end namespaces
