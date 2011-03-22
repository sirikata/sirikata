#include "JSWhenStruct.hpp"
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "JSSuspendable.hpp"
#include "../JSLogging.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"
#include "JSQuotedStruct.hpp"

#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include "JSWhenWatchedItemStruct.hpp"



namespace Sirikata {
namespace JS {


JSWhenStruct::JSWhenStruct(v8::Handle<v8::Array>predArray, v8::Handle<v8::Function> callback,JSObjectScript* jsobj, JSContextStruct* jscontextstr)
 :  JSSuspendable(),
    predState(false),
    mObjScript(jsobj),
    jscont(jscontextstr)
{
    //enter
    v8::HandleScope handle_scope;
    mContext = v8::Persistent<v8::Context>::New(v8::Context::GetCurrent());


    //generate the predicate function and callback function.
    whenCreatePredFunc(predArray);
    whenCreateCBFunc(callback);


    //linking everything so that will be able to chek
    addWhenToContext();
    addWhenToScript();
}


JSWhenStruct* JSWhenStruct::decodeWhenStruct(v8::Handle<v8::Value> toDecode,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of when object.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != WHEN_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of when bject.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsWhenStruct field
    v8::Local<v8::External> wrapJSWhenObj;
    wrapJSWhenObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(WHEN_TEMPLATE_FIELD));
    void* ptr = wrapJSWhenObj->Value();

    JSWhenStruct* returner;
    returner = static_cast<JSWhenStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of when object.  Internal field of object given cannot be casted to a JSWhentruct.";

