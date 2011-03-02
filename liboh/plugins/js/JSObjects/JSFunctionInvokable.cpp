#include <vector>
#include <boost/any.hpp>
#include <v8.h>


#include "JSFunctionInvokable.hpp"
#include "../JSObjectScript.hpp"

namespace Sirikata
{
namespace JS
{

namespace {
/** Converts a boost::any into a V8 object, or returns undefined if the object
 *  can't be translated.
 */
v8::Handle<v8::Value> AnyToV8(const boost::any& val) {
    if(val.type() == typeid(std::string) )
    {
        string s = boost::any_cast<std::string>(val);
        return v8::String::New(s.c_str(), s.length());
    }
    else if (val.type() == typeid(float)) {
        double d = boost::any_cast<float>(val);
        return v8::Number::New(d);
    }
    else if (val.type() == typeid(double)) {
        double d = boost::any_cast<double>(val);
        return v8::Number::New(d);
    }
    else if (val.type() == typeid(uint8)) {
        uint32 d = boost::any_cast<uint8>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(uint16)) {
        uint32 d = boost::any_cast<uint16>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(uint32)) {
        uint32 d = boost::any_cast<uint32>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(uint64)) {
        uint32 d = boost::any_cast<uint64>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(int8)) {
        int32 d = boost::any_cast<int8>(val);
        return v8::Int32::New(d);
    }
    else if (val.type() == typeid(int16)) {
        int32 d = boost::any_cast<int16>(val);
        return v8::Int32::New(d);
    }
    else if (val.type() == typeid(int32)) {
        int32 d = boost::any_cast<int32>(val);
        return v8::Int32::New(d);
    }
    else if (val.type() == typeid(int64)) {
        int32 d = boost::any_cast<int64>(val);
        return v8::Int32::New(d);
    }
    else if (val.type() == typeid(bool)) {
        bool b = boost::any_cast<bool>(val);
        return v8::Boolean::New(b);
    }
    else if (val.type() == typeid(Invokable::Array)) {
        Invokable::Array native_arr = boost::any_cast<Invokable::Array>(val);
        v8::Local<v8::Array> arr = v8::Array::New();
        for(int ii = 0; ii < native_arr.size(); ii++) {
            v8::Handle<v8::Value> rhs = AnyToV8(native_arr[ii]);
            if (!rhs.IsEmpty())
                arr->Set(ii, rhs);
        }
        return arr;
    }
    else if (val.type() == typeid(Invokable::Dict)) {
        Invokable::Dict native_dict = boost::any_cast<Invokable::Dict>(val);
        v8::Local<v8::Object> dict = v8::Object::New();
        for(Invokable::Dict::const_iterator di = native_dict.begin(); di != native_dict.end(); di++) {
            v8::Handle<v8::Value> rhs = AnyToV8(di->second);
            if (!rhs.IsEmpty())
                dict->Set(v8::String::New(di->first.c_str()), rhs);
        }
        return dict;
    }

    return v8::Handle<v8::Value>();
}

} // namespace


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
       argv[base_offset+i] = AnyToV8(params[i]);

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
