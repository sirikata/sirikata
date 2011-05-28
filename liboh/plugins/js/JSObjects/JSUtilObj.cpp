#include "JSUtilObj.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"
#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include "../JSObjectStructs/JSUtilStruct.hpp"
#include "../JSObjectStructs/JSVisibleStruct.hpp"
#include "../JSObjectStructs/JSPresenceStruct.hpp"
#include <math.h>

#include <sirikata/core/util/Random.hpp>



namespace Sirikata{
namespace JS{
namespace JSUtilObj{

/**
   Overloads the '-' operator for many types.  a and b must be of the same type
   (either vectors or numbers).  If a and b are vectors (a =
   <ax,ay,az>; b = <bx,by,bz>, returns <ax-bx, ay-by, az-bz>).  If a and b are
   numbers, returns a - b.

   @param a Of type vector or number.
   @param b Of type vector or number.
 */
v8::Handle<v8::Value> ScriptMinus(const v8::Arguments& args)
{
    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: minu requires two arguments.")) );

    String dummyErr;
    //check if numbers
    bool isNum1, isNum2;
    float num1,num2;
    isNum1 = NumericValidate(args[0]);
    if (isNum1)
    {
        isNum2 = NumericValidate(args[1]);
        if (! isNum2)
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: minus requires two arguments of same type.  First argument is number.  Second argument is not.")) );


        num1 = NumericExtract(args[0]);
        num2 = NumericExtract(args[1]);

        return v8::Number::New(num1-num2);
    }

    //check if vecs
    Vector3d vec1,vec2;
    if (Vec3ValValidate(args[0]))
    {
        if (! Vec3ValValidate(args[1]))
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: minus requires two arguments of same type.  First argument is vec3.  Second argument is not.")) );

        v8::HandleScope handle_scope;
        v8::Handle<v8::Object> o1,o2;
        o1 = args[0]->ToObject();
        o2 = args[1]->ToObject();
        vec1 = Vec3Extract(o1);
        vec2 = Vec3Extract(o2);

        Handle<Value> CreateJSResult_Vec3Impl(v8::Handle<v8::Context>& ctx, const Vector3d& src);

        String errMsg = "Error in JSUtilObj.cpp when trying to subtract.  Could not decode util struct.  ";
        JSUtilStruct* jsutil = JSUtilStruct::decodeUtilStruct(args.This() ,errMsg);

        if (jsutil == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str())) );

        vec1 = vec1-vec2;

        return jsutil->struct_createVec3(vec1);
    }

    return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: minus requires two arguments.  Both must either be vectors or numbers.")) );
}


/**
   Overloads the '+' operator for many types.  a and b must be of the same type
   (either vectors, numbers, or strings).  If a and b are vectors (a =
   <ax,ay,az>; b = <bx,by,bz>, returns <ax+bx, ay+by, az+bz>).  If a and b are
   numbers, returns a + b.  If a and b are strings, returns concatenated string.

   @param a Of type vector, number, or string.
   @param b Of type vector, number, or string.
 */
v8::Handle<v8::Value> ScriptPlus(const v8::Arguments& args)
{
    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: plus requires two arguments.")) );

    String dummyErr;
    //check if numbers
    bool isNum1, isNum2;
    float num1,num2;
    isNum1 = NumericValidate(args[0]);
    if (isNum1)
    {
        isNum2 = NumericValidate(args[1]);
        if (! isNum2)
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: plus requires two arguments of same type.  First argument is number.  Second argument is not.")) );


        num1 = NumericExtract(args[0]);
        num2 = NumericExtract(args[1]);

        return v8::Number::New(num1+num2);
    }

    //check if vecs
    Vector3d vec1,vec2;
    if (Vec3ValValidate(args[0]))
    {
        if (! Vec3ValValidate(args[1]))
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: plus requires two arguments of same type.  First argument is vec3.  Second argument is not.")) );

        v8::HandleScope handle_scope;
        v8::Handle<v8::Object> o1,o2;
        o1 = args[0]->ToObject();
        o2 = args[1]->ToObject();
        vec1 = Vec3Extract(o1);
        vec2 = Vec3Extract(o2);

        Handle<Value> CreateJSResult_Vec3Impl(v8::Handle<v8::Context>& ctx, const Vector3d& src);

        String errMsg = "Error in JSUtilObj.cpp when trying to add.  Could not decode util struct.  ";
        JSUtilStruct* jsutil = JSUtilStruct::decodeUtilStruct(args.This() ,errMsg);

        if (jsutil == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str())) );


