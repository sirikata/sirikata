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

//this function calculates when a condition of the scripted function should
//be re-evaluated on a timer.  It's mostly for distances between objects.
//one would say something like create_when_timeout_lt(a,b,6).  Which means,
//fire the predicate when the distance from a to b is less than 6.  (Does this
//time calculation based on the current velocities and positions of a and b.)
//a or b can either be presences, vec3s, or visibles.
v8::Handle<v8::Value> ScriptCreateWhenTimeoutLT(const v8::Arguments& args)
{
    if (args.Length() != 3)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWhenTimeoutLT: requires three arguments.  First two arguments should be either presences, vec3s, or visibles.  The last argument should be a number.")));

    //check that last arg is a number
    if (! NumericValidate(args[2]))
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWhenTimeoutLT: requires three arguments.  First two arguments should be either presences, vec3s, or visibles.  The last argument should be a number.  In this case, the last arg was not a number.")));

    double ltRHS = NumericExtract(args[2]);

    String errorMsg1 = "Error decoding first arg of whenTimeoutLT as a presence, visible, or vec3.  ";
    String errorMsg2 = "Error decoding second arg of whenTimeoutLT as a presence, visible, or vec3.  ";
    //check if args are presences
    JSPresenceStruct* presStruct_LHS_1 = JSPresenceStruct::decodePresenceStruct(args[0],errorMsg1);
    JSPresenceStruct* presStruct_LHS_2 = JSPresenceStruct::decodePresenceStruct(args[1],errorMsg2);
    //check if args are visibles
    JSVisibleStruct* visStruct_LHS_1 = JSVisibleStruct::decodeVisible(args[0],errorMsg1);
    JSVisibleStruct* visStruct_LHS_2 = JSVisibleStruct::decodeVisible(args[1],errorMsg2);
    //check if args are vec3s
    Vector3d vec3_LHS_1 = Vector3d::nil();
    Vector3d vec3_LHS_2 = Vector3d::nil();

    bool Vec3Validate(Handle<Object>& src);


    if ((presStruct_LHS_1 == NULL) && (visStruct_LHS_1 == NULL))
    {
        //check if the first arg is a vec3.;
        //if it is not (and the upper check indicates that it wasn't a visible
        //or a presence, throw error!
        if (!Vec3ValValidate(args[0]))
            return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg1.c_str())));

        vec3_LHS_1 = Vec3ValExtract(args[0]);
    }

    //same thing for second arg
    if ((presStruct_LHS_2 == NULL) && (visStruct_LHS_2 == NULL))
    {
        //check if the second arg is a vec3.;
        //if it is not (and the upper check indicates that it wasn't a visible
        //or a presence, throw error!
        if (!Vec3ValValidate(args[1]))
            return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg2.c_str())));

        vec3_LHS_2 = Vec3ValExtract(args[1]);
    }

    //grab the util object
    String errorMessage = "Error in ScriptCreateWhenTimeoutLT of JSUtilObj.cpp.  Cannot decode the jsutil field of the util object.  ";
    JSUtilStruct* jsutil = JSUtilStruct::decodeUtilStruct(args.This(),errorMessage);

    return jsutil->struct_createWhenTimeoutLT(presStruct_LHS_1,visStruct_LHS_1,vec3_LHS_1,presStruct_LHS_2, visStruct_LHS_2,vec3_LHS_2, ltRHS);
}


v8::Handle<v8::Value> ScriptCreateWhenWatchedItem(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWhenWatchedItem: requires a single argument (an array of strings) that lists a variable's name.  For instance, var x.y.z would give array ['x','y','z']")));

    if (! args[0]->IsArray())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWhenWatchedItem: requires a single argument (an array of strings) that lists a variable's name.  For instance, var x.y.z would give array ['x','y','z']")));

    v8::Handle<v8::Array> itemArray = v8::Handle<v8::Array>::Cast(args[0]->ToObject());


    String errorMessage = "Error in ScriptCreateWhenWatchedItem of JSUtilObj.cpp.  Cannot decode the jsutil field of the util object.  ";
    JSUtilStruct* jsutil = JSUtilStruct::decodeUtilStruct(args.This(),errorMessage);

    if (jsutil == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsutil->struct_createWhenWatchedItem(itemArray);
}

v8::Handle<v8::Value> ScriptCreateWhenWatchedList(const v8::Arguments& args)
{
    //check do not have too many arguments
    if (args.Length() !=1)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWhenWatchedList: requires one argument (an array of createWhenWatchedItems).")));


    //check that first arg is an array.
    if (! args[0]->IsArray())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWhenWatchedItem of JSUtilObj.cpp.  First argument passed to create_when_watched_list should be an array each containing a single watched_item.")));

    //try to decode object
    String errorMessage = "Error in ScriptCreateWhenWatchedItem of JSUtilObj.cpp.  Cannot decode the jsutil field of the util object.  ";
    JSUtilStruct* jsutil = JSUtilStruct::decodeUtilStruct(args.This(),errorMessage);

    if (jsutil == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));


    v8::Handle<v8::Array> arrayOfItems = v8::Handle<v8::Array>::Cast(args[0]);

    return jsutil->struct_createWhenWatchedList(arrayOfItems);
}


v8::Handle<v8::Value> ScriptCreateQuotedObject(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateQuotedObject of JSUtilObj.cpp.  Quoted object constructor requires a single argument (text, as a string, to be quoted).")));

    //try decoding first argument as a string.
    String decodedString;
    String errorMessageStrDec  = "Error decoding first argument of ScriptCreateQuotedObject as a string.  ";
    bool stringArgDecodeSuccess = decodeString(args[0], decodedString, errorMessageStrDec);
    if (! stringArgDecodeSuccess)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessageStrDec.c_str(), errorMessageStrDec.length())));


    //try decoding the util object.
    String errorMessage = "Error in ScriptCreateQuotedObject of JSUtilObj.cpp.  Cannot decode the jsutil field of the util object.  ";
    JSUtilStruct* jsutil = JSUtilStruct::decodeUtilStruct(args.This(),errorMessage);

    if (jsutil == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsutil->struct_createQuotedObject(decodedString);
}



//first arg is an array that should be mashed into a predicate.
//second argument should be a function.
v8::Handle<v8::Value> ScriptCreateWhen(const v8::Arguments& args)
{
    //check have correct number of arguments.
    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Require two arguments when creating a when object")) );

    //check that first arg is an array.
    if (! args[0]->IsArray())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Require first argument when creating when to be an array.")) );

    if (! args[1]->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Require second argument when creating when to be a function.")) );


    v8::Handle<v8::Array> arrayPredObj = v8::Handle<v8::Array>::Cast(args[0]);
    v8::Handle<v8::Function> funcCBObj = v8::Handle<v8::Function>::Cast(args[1]);


    //decode util object
    String errorMessage  = "Error creating when in ScriptCreateWhen of JSUtilObj.cpp.  Cannot decode util object.  ";
    JSUtilStruct* jsutil = JSUtilStruct::decodeUtilStruct(args.This(),errorMessage);
    if (jsutil == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));


    return jsutil->struct_createWhen(arrayPredObj,funcCBObj);
}



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

        return v8::String::New((str1+str2).c_str());
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
