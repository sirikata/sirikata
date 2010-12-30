#include "JSContext.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"
#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include "Addressable.hpp"
#include "../JSContextStruct.hpp"
#include <math.h>
#include <stdlib.h>
#include <float.h>

namespace Sirikata{
namespace JS{
namespace JSContext{




v8::Handle<v8::Value> ScriptTimeout(const v8::Arguments& args)
{
    return v8::Undefined();
}


template<typename WithHolderType>
JSObjectScript* GetTargetJSObjectScript(const WithHolderType& with_holder) {
    v8::Local<v8::Object> self = with_holder.Holder();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    v8::Local<v8::External> wrap;
    if (self->InternalFieldCount() > 0)
        wrap = v8::Local<v8::External>::Cast(
            self->GetInternalField(0)
        );
    else
        wrap = v8::Local<v8::External>::Cast(
            v8::Handle<v8::Object>::Cast(self->GetPrototype())->GetInternalField(0)
        );
    void* ptr = wrap->Value();
    return static_cast<JSObjectScript*>(ptr);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args)
{
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope;
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        v8::String::Utf8Value str(args[i]);
        const char* cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
    return v8::Undefined();
}


v8::Handle<v8::Value> ScriptExecute(const v8::Arguments& args)
{
    if (args.Length() == 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("\nInvalid arguments to ScriptExecute through context.  Require first argument to be a function.  Later arguments should be arguments for that function.\n")));

    //ensure first argument is function.
    v8::Handle<v8::Value> func_args = args[0];
    if (! func_args->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("\nInvalid arguments to ScriptExecute through context.  Require first argument to be a function\n")));

    //FIXME: need to check if the arguments passed on the scriptexecute call match the arguments required by the function;
    
    JSContextStruct* jscontstruct = getContStructFromArgs(args);
    if (jscontstruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("\nInvalid call to ScriptExecute.  Cannot decode JSContextStruct.\n")));

    //first argument is a function
    //subsequent arguments are the arguments to that function


    int argc = args.Length() - 1; //args to function.
    Handle<Value>* argv = new Handle<Value>[argc];
    for (int s=1; s < args.Length(); ++s)
        argv[s-1] = args[s];


    v8::Handle<v8::Function> exec_func = v8::Handle<v8::Function>::Cast(args[0]);
    jscontstruct->executeScript(exec_func,argc, argv);


    std::cout<<"\n\nFIXME: JSContext: free arguments\n\n";
    //should probably free the arguments here;
}


}//jscontext namespace
}//js namespace
}//sirikata namespace