        vec1 = vec1+vec2;

        return jsutil->struct_createVec3(vec1);
    }



    //check if strings
    String str1,str2;
    bool isStr1, isStr2;
    isStr1 = decodeString(args[0], str1, dummyErr);
    if (isStr1)
    {
        isStr2 = decodeString(args[1], str2, dummyErr);
        if (!isStr2)
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: plus requires two arguments of same type.  First argument is string.  Second argument is not.")) );

        String newStr = str1+str2;
        return v8::String::New(newStr.c_str(), newStr.size());
    }



    return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: plus requires two arguments.  Both must either be vectors, strings, or numbers.")) );
}



/**
  @return a random float from 0 to 1
 */
v8::Handle<v8::Value> ScriptRandFunction(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to rand.")) );

    float rander = randFloat(0,1);
    return v8::Number::New(rander);
}


/**
   @param takes in a single argument
   @return returns a float
*/
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

/**
   @param float to take arccosine of
   @return angle in radians
 */
v8::Handle<v8::Value> ScriptAcosFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to acos.")) );

    v8::Handle<v8::Value> toSqrt = args[0];

    double d_toSqrt = NumericExtract(toSqrt);

    //return v8::Handle<v8::Number>::New(sqrt(d_toSqrt));
    return v8::Number::New(acos(d_toSqrt));
}

/**
   @param angle in radians to take cosine of
   @return cosine of that angle
 */
v8::Handle<v8::Value> ScriptCosFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to cos.")) );

    v8::Handle<v8::Value> toSqrt = args[0];

    double d_toSqrt = NumericExtract(toSqrt);


    //return v8::Handle<v8::Number>::New(sqrt(d_toSqrt));
    return v8::Number::New(cos(d_toSqrt));
}

/**
   @param angle in radians to take sine of
   @return sine of that angle
 */
v8::Handle<v8::Value> ScriptSinFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sin.")) );

    v8::Handle<v8::Value> toSqrt = args[0];

    double d_toSqrt = NumericExtract(toSqrt);


    return v8::Number::New(sin(d_toSqrt));
}

/**
   @param float to take arcsine of
   @return angle in radians
 */
v8::Handle<v8::Value> ScriptAsinFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to asin.")) );

    v8::Handle<v8::Value> toSqrt = args[0];

    double d_toSqrt = NumericExtract(toSqrt);


    //return v8::Handle<v8::Number>::New(sqrt(d_toSqrt));
    return v8::Number::New(asin(d_toSqrt));
}

/**
   @param base
   @param exponent
   @return returns base to the exponent
 */
v8::Handle<v8::Value> ScriptPowFunction(const v8::Arguments& args)
{
    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: power function requires two arguments.  Expecting <base> and <exponent>")) );

    v8::HandleScope handle_scope;
    double base     = NumericExtract(args[0]);
    double exponent = NumericExtract(args[1]);

    double returner = pow(base,exponent);

    return v8::Number::New( returner );
}

/**
   @param exponent
   @return returns e to the exponent
 */
v8::Handle<v8::Value> ScriptExpFunction(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: exp function requires 1 argument.")) );

    v8::HandleScope handle_scope;
    double exponent = NumericExtract(args[0]);

    double returner = exp(exponent);

    return v8::Number::New( returner );
}


/**
   @param number to take abs of
   @return returns absolute value of argument.
 */
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
