#include <vector>
#include <boost/any.hpp>
#include <v8.h>


#include "JSFunctionInvokable.hpp"
#include "../JSObjectScript.hpp"
#include "JSInvokableUtil.hpp"

namespace Sirikata
{
namespace JS
{

  boost::any JSFunctionInvokable::invoke(std::vector<boost::any>& params)
  {
    /* Invoke the function handle */

    int argc =
#ifdef _WIN32
               1
#else
               0
#endif
               ;
    int base_offset = argc; // need to work around windows weirdness
   argc += params.size();

   v8::HandleScope handle_scope;
   v8::Context::Scope  context_scope(script_->context());

   v8::Handle<v8::Value> argv[argc];

   if (base_offset) argv[0] = v8::Handle<v8::Value>();

   for(int i = 0; i < params.size(); i++)
       argv[base_offset+i] = InvokableUtil::AnyToV8(script_, params[i]);

  //TryCatch try_catch;

   // We are currently executing in the global context of the entity
   // FIXME: need to take care fo the "this" pointer
   v8::Handle<v8::Value> result = function_->Call(script_->context()->Global(), argc, argv);

   if(result.IsEmpty())
   {
     /*
     v8::String::Utf8Value error(try_catch.Exception());
     const char* cMsg = ToCString(error);
     std::cerr << cMsg << "\n";
     */
   }


   return boost::any(result) ;


  }
}
}
