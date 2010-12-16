#include <vector>
#include <boost/any.hpp>
#include <v8.h>


#include "JSFunctionInvokable.hpp"
#include "../JSObjectScript.hpp"

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

   argc += params.size();            
  
   v8::HandleScope handle_scope;
   v8::Context::Scope  context_scope(script_->context());
   
   v8::Handle<v8::Value> argv[argc];
   
   
   for(int i = 0; i < argc; i++)
   {
     //argv[i] = boost::any_cast< v8::Handle<v8::Value>& >(params[i]);
     
     if( params[i].type() == typeid(std::string) )
     {
       string s = boost::any_cast<std::string&>(params[i]); 
       argv[i] = v8::String::New(s.c_str(), s.length()); 
     }
   }

    
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
