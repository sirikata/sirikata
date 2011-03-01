#include "JSInvokableObject.hpp"
#include "../JSObjectScript.hpp"
#include "JSFields.hpp"
#include "JSFunctionInvokable.hpp"

#include <cassert>
#include <vector>
namespace Sirikata
{
namespace JS
{
namespace JSInvokableObject
{


namespace {

/** Converts a V8 object into a boost:any, leaving the boost::any empty if the
 *  object cannot be translated.
 */
boost::any V8ToAny(JSObjectScript* parent, v8::Handle<v8::Value> val) {
    /* Pushing only string params for now */
    if(val->IsString())
    {
      v8::String::AsciiValue str(val);
      string s = string(*str);
      return boost::any(s);
    }
    else if(val->IsFunction())
    {
      v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(val);
      v8::Persistent<v8::Function> function_persist = v8::Persistent<v8::Function>::New(function);

      JSFunctionInvokable* invokable = new JSFunctionInvokable(function_persist, parent);
      Invokable* in = invokable;
      return boost::any(in);
    }
    return boost::any();
}

} // namespace

v8::Handle<v8::Value> invoke(const v8::Arguments& args)
{
 /* Decode the args array and send the call to the internal object  */
  JSObjectScript* caller;
  JSInvokableObjectInt* invokableObj;

  bool decoded = decodeJSInvokableObject(args.This(), caller, invokableObj);
  assert(decoded);


  assert(invokableObj);

  /*
  create a vector of boost::any params here
  For now just take the first param and see that

  FIXME

  */
  std::vector<boost::any> params;
  HandleScope scope;

  //assert(args.Length() == 1);
  for(int i =0; i < args.Length(); i++)
      params.push_back(V8ToAny(caller, args[i]));

  /* This is just a trampoline pattern */

  boost::any b = invokableObj->invoke(params);
  if(b.empty())
  {
    return v8::Undefined();
  }
  Invokable* newInvokableObj = boost::any_cast<Invokable*>(b);
  //Invokable* newInvokableObj = (boost::unsafe_any_cast<Invokable>(&b) );  //boost::any_cast<*>( invokableObj->invoke(params) );

  Local<Object> tmpObj = caller->manager()->mInvokableObjectTemplate->NewInstance();
  Persistent<Object>tmpObjP = Persistent<Object>::New(tmpObj);
  tmpObjP->SetInternalField(JSINVOKABLE_OBJECT_JSOBJSCRIPT_FIELD,External::New(caller));
  tmpObjP->SetInternalField(JSINVOKABLE_OBJECT_SIMULATION_FIELD,External::New(  new JSInvokableObjectInt(newInvokableObj) ));
  tmpObjP->SetInternalField(TYPEID_FIELD,External::New(  new String (JSINVOKABLE_TYPEID_STRING)));

  return tmpObj;
}

boost::any JSInvokableObjectInt::invoke(std::vector<boost::any> &params)
{
  /* Invoke the invokable version */
    SILOG(js,detailed,"JSInvokableObjectInt::invoke(): invokable_ type is " << typeid(invokable_).name());
  return invokable_->invoke(params);
}

bool decodeJSInvokableObject(v8::Handle<v8::Value> senderVal, JSObjectScript*& jsObjScript, JSInvokableObjectInt*& simObj)
{

   if ((!senderVal->IsObject()) || (senderVal->IsUndefined()))
    {
        jsObjScript = NULL;
        simObj = NULL;
        return false;
    }



    v8::Handle<v8::Object>sender = v8::Handle<v8::Object>::Cast(senderVal);

    if (sender->InternalFieldCount() != JSINVOKABLE_OBJECT_TEMPLATE_FIELD_COUNT)
    {
      return false;
    }

    v8::Local<v8::External> wrapJSObj;
    wrapJSObj = v8::Local<v8::External>::Cast(sender->GetInternalField(JSINVOKABLE_OBJECT_JSOBJSCRIPT_FIELD));

    void* ptr = wrapJSObj->Value();
    jsObjScript = static_cast<JSObjectScript*>(ptr);

    if (jsObjScript == NULL)
    {
        simObj = NULL;
        return false;
    }

    v8::Local<v8::External> wrapSimObjRef;
    wrapSimObjRef = v8::Local<v8::External>::Cast(sender->GetInternalField(JSINVOKABLE_OBJECT_SIMULATION_FIELD));
    void* ptr2 = wrapSimObjRef->Value();
    simObj = static_cast<JSInvokableObject::JSInvokableObjectInt*>(ptr2);

    if(!simObj)
    {
      jsObjScript = NULL;
      return false;
    }

    return true;

}


}
}
}
