#include "JSUtilObj.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"
#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include <math.h>

#include <sirikata/core/util/Random.hpp>



namespace Sirikata{
namespace JS{
namespace JSUtilObj{




v8::Handle<v8::Value> ScriptCreateWatched(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWatched of JSUtilObj.cpp.  Watched constructor takes no args.")));

    
    String errorMessage = "Error in ScriptCreateWatched of JSUtilObj.cpp.  Cannot decode the jsobjscript field of the util object.  ";
    JSObjectScript* jsobj = JSObjectScript::decodeUtilObject(args.This(),errorMessage);
    
    return jsobj->createWatched();
}

//when:
//1: a predicate function to check
//2: a callback function to callback
//3: null or a sampling period
//4-end: list of watchable args this depends on
//last few arguments are 
v8::Handle<v8::Value> ScriptCreateWhen(const v8::Arguments& args)
{
    if (args.Length() <4)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateWhen of JSUtilObj.cpp.  Requires at least 4 args: <predicate function> <callback function> <sampling period (or null if no sampling)> <first watchable arg> <second watchable arg> ...")) );

    //check can get jsobjscript passed in
    String errorMessage = "Error in ScriptCreateWhen of JSUtilObj.  Cannot decode jsobjectscript associated with util object. ";
    JSObjectScript* jsobj = JSObjectScript::decodeUtilObject(args.This(),errorMessage);
    if (jsobj == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));
    
    //check args passed in
    if (! args[0]->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateWhen of JSUtilObj.cpp.  First argument should be a function (predicate of when statement).")));
    if (! args[1]->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateWhen of JSUtilObj.cpp.  Second argument should be a function (callback of when).")));
    if ((! args[2]->IsNull()) && (! args[2]->IsNumber()))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateWhen of JSUtilObj.cpp.  Third argument should be a number or null (min sampling period).")));

    WatchableMap watchMap;
    
    //check all of the watchable args
    for (int s=3; s < args.Length(); ++s)
    {
        String errorMessage = "Error decoding watchable arg of in ScriptWhenCreate of JSUtilObj.cpp.  ";
        JSWatchable* watchableArg = decodeWatchable(args[s],errorMessage);
        if (watchableArg ==NULL)
            return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

        watchMap[watchableArg] = true;
    }
    
    v8::Persistent<v8::Function> per_pred = v8::Persistent<v8::Function>::New(v8::Handle<v8::Function>::Cast(args[0]));
    v8::Persistent<v8::Function> per_cb   = v8::Persistent<v8::Function>::New(v8::Handle<v8::Function>::Cast(args[1]));

    
    float minPeriod = JSWhenStruct::WHEN_PERIOD_NOT_SET;
    if (args[2]->IsNumber())
        minPeriod = NumericExtract(args[2]);


    //create new when object;
    return jsobj->create_when(per_pred,per_cb,minPeriod,watchMap);
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
        return v8::ThrowException( v8::Exception::Error(v8::String::New("sqrt function requires one argument.")) );

    v8::Handle<v8::Value> toSqrt = args[0];
    
    double d_toSqrt = NumericExtract(toSqrt);

    if (d_toSqrt < 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Invalid parameters passed to sqrt.  Argument must be >=0.")) );

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

v8::Handle<v8::Value> ScriptPowFunction(const v8::Arguments& args)
{
    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: power function requires two arguments.  Expecting <base> and <exponent>")) );

    v8::HandleScope handle_scope;
    double base     = NumericExtract(args[0]);
    double exponent = NumericExtract(args[1]);

    double returner = pow(base,exponent);
    std::cout<<"\n\nThis is the value of the math: "<<returner<<"\n\n";
    std::cout.flush();
    
    return v8::Number::New( returner );
}



v8::Handle<v8::Value> ScriptAbsFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: abs function requires a single argument.  Expecting <number to take absolute value of>")) );

    v8::HandleScope handle_scope;
    double numToAbs     = NumericExtract(args[0]);

    return v8::Number::New( fabs(numToAbs) );
    
}


}//JSUtilObj namespace
}//JS namespace
}//sirikata namespace
