#include "JSMath.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"
#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include "Addressable.hpp"
#include <math.h>

#include <core/util/Random.hpp>



namespace Sirikata{
namespace JS{
namespace JSMath{


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




//returns a random float from 0 to 1
v8::Handle<v8::Value> ScriptRandFunction(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to rand.")) );

    float rander = randFloat(0,1);
    return v8::Number::New(rander);
}


//takes in a single argument
//returns a float
v8::Handle<v8::Value> ScriptSqrtFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sqrt.")) );

    v8::Handle<v8::Value> toSqrt = args[0];
    
    double d_toSqrt = NumericExtract(toSqrt);
    

    //return v8::Handle<v8::Number>::New(sqrt(d_toSqrt));
    return v8::Number::New(sqrt(d_toSqrt));
}

v8::Handle<v8::Value> ScriptAcosFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to acos.")) );

    v8::Handle<v8::Value> toSqrt = args[0];
    
    double d_toSqrt = NumericExtract(toSqrt);
    
    
    std::cout<<"\n\nThis is number to acos: "<<d_toSqrt<<"\n\n";
    std::cout.flush();

    //return v8::Handle<v8::Number>::New(sqrt(d_toSqrt));
    return v8::Number::New(acos(d_toSqrt));
}

v8::Handle<v8::Value> ScriptCosFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to cos.")) );

    v8::Handle<v8::Value> toSqrt = args[0];
    
    double d_toSqrt = NumericExtract(toSqrt);
    
    
    std::cout<<"\n\nThis is number to cos: "<<d_toSqrt<<"\n\n";
    std::cout.flush();

    //return v8::Handle<v8::Number>::New(sqrt(d_toSqrt));
    return v8::Number::New(cos(d_toSqrt));
}

v8::Handle<v8::Value> ScriptSinFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sin.")) );

    v8::Handle<v8::Value> toSqrt = args[0];
    
    double d_toSqrt = NumericExtract(toSqrt);
    
    
    std::cout<<"\n\nThis is number to sin: "<<d_toSqrt<<"\n\n";
    std::cout.flush();

    return v8::Number::New(sin(d_toSqrt));
}

v8::Handle<v8::Value> ScriptAsinFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to asin.")) );

    v8::Handle<v8::Value> toSqrt = args[0];
    
    double d_toSqrt = NumericExtract(toSqrt);
    
    
    std::cout<<"\n\nThis is number to asin: "<<d_toSqrt<<"\n\n";
    std::cout.flush();

    //return v8::Handle<v8::Number>::New(sqrt(d_toSqrt));
    return v8::Number::New(asin(d_toSqrt));
}


}
}
}//sirikata namespace
