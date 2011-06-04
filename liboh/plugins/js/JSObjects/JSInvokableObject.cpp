#include "JSInvokableObject.hpp"
#include "../EmersonScript.hpp"
#include "JSFields.hpp"
#include "JSFunctionInvokable.hpp"
#include "JSInvokableUtil.hpp"

#include <sirikata/core/network/Asio.hpp>
#include <cassert>
#include <vector>
namespace Sirikata
{
namespace JS
{
namespace JSInvokableObject
{

v8::Handle<v8::Value> invoke(const v8::Arguments& args)
{
 /* Decode the args array and send the call to the internal object  */
    EmersonScript* caller;
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
      params.push_back(InvokableUtil::V8ToAny(caller, args[i]));

  /* This is just a trampoline pattern */

  boost::any b = invokableObj->invoke(params);
  if(b.empty())
  {
    return v8::Undefined();
  }
  Handle<Value> retval = InvokableUtil::AnyToV8(caller, b);
  return retval;
}

boost::any JSInvokableObjectInt::invoke(std::vector<boost::any> &params)
{
  /* Invoke the invokable version */
    SILOG(js,detailed,"JSInvokableObjectInt::invoke(): invokable_ type is " << typeid(invokable_).name());
  return invokable_->invoke(params);
}

bool decodeJSInvokableObject(v8::Handle<v8::Value> senderVal, EmersonScript*& emerScript, JSInvokableObjectInt*& simObj)
{

   if ((!senderVal->IsObject()) || (senderVal->IsUndefined()))
    {
        emerScript = NULL;
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
    emerScript = static_cast<EmersonScript*>(ptr);

    if (emerScript == NULL)
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
      emerScript = NULL;
      return false;
    }

    return true;

}


}
}
}