    return returner;
}


/**
   Simple.  Make callback more permanent (ie won't be disposed until you
   explicitly tell it to be).
 */
void JSWhenStruct::whenCreateCBFunc(v8::Handle<v8::Function>callback)
{
    v8::HandleScope handle_scope;
    mCB = v8::Persistent<v8::Function>::New ( callback );
}


void JSWhenStruct::buildWatchedItems(const String& whenPredAsString)
{
    JSLOG(insane, "building a list of watched items for predicate of when statement.  here is predicate: "<<whenPredAsString);

    //whenPredAsString:  x < 3 + z || x.y.a > 2
    //tokenizedPred: [ util.create_when_watched_item(['x']),
    //util.create_when_watched(['x']),
    //util.create_when_watched_item(['x','y','a])]
    String tokenizedPred = mObjScript->tokenizeWhenPred(whenPredAsString);

    //evaluate the tokenizedPred, which returns an array object
    String fncTokePred = tokenizedPred;
    JSLOG(insane,"when predicate function associated with tokenized predicate: "<<fncTokePred);

    v8::ScriptOrigin origin(v8::String::New("(whenpredicate)"));
    v8::Handle<v8::Value> compileFuncResult =   mObjScript->internalEval(mContext,fncTokePred,&origin,true);


    String errorMessage = "Error building watched items in jswhenstruct.  Trying to decode a list of items to watch associated with the when predicate.  ";
    mWWLS = JSWhenWatchedListStruct::decodeWhenWatchedListStruct(compileFuncResult,errorMessage);
    if (mWWLS == NULL)
    {
        errorMessage += ("\n\n predicate decoding:  " + fncTokePred );
        JSLOG(error, errorMessage);
        return;
    }

    //for debugging.
    //mWWLS->debugPrintWatchedList();
}

//This function takes in the array that represents the when's predicate.
//If there is a quoted object in the array, its quote explicitly gets appended
//to the when predicate string.
//If there is a non-quoted value in it, rename the value in the current context,
//and append the re-named onto the when.
void JSWhenStruct::whenCreatePredFunc(v8::Handle<v8::Array>predArray)
{
    String whenPredAsString;
    std::vector<String> dependentParts;

    for (int s=0; s < (int)predArray->Length(); ++s)
    {
        String errorMessage;
        JSQuotedStruct* fromPredArray = JSQuotedStruct::decodeQuotedStruct(predArray->Get(s),errorMessage);
        if (fromPredArray == NULL)
        {
            //create a new value in the context that is equal to this object
            String newName = mObjScript->createNewValueInContext(predArray->Get(s), mContext);
            whenPredAsString += newName;
        }
        else
        {
            dependentParts.push_back(fromPredArray->getQuote());
            //dependentParts.push_back(parseParts(fromPredArray->getQuote()));
            whenPredAsString += fromPredArray->getQuote();
        }
    }

    //still need to do something to parse out dependent parts;
    buildWatchedItems(whenPredAsString);


    //compile function;
    //note: additional parentheses and semi-colon around outside of the
    //expression get around a minor idiosyncracy v8 has about compiling
    //anonymous functions.
    whenPredAsString = "(function()  {  return ( " + whenPredAsString + " ); });";


    mContext->Enter();
    v8::ScriptOrigin origin(v8::String::New("(whenpredicate)"));
    v8::Handle<v8::Value> compileFuncResult = mObjScript->internalEval(mContext, whenPredAsString, &origin, true);

    if (! compileFuncResult->IsFunction())
    {
        JSLOG(error, "Error when creating when predicate.  Predicate did not resolve to a function.");
        //Note: if the function is incorrectly declared.  The when struct stays
        //permanently suspended.  Will never resume.
        suspend();
        return;
    }

    v8::HandleScope handle_scope;
    mPred = v8::Persistent<v8::Function>::New ( v8::Handle<v8::Function>::Cast(compileFuncResult));
    mContext->Exit();
}




String JSWhenStruct::createNewValueIncontext(v8::Handle<v8::Value> toRenameInContext)
{
    JSLOG(insane, "got a dollared value to rename inside of when context.  Doing so now.");
    return mObjScript->createNewValueInContext(toRenameInContext, mContext);
}

JSWhenStruct::~JSWhenStruct()
{
    if (! getIsCleared())
    {
        mContext.Dispose();
        mPred.Dispose();
        mCB.Dispose();

        if (jscont != NULL)
            jscont->struct_deregisterSuspendable(this);

        mObjScript->removeWhen(this);
    }
}


void JSWhenStruct::addWhenToContext()
{
    if (jscont != NULL)
        jscont->struct_registerSuspendable(this);
}

void JSWhenStruct::addWhenToScript()
{
    mObjScript->addWhen(this);
}


bool JSWhenStruct::checkPredAndRun()
{
    if (getIsSuspended() || getIsCleared())
        return false;

    //updateVisibles();
    bool prevPredState=predState;

    predState = evalPred();
    if (predState && !prevPredState)
    {
        runCallback();
        return true;
    }
    return false;
}



//this function evaluates the predicate within the context
bool JSWhenStruct::evalPred()
{
    v8::HandleScope handle_scope;

    //the function passed in shouldn't take any arguments
    v8::Handle<v8::Value>predReturner = mObjScript->handleTimeoutContext(mPred, (JSContextStruct*)NULL);

    String dummyErrorMessage;
    bool decodedVal;
    bool returnedBool = decodeBool(predReturner,decodedVal,dummyErrorMessage);

    if (! returnedBool)
    {
        JSLOG(error,"Error in evalPred of JSWhenStruct.cpp.  Predicate did not return bool.  Suspending when statement");
        return false;
    }

    return decodedVal;
}


void JSWhenStruct::runCallback()
{
    v8::HandleScope handle_scope;

    //the function passed in shouldn't take any arguments
    mObjScript->handleTimeoutContext(mCB, (JSContextStruct*)NULL);
}


v8::Handle<v8::Value>JSWhenStruct::struct_whenGetLastPredState()
{
    v8::HandleScope handle_scope;
    return v8::Boolean::New(predState);
}


v8::Handle<v8::Value>JSWhenStruct::suspend()
{
    return JSSuspendable::suspend();
}



/**
   Overriding clear to explicitly dispose of
 */
v8::Handle<v8::Value>JSWhenStruct::clear()
{

    if (! getIsCleared())
    {
        //should only dispose of these objects once.
        if (! mPred->IsUndefined())  //may be undefined if passed in an
                                     //incorrect function that wouldn't compile.
            mPred.Dispose();

        if (! mCB->IsUndefined())
            mCB.Dispose();

        //will always be defined if haven't been cleared.
        mContext.Dispose();

        if (jscont != NULL)
            jscont->struct_deregisterSuspendable(this);

        mObjScript->removeWhen(this);
    }

    return JSSuspendable::clear();
}




/**
   Overriding the resume function.  We never want to be evaluating the predicate
   if it is undefined.  Instead, we want to stay permanently suspended.
 */
v8::Handle<v8::Value>JSWhenStruct::resume()
{
    if ((mPred->IsUndefined()) || (mCB->IsUndefined()))
    {
        JSLOG(error, "Cannot resume because incorrectly declared when predicate or callback.  Staying suspended forever.");
        return suspend();
    }

    if (getIsCleared())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Cannot resume a when that has already been cleared.")));


    return JSSuspendable::resume();
}




} //end namespace js
} //end namespace sirikata
