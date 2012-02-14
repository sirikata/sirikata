#include "JSSystem.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSObjectStructs/JSContextStruct.hpp"
#include "JSFields.hpp"
#include "JSObjectsUtils.hpp"
#include "../JSSystemNames.hpp"
#include "../JSObjectStructs/JSSystemStruct.hpp"
#include "../JSEntityCreateInfo.hpp"
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include "JSVec3.hpp"
#include "JSQuaternion.hpp"
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <algorithm>
#include <cctype>
#include <boost/any.hpp>

namespace Sirikata {
namespace JS {
namespace JSSystem {



/**
   @param Which presence to send from.
   @param Message value to send.
   @param Visible to send to.
   @param (Optional) Error handler function.

   Sends a message to the presence associated with this visible object from the
   presence that can see this visible object.
 */
v8::Handle<v8::Value> sendMessageReliable (const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    return handle_scope.Close(sendMessage(args,true));
}

v8::Handle<v8::Value> sendMessageUnreliable(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    return handle_scope.Close(sendMessage(args,false));
}


v8::Handle<v8::Value> sendMessage(const v8::Arguments&args, bool reliable)
{
    if ((args.Length() != 4) && (args.Length() != 3))
    {
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Requires 3 or four arguments.  <which presence to send from><msg object to send><visible to send to><(optional)error handler function>")));
    }

    //which pres to send from
    v8::Handle<v8::Value> presToSendFrom = args[0];
    String errMsg = "Error decoding presence argument to send message.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(presToSendFrom,errMsg);
    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str())));

    //convert the message value to send to a string.
    std::string serialized_message = JSSerializer::serializeMessage(args[1]);

    //visible to send to
    v8::Handle<v8::Value> visToSendTo = args[2];
    //decode the visible struct associated with this object
    std::string errorMessage = "In __visibleSendMessage function of visible.  ";

    //want to decode to position listener to send message out of.
    JSPositionListener* jspl = decodeJSPosListener(visToSendTo,errorMessage);

    if (jspl == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    //decode system object
    errorMessage = "Error decoding error message when sending message";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));


    return jsfake->sendMessageNoErrorHandler(jspres,serialized_message,jspl,reliable);
}


v8::Handle<v8::Value> pushEvalContextScopeDirectory(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    if (args.Length() != 1)
        V8_EXCEPTION_CSTR("Error pushing eval scope directory.  Requires 1 argument: a string.");

    INLINE_SYSTEM_CONV_ERROR(args.This(),getAssociatedPresence,this,jssys);
    INLINE_STR_CONV_ERROR(args[0],pushEvalContextScopeDirectory,1,newDir);
    
    return handle_scope.Close(jssys->pushEvalContextScopeDirectory(newDir));
}

v8::Handle<v8::Value> popEvalContextScopeDirectory(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    if (args.Length() != 0)
        V8_EXCEPTION_CSTR("Error popping eval scope directory.  Requires 0 arguments");

    INLINE_SYSTEM_CONV_ERROR(args.This(),getAssociatedPresence,this,jssys);
    
    return handle_scope.Close(jssys->popEvalContextScopeDirectory());
}


//returns wrapped presence struct that's associated with system's context
//if system's context is not associated with a presence struct (ie, it's the
//root context), then return undefined.
v8::Handle<v8::Value> getAssociatedPresence(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    if (args.Length() != 0)
        V8_EXCEPTION_CSTR("Error getting associated presence.  Requires 0 args to be passed.");

    INLINE_SYSTEM_CONV_ERROR(args.This(),getAssociatedPresence,this,jssys);
    return handle_scope.Close(jssys->getAssociatedPresence());
}


v8::Handle<v8::Value> evalInGlobal(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Eval only takes one parameter: the program text to evaluate.")) );

    v8::Handle<v8::Value> contents = args[0];

    StringCheckAndExtract(native_contents, contents);


    String errorMessage       = "Error calling eval in context.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct( args.This(), errorMessage);
    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    ScriptOrigin origin = args.Callee()->GetScriptOrigin();
    return jsfake->struct_evalInGlobal(native_contents,&origin);
}



//single argument should be a function.
v8::Handle<v8::Value> setSandboxMessageCallback(const v8::Arguments& args)
{
    if (args.Length() != 1)
        V8_EXCEPTION_CSTR("Setting callbacks for messages from sandboxes requires exactly one argument.");

    //get jssystemstruct
    INLINE_SYSTEM_CONV_ERROR(args.This(),sendSandbox,this,jssys);

    v8::Handle<v8::Value> cbVal = args[0];
    if (!cbVal -> IsFunction())
        V8_EXCEPTION_CSTR("Error in setSandboxMessageCallback.  First argument should be a function.");

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cbVal);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);
    return jssys->setSandboxMessageCallback(cb_persist);
}

v8::Handle<v8::Value> setPresenceMessageCallback(const v8::Arguments& args)
{
    if (args.Length() != 1)
        V8_EXCEPTION_CSTR("Setting callbacks for messages from presences requires exactly one argument.");

    //get jssystemstruct
    INLINE_SYSTEM_CONV_ERROR(args.This(),sendSandbox,this,jssys);

    v8::Handle<v8::Value> cbVal = args[0];
    if (!cbVal -> IsFunction())
        V8_EXCEPTION_CSTR("Error in setPresenceMessageCallback.  First argument should be a function.");

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cbVal);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);
    return jssys->setPresenceMessageCallback(cb_persist);
}


v8::Handle<v8::Value> getUniqueToken(const v8::Arguments& args)
{
    if (args.Length() != 0)
        V8_EXCEPTION_CSTR("getUniqueToken requires 0 arguments.");

    UUID randUUID = UUID::random();
    return v8::String::New(randUUID.toString().c_str());
}


/**
   First argument should contain a message object that will be
   serialized and sent to another sandbox on same entity.

   Second argument should either be == null (send to parent), or should contain
   a sandbox object.
 */
v8::Handle<v8::Value> root_sendSandbox(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;

    if (args.Length() != 2)
        V8_EXCEPTION_CSTR("Send sandbox requires exactly two arguments");


    //get jssystemstruct
    INLINE_SYSTEM_CONV_ERROR(args.This(),sendSandbox,this,jssys);

    //decode message.
    String serializedMessage = JSSerializer::serializeMessage(args[0]);


    //recipeint == null implies send to parent (if it exists).
    JSContextStruct* recipient = NULL;
    if (! args[1]->IsNull())
    {
        //if it isn't null then we should be trying to send to another sandbox.
        //try to decode that sandbox here.  if decode fails, throw error.
        INLINE_CONTEXT_CONV_ERROR(args[1],sendSandbox,2,rec);
        recipient = rec;
    }


    return handle_scope.Close(jssys->sendSandbox(serializedMessage,recipient));
}


v8::Handle<v8::Value> emersonCompileString(const v8::Arguments& args)
{
    HandleScope handle_scope;
    if (args.Length() != 1)
        V8_EXCEPTION_CSTR("emersonCompileString takes in a single argument");

    INLINE_STR_CONV_ERROR(args[0],emersonCompileString,1,strToCompile);
    INLINE_SYSTEM_CONV_ERROR(args.This(),emersonCompileString,this,jssys);
    return handle_scope.Close(jssys->emersonCompileString(strToCompile));
}

v8::Handle<v8::Value> storageBeginTransaction(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling storageBeginTransaction.  BeginTransaction takes 0 arguments.")));

    //decode system object
    String errorMessage = "Error decoding error message when storageBeginTransactioning";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageBeginTransaction();
}

namespace {
v8::Handle<v8::Function> maybeDecodeCallbackArgument(const v8::Arguments& args, int32 idx) {
    v8::Handle<v8::Function> cb;
    if (args.Length() > idx) {
        v8::Handle<v8::Value> cbVal = args[idx];
        if (cbVal->IsFunction())
            cb = v8::Handle<v8::Function>::Cast(cbVal);
    }
    return cb;
}
}

v8::Handle<v8::Value> storageCommit(const v8::Arguments& args)
{
    // Potentially decode a callback
    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 0);

    //decode system object
    String errorMessage = "Error decoding error message when storageCommiting";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageCommit(cb);
}

v8::Handle<v8::Value> storageErase(const v8::Arguments& args)
{
    if (args.Length() != 1 && args.Length() != 2)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling erase.  Require 1 or 2 arguments: an item to delete and optional callback")));

    INLINE_STR_CONV_ERROR(args[0],storageErase,1,key);
    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 1);

    //decode system object
    String errorMessage = "Error decoding error message when storageErase";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageErase(key, cb);
}

v8::Handle<v8::Value> root_killEntity(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    //decode the system object
    INLINE_SYSTEM_CONV_ERROR(args.This(),killEntity,this,jssystem);
    return handle_scope.Close(jssystem->killEntity());
}



/**
   @param {String} type (GET or POST) are only two supported for now.
   @param {String} url or ip address.
   @param {String} request paramerts
   @param {function} callback to execute on success or failure (first arg of
   function is bool.  If success, bool is true, if fail, bool is false).
   Success callbacks have a second arg that takes in an object with the
   following fields:
      respHeaders (string map).
      contentLength (number).
      status code (number).
      data (string).
 */
v8::Handle<v8::Value> root_http(const v8::Arguments& args)
{
    if (args.Length() != 4)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in http request.  Require 4 arguments")));


    //system object
    INLINE_SYSTEM_CONV_ERROR(args.This(),http,this,jssys);

    //http command, get, head, etc.
    INLINE_STR_CONV_ERROR(args[0], http, 1,httpComm);

    //check if it's a get or post: first convert to lower case
    for(uint32 i=0; i < httpComm.size(); ++i)
        httpComm[i] = std::tolower(httpComm[i]);


    Transfer::HttpManager::HTTP_METHOD httpCommType;
    if (httpComm == "get")
        httpCommType = Transfer::HttpManager::GET;
    else if (httpComm == "head")
        httpCommType= Transfer::HttpManager::HEAD;
    else
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in http request.  Http query type must be get or head.")));


    //url
    INLINE_STR_CONV_ERROR(args[1], http, 2, urlStr);
    Network::Address addr = Network::Address::null();
    try
    {
        addr = Network::Address::lexical_cast(urlStr).as<Network::Address>();
    }
    catch(std::invalid_argument& ia)
    {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in http request.  Could not decode address.")));
    }


    //request params
    INLINE_STR_CONV_ERROR(args[2], http,3,reqParams);


    //callback function.
    if (! args[3]->IsFunction())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in http request: callback must be a function")));

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(args[3]);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);

    return jssys->httpRequest(addr, httpCommType, reqParams, cb_persist);
}


v8::Handle<v8::Value> storageWrite(const v8::Arguments& args)
{
    if (args.Length() != 2 && args.Length() != 3)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling storageWrite.  Require 2 or 3 arguments: a key (string), a string to write (string), and an optional callback")));

    INLINE_STR_CONV_ERROR(args[0],storageWrite,1,key);

    if (! args[1]->IsString())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in storageWrite, second argument (string) should be a string.")));

    String toWrite =  uint16StrToStr(args[1]->ToString());
    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 2);

    //decode system object
    String errorMessage = "Error decoding error message when storageWriting";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageWrite(key,toWrite, cb);
}

v8::Handle<v8::Value> storageRead(const v8::Arguments& args)
{
    if (args.Length() != 1 && args.Length() != 2)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling storageRead.  Require 1 or 2 arguments: an key (string) and optional callback")));

    INLINE_STR_CONV_ERROR(args[0],storageRead,1,key);
    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 1);

    //decode system object
    String errorMessage = "Error decoding error message when storageReading";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageRead(key, cb);
}

v8::Handle<v8::Value> storageRangeRead(const v8::Arguments& args)
{
    if (args.Length() != 3)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling storageRangeRead.  Require 3 arguments: a start key (string), a finish key (string), and a callback")));

    INLINE_STR_CONV_ERROR(args[0],storageRangeRead,1,start);
    INLINE_STR_CONV_ERROR(args[1],storageRangeRead,2,finish);

    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 2);

    //decode system object
    String errorMessage = "Error decoding error message when storageRangeReading";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageRangeRead(start, finish, cb);
}

v8::Handle<v8::Value> storageRangeErase(const v8::Arguments& args)
{
    if (args.Length() != 3)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling storageRangeErase.  Require 3 arguments: a start key (string), a finish key (string), and a callback")));

    INLINE_STR_CONV_ERROR(args[0],storageRangeErase,1,start);
    INLINE_STR_CONV_ERROR(args[1],storageRangeErase,2,finish);

    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 2);

    //decode system object
    String errorMessage = "Error decoding error message when storageRangeErasing";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageRangeErase(start, finish, cb);
}

v8::Handle<v8::Value> storageCount(const v8::Arguments& args)
{
    if (args.Length() != 3)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling storageCount.  Require 3 arguments: a start key (string), a finish key (string), and a callback")));

    INLINE_STR_CONV_ERROR(args[0],storageCount,1,start);
    INLINE_STR_CONV_ERROR(args[1],storageCount,2,finish);

    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 2);

    //decode system object
    String errorMessage = "Error decoding error message when storageCounting";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->storageCount(start, finish, cb);
}

v8::Handle<v8::Value> setRestoreScript(const v8::Arguments& args) {
    if (args.Length() != 1 && args.Length() != 2)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling setRestoreScript. Require 1 or 2 arguments: an script (string or function) and optional callback")));

    INLINE_STR_CONV_ERROR(args[0],setRestoreScript,1,script);
    v8::Handle<v8::Function> cb = maybeDecodeCallbackArgument(args, 1);

    //decode system object
    String errorMessage = "Error decoding error message in setRestoreScript";
    JSSystemStruct* jsfake = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->setRestoreScript(script, cb);
}

v8::Handle<v8::Value> debug_fileRead(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling fileRead.  Require 1 argument: a string filename to read from")));

    v8::Handle<v8::Value> fileToReadFromVal = args[0];

    String errMsg = "Error in debug_fileRead.  Could not decode argument as string.  ";
    String fileToReadFrom;
    bool strDecode = decodeString(fileToReadFromVal,fileToReadFrom,errMsg);
    if (!strDecode)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str())));

    errMsg = "Error decoding system struct when calling fileRead. ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errMsg);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));

    return jssys->debug_fileRead(fileToReadFrom);
}

v8::Handle<v8::Value> debug_fileWrite(const v8::Arguments& args)
{
    if(args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error calling fileWrite.  Require 2 arguments: 1 a string to write and 2 a filename to write to")));

    v8::Handle<v8::Value> strToWriteVal    = args[0];
    v8::Handle<v8::Value> fileToWriteToVal = args[1];

    if (! strToWriteVal->IsString())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in fileWrite, first argument (string) should be a string.")));


    String strToWrite =  uint16StrToStr(strToWriteVal->ToString());
    String errMsg = "Error in debug_fileWrite.  Could not decode argument as string.  ";
    String filename;
    bool strDecode = decodeString(fileToWriteToVal,filename,errMsg);
    if (!strDecode)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str())));

    errMsg = "Error decoding system struct when calling fileWrite. ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errMsg);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));

    return jssys->debug_fileWrite(strToWrite,filename);
}


v8::Handle<v8::Value> root_serialize(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error calling serialize.  Must pass in at least one argument to be serialized.")));


    String stringifiedValue = JSSerializer::serializeMessage(args[0]);

    String errorMessage = "Error decoding error message when serializing object";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));


    v8::Handle<v8::Value> returner = strToUint16Str(stringifiedValue);

    return handle_scope.Close(returner);
}

v8::Handle<v8::Value> root_proxAddedHandler(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error registering prox added handler requires one argument <function>")));

    String errMsg = "Error decoding system struct when registering prox added handler. ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errMsg);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    v8::Handle<v8::Value> cbVal = args[0];
    if (!cbVal -> IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New( "Error in prox added handler.  First argument should be a function.")));

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cbVal);
    return jssys->proxAddedHandlerCallallback(cb);
}


v8::Handle<v8::Value> root_proxRemovedHandler(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error registering prox removed handler requires one argument <function>")));

    String errMsg = "Error decoding system struct when registering prox removed handler. ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errMsg);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    v8::Handle<v8::Value> cbVal = args[0];
    if (!cbVal -> IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New( "Error in prox removed handler.  First argument should be a function.")));

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cbVal);
    return jssys->proxRemovedHandlerCallallback(cb);
}



v8::Handle<v8::Value> root_deserialize(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;

    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error calling deserialize.  Must pass in an object to be deserialized.")));


    if (! args[0]->IsString())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error calling deserialize.  First argument to deserialize should be string")));


    String serString = uint16StrToStr(args[0]->ToString());

    String errMsg = "Error decoding system struct when deserializing. ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errMsg);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str(), errMsg.length())));

    return jssys->deserialize(serString);
}

v8::Handle<v8::Value> root_headless(const v8::Arguments& args)
{

    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error checking headless.  Takes no arguments.")));

    String errMsg = "Error decoding system struct when checking headless. ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errMsg);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    return jssys->checkHeadless();
}




//address of visible watching;
//vector3 of position x,y,z;
//vector3 of velocity x,y,z;
//string time
//quaternion orientation
//quaternion orientation vel
//string time
//vector 3 of center position
//float of radius
//mesh
//physics
v8::Handle<v8::Value> root_createVisible(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;

    if ((args.Length() != 11) && (args.Length() != 1))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in createVisible call.  Require either a single string argument to create visible object.  Or requires 11 arguments.  See documentation.")));

    String errMsg_sys = "Error decoding system struct when creating visible. ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errMsg_sys);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg_sys.c_str())));


    //decode first arg: sporef watching
    SpaceObjectReference sporefVisWatching;
    String baseErrMsg  = "Error in system when trying to createVisible.  ";
    String errMsg      = baseErrMsg + "Could not decode first argument.";
    bool sporefDecoded = decodeSporef(args[0],sporefVisWatching,errMsg);

    if (! sporefDecoded)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));

    if  (args.Length() == 1)
        return jssys->struct_create_vis(sporefVisWatching, JSVisibleDataPtr());



    //decode second-4th args: timed motion vector
    TimedMotionVector3f location;
    errMsg = baseErrMsg + "Could not decode 2-4 arguments corresponding to position.  ";
    bool decodedTMV = decodeTimedMotionVector(args[1],args[2],args[3], location,errMsg);

    if (! decodedTMV)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));

    //decode fifth-seventh args: timed motion quaternion
    TimedMotionQuaternion orientation;
    errMsg = baseErrMsg + "Could not decode 5-7 arguments corresponding to orientation.  ";
    bool decodedTMQ = decodeTimedMotionQuat(args[4],args[5],args[6], orientation,errMsg);

    if (! decodedTMQ)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //decode eighth-ninth args: bounding sphere
    BoundingSphere3f bsph;
    errMsg = baseErrMsg + "Could not decode 8-9 arguments corresponding to bounding sphere.  ";
    bool decodedBSPH = decodeBoundingSphere3f(args[7],args[8], bsph,errMsg);

    if (! decodedBSPH)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //decode tenth arg: mesh string
    String meshString;
    errMsg = baseErrMsg + "Could not decode 10th argument corresponding to mesh string.  ";
    bool meshDecoded = decodeString(args[9],meshString,errMsg);

    if (! meshDecoded)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //decode eleventh arg: physics string
    String physicsString;
    errMsg = baseErrMsg + "Could not decode 11th argument corresponding to physics string.  ";
    bool physicsDecoded = decodeString(args[10],physicsString,errMsg);

    if (! physicsDecoded)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //do not delete this bcause it gets put into a shared pointer.
    //note, do not need to point at
    //emerScript here.
    JSVisibleDataPtr jspd(
        new JSRestoredVisibleData(
            NULL, sporefVisWatching,
            PresenceProperties(location, orientation, bsph, Transfer::URI(meshString), physicsString)
        )
    );

    v8::Handle<v8::Value> returner = jssys->struct_create_vis(sporefVisWatching,jspd);
    return handle_scope.Close(returner);
}



/**
   @return Boolean indicating whether this sandbox has permission to send out of
   its default presence.
 */
v8::Handle<v8::Value> root_canSendMessage(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canSendMessage.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canSendMessage();
}

/**
   @param String corresponding to the filename to look for file to include.

   Library include mechanism.  Calling require makes it so that system searches
   for file named by argument passed in.  If system hasn't already executed this
   file, it reads file, and executes it.  (See import as well.)
 */
v8::Handle<v8::Value> root_require(const v8::Arguments& args)
{
    return commonRequire(args,false);
}

v8::Handle<v8::Value> commonRequire(const v8::Arguments& args,bool isJS)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Require only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];

    StringCheckAndExtract(native_filename, filename);
    String errorMessage = "Error decoding the system object from root_canSendMessage.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));

    return jsfake->struct_require(native_filename,isJS);
}






/**
   Single parameter: an object with information on each presence's prox result
   set:

   obj[pres0.sporef] = [visTo0_a.sporef,visTo0_b.sporef, ...];
   obj[pres1.sporef] = [visTo1_a.sporef,visTo1_b.sporef, ...];
   ...

   Destroys all created objects, except presences in the
   root context and the visibles that were in those presences' prox result set.
   Then executes script associated with root context.  (Use
   system.setScript to set this script.)
 */
v8::Handle<v8::Value> root_reset(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error. reset takes a single argument: an object that contains an array of the sporefs for visibles in presences' result sets.  See documentation in JSSystem.cpp.")));

    if (!args[0]->IsObject())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: reset requires an *object* to be passed in.")));


    v8::Handle<v8::Object> proxResSet = args[0]->ToObject();
    std::map<SpaceObjectReference,std::vector<SpaceObjectReference> > proxResultSetArg;
    bool decodeSuccess= decodeResetArg(proxResSet,proxResultSetArg);
    if (!decodeSuccess)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error. Could not decode reset arg.  See documentation in JSSystem.cpp.")));



    String errorMessage = "Error in reset of system object.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));

    return jsfake->struct_reset(proxResultSetArg);
}

//turns a v8 object, arg, of the form:
// arg[pres0.sporef] = [visTo0_a.sporef,visTo0_b.sporef, ...];
// arg[pres1.sporef] = [visTo1_a.sporef,visTo1_b.sporef, ...];
//into a std::map<sporef, std::vec<sporef>>, where index of map are
// presence's sporefs and values of map (vectors) are sporefs contained in
// vectors.
bool decodeResetArg(v8::Handle<v8::Object> arg, std::map<SpaceObjectReference, std::vector<SpaceObjectReference> > & cppRes)
{

    v8::Local<v8::Array> allPresSporefs = arg->GetPropertyNames();

    std::vector<v8::Local<v8::Object> > propertyNames;
    for (int s=0; s < (int)allPresSporefs->Length(); ++s)
    {

        String dummy; //do not need to track errors from decode sporef, so just
                      //inserting blank string.
        SpaceObjectReference presSporef;
        if (!decodeSporef(allPresSporefs->Get(s), presSporef,dummy))
            return false;  //index wasn't a sporef.

        //presSporef should now contain the sporef of a local presence.
        //getting the field of arg corresponding to allPresSporefs->Get(s)
        //should give arrays of sporefs of visibles that are within presSporef's
        //prox set.
        v8::Local<v8::Value> visSetVal = arg->Get(allPresSporefs->Get(s));

        //arg's values must be arrays of sporefs.
        if (!visSetVal->IsArray())
            return false;

        v8::Local<v8::Array> visSetArray = v8::Local<v8::Array>::Cast(visSetVal);

        for (int t= 0; t < (int) visSetArray->Length(); ++t)
        {
            SpaceObjectReference visSporef;
            if (!decodeSporef(visSetArray->Get(t), visSporef,dummy))
                return false;

            //visSporef should now contain a sporef for a visible object that is
            //within the prox result set for presSporef.
            cppRes[presSporef].push_back(visSporef);
        }
    }

    return true;
}




v8::Handle<v8::Value> root_getScript(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error. getScript requires 0 arguments passed in.")));

    String errorMessage = "Error in getScript of system object.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));


    return jsfake->struct_getScript();
}

/*
  @param String containing a full script.
  Takes a string, which is a script.  When scripter calls system.setScript in
  root context, will re-execute this script.
 */
v8::Handle<v8::Value> root_setScript(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error. setScript requires a string as first arg.")));

    //check that first arg is actually a string
    String decodedScript;
    String strDecodeErrorMsg = "Error decoding string in setScript.";
    bool strDecodeSuccess = decodeString(args[0], decodedScript, strDecodeErrorMsg);
    if (! strDecodeSuccess)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(strDecodeErrorMsg.c_str())));


    String errorMessage = "Error in reset of system object.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));


    return jsfake->struct_setScript(decodedScript);
}


/**
   @return Boolean indicating whether this sandbox has permission to receive
   general messages.
 */
v8::Handle<v8::Value> root_canRecvMessage(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canRecvMessage.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canRecvMessage();
}

/**
   @return Boolean indicating whether this sandbox has permission to receive
   import files.
 */
v8::Handle<v8::Value> root_canImport(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canImport.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canImport();
}

/**
   @return Boolean indicating whether this sandbox has capability to set
   proximity queries associated with its default presence.
 */
v8::Handle<v8::Value> root_canProxCallback(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canProxCallback.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canProxCallback();
}

v8::Handle<v8::Value> root_canProxChangeQuery(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canProxChangeQuery.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canProxChangeQuery();
}


/**
   @return Boolean indicating whether this sandbox has capability to create presences
 */
v8::Handle<v8::Value> root_canCreatePres(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canCreatePres.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canCreatePres();
}

/**
   @return Boolean indicating whether this sandbox has capability to create entities
 */
v8::Handle<v8::Value> root_canCreateEnt(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canCreateEnt.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canCreateEnt();
}

/**
   @return Boolean indicating whether this sandbox has capability to call system.eval
 */
v8::Handle<v8::Value> root_canEval(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canEval.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canEval();
}


v8::Handle<v8::Value> root_jsrequire(const v8::Arguments& args)
{
    return commonRequire(args,true);
}


v8::Handle<v8::Value> root_jsimport(const v8::Arguments& args)
{
    return commonImport(args,true);
}

v8::Handle<v8::Value> root_import(const v8::Arguments& args)
{
    return commonImport(args,false);
}

v8::Handle<v8::Value> commonImport(const v8::Arguments& args, bool isJS)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Import only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];

    //decode the filename to import from.
    String strDecodeErrorMessage = "Error decoding string as first argument of root_import of jssystem.  ";
    String native_filename; //string to decode to.
    bool decodeStrSuccessful = decodeString(filename,native_filename,strDecodeErrorMessage);
    if (! decodeStrSuccessful)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(strDecodeErrorMessage.c_str(), strDecodeErrorMessage.length())) );

    //decode the system object
    String errorMessage = "Error decoding the system object from root_import.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_import(native_filename,isJS);
}

/**
   @return String corresponding to version number of Emerson.
 */
v8::Handle<v8::Value> root_getVersion(const v8::Arguments& args)
{
    return v8::String::New( JSSystemNames::EMERSON_VERSION);
}



/**
   @param Object or value to print

   Prints the object or value to scripting window.
 */
v8::Handle<v8::Value> root_print(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in root_print.  Requires exactly one argument: a string to print.")));

    String errorMessage = "Error decoding the system object from root_print.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    INLINE_STR_CONV_ERROR(args[0],root_print,0,toPrint);

    return jsfake->struct_print(toPrint);
}

/**
   @param A message object.

   Tries to send argument to writer of code in this sandbox (external presence)
   from the internal presence that this sandbox is associated with.  Calling
   from root sandbox, or calling on a sandbox for which you do not have
   capabilities to query for position throws an exception.
*/
v8::Handle<v8::Value> root_sendHome(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in root_sendHome.  Requires exactly one argument: an object to send.")));

    if (! args[0]->IsObject())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in root_sendHome.  Requires argument to be an object.")));

    //decode string argument
    v8::Handle<v8::Value> messageBody = args[0];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object in root_sendHome.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    String serialized_message = JSSerializer::serializeObject(v8Object);


    String errorMessage = "Error decoding the system object from root_print.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));

    return jsfake->struct_sendHome(serialized_message);
}


/**
   @param {string} sporef,
   @param {vec3} pos,
   @param {vec3} vel,
   @param {string} posTime,
   @param {quaternion} orient,
   @param {quaternion} orientVel,
   @param {string} orientTime,
   @param {string} mesh,
   @param {string} physics,
   @param {number} scale,
   @param {boolean} isCleared ,
   @param {uint32} contextId,
   @param {boolean} isConnected,
   @param {function, null} connectedCallback,
   @param {boolean} isSuspended,
   @param {vec3,optional} suspendedVelocity,
   @param {quaternion,optional} suspendedOrientationVelocity,
   @param {float} solidAngleQuery
 */
v8::Handle<v8::Value> root_restorePresence(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;

    if (args.Length() != 18)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when trying to restore presence through system object.  restore_presence requires 18 arguments")));


    v8::Handle<v8::Value> mSporefArg                       = args[0];
    v8::Handle<v8::Value> posArg                           = args[1];
    v8::Handle<v8::Value> velArg                           = args[2];
    v8::Handle<v8::Value> posTimeArg                       = args[3];
    v8::Handle<v8::Value> orientArg                        = args[4];
    v8::Handle<v8::Value> orientVelArg                     = args[5];
    v8::Handle<v8::Value> orientTimeArg                    = args[6];
    v8::Handle<v8::Value> meshArg                          = args[7];
    v8::Handle<v8::Value> physicsArg                       = args[8];
    v8::Handle<v8::Value> scaleArg                         = args[9];
    v8::Handle<v8::Value> isClearedArg                     = args[10];
    v8::Handle<v8::Value> contextIDArg                     = args[11];
    v8::Handle<v8::Value> isConnectedArg                   = args[12];
    v8::Handle<v8::Value> connectedCallbackArg             = args[13];
    v8::Handle<v8::Value> isSuspendedArg                   = args[14];
    v8::Handle<v8::Value> suspendedVelocityArg             = args[15];
    v8::Handle<v8::Value> suspendedOrientationVelocityArg  = args[16];
    v8::Handle<v8::Value> queryArg                         = args[17];

    //now, it's time to decode them.

    String baseErrMsg = "Error in restorePresence.  Could not decode ";

    String specificErrMsg =baseErrMsg + "sporef.";
    SpaceObjectReference mSporef;

    bool sporefDecoded = decodeSporef(mSporefArg, mSporef, specificErrMsg);
    if (! sporefDecoded)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    Nullable<Time> mPositionTime;
    if (! posTimeArg->IsNull())
    {
        INLINE_TIME_CONV_ERROR(posTimeArg,restorePresences,4,posTime);
        mPositionTime.setValue(posTime);
    }

    if (! Vec3ValValidate(posArg))
        V8_EXCEPTION_CSTR("Cannot decode position arg in restore presence as vector");
    Vector3f mPosition(Vec3ValExtract(posArg));



    if (! Vec3ValValidate(velArg))
        V8_EXCEPTION_CSTR("Cannot decode velocity arg in restore presence as vector");
    Vector3f mVelocity(Vec3ValExtract(velArg));



    Nullable<Time> mOrientTime;
    if (! orientTimeArg->IsNull())
    {
        INLINE_TIME_CONV_ERROR(orientTimeArg,restorePresences,7,orientTime);
        mOrientTime.setValue(orientTime);
    }


    if (! QuaternionValValidate(orientArg))
        V8_EXCEPTION_CSTR("Cannot decode orientation arg in restore presence as quaternion");
    Quaternion mOrient(QuaternionValExtract(orientArg));


    if (! QuaternionValValidate(orientVelArg))
        V8_EXCEPTION_CSTR("Cannot decode orientation velocity arg in restore presence as quaterinion");
    Quaternion mOrientVelocity(QuaternionValExtract(orientVelArg));


    String mesh;
    specificErrMsg = baseErrMsg + "mesh.";
    bool meshDecodeSuccessful = decodeString(meshArg, mesh, specificErrMsg);
    if (! meshDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));

    String physics;
    specificErrMsg = baseErrMsg + "physics.";
    bool physicsDecodeSuccessful = decodeString(physicsArg, physics, specificErrMsg);
    if (! physicsDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    specificErrMsg = baseErrMsg + "scale.";
    if (! NumericValidate(scaleArg))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));
    double scale = NumericExtract(scaleArg);

    bool isCleared;
    specificErrMsg = baseErrMsg + "isCleared.";
    bool isClearedDecodeSuccessful = decodeBool(isClearedArg, isCleared, specificErrMsg);
    if (! isClearedDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    Nullable<uint32> contextID;
    if (! contextIDArg->IsNull())
    {
        uint32 cid;
        specificErrMsg = baseErrMsg + "contextID.";
        bool contextIDDecodeSuccessful = decodeUint32(contextIDArg, cid, specificErrMsg);
        if (! contextIDDecodeSuccessful)
            return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));

        contextID.setValue(cid);
    }

    bool isConnected;
    specificErrMsg = baseErrMsg + "isConnected.";
    bool isConnectedDecodeSuccessful = decodeBool(isConnectedArg, isConnected, specificErrMsg);
    if (! isConnectedDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    specificErrMsg = baseErrMsg + "connectedCallback.";
    Nullable<v8::Handle<v8::Function> > connCB;
    if (connectedCallbackArg->IsFunction())
        connCB.setValue(v8::Handle<v8::Function>::Cast(connectedCallbackArg));
    else if (! connectedCallbackArg->IsNull())
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));

    bool isSuspended;
    specificErrMsg = baseErrMsg + "isSuspended.";
    bool isSuspendedDecodeSuccessful = decodeBool(isSuspendedArg, isSuspended, specificErrMsg);
    if (! isSuspendedDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    Nullable<Vector3f> suspendedVelocity;
    specificErrMsg = baseErrMsg + "suspendedVelocity.";
    if (!Vec3ValValidate(suspendedVelocityArg))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));
    suspendedVelocity.setValue(Vec3ValExtractF(suspendedVelocityArg));

    Nullable<Quaternion>     suspendedOrientationVelocity;
    specificErrMsg = baseErrMsg + "suspendedOrientationVelocity.";
    if (!QuaternionValValidate(suspendedOrientationVelocityArg))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));
    suspendedOrientationVelocity.setValue( QuaternionValExtract(suspendedOrientationVelocityArg));


    String query;
    specificErrMsg = baseErrMsg + "query.";
    bool queryDecodeSuccessful = decodeString(queryArg, query, specificErrMsg);
    if (! queryDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));

    //decode system.
    String errorMessageFRoot = "Error decoding the system object from restorePresence.  ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageFRoot);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageFRoot.c_str() )));


    PresStructRestoreParams restParams(
        mSporef,
        mPositionTime,
        mPosition,
        mVelocity,
        mOrientTime,
        mOrient,
        mOrientVelocity,
        mesh,
        physics,
        scale,
        isCleared,
        contextID,
        isConnected,
        connCB,
        isSuspended,
        suspendedVelocity,
        suspendedOrientationVelocity,
        query
    );

    return handle_scope.Close(jssys->restorePresence(restParams));
}



/**
   @param Vec3 (eg. new util.Vec3(0,0,0);).  Corresponds to position to place
   new entity in world.
   @param String.  Script option to pass in.  Almost always pass "js"
   @param String.  New script to execute.
   @param String.  Mesh uri corresponding to mesh you want to use for this
   entity.
   @param Number.  Scale of new mesh.  (Higher number means increase mesh's size.)
   @param Number.  Solid angle that entity's new presence queries with.
   @param String.  Space should create the new entity in.

   Note: calling create_entity in a sandbox without the capabilities to create
   entities throws an exception.
 */
v8::Handle<v8::Value> root_createEntity(const v8::Arguments& args)
{
    if (args.Length() != 7)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error!  Requires <position vec>,<script type>, <script to execute>, <mesh uri>,<float scale>,<float solid_angle>,<space id> arguments")) );


    //decode root
    String errorMessageFRoot = "Error decoding the system object from root_createEntity.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageFRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageFRoot.c_str() )));

    // get the location from the args

    //get position
    Handle<Object> val_obj = ObjectCast(args[0]);
    if( !Vec3Validate(val_obj))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: must have a position vector as first argument")) );

    Vector3d pos(Vec3Extract(val_obj));

    //getting script type
    INLINE_STR_CONV_ERROR(args[1],createEntity,1,scriptType);

    // get the script to attach from the args
    //script is a string args
    INLINE_STR_CONV_ERROR(args[2],createEntity,2,scriptContents);

    //get the mesh to represent as
    INLINE_STR_CONV_ERROR(args[3],createEntity,3,mesh);



    //get the scale
    Handle<Object> scale_arg = ObjectCast(args[4]);
    if (!NumericValidate(scale_arg))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateEntity function. Wrong argument: require a number for scale.")) );

    float scale  =  NumericExtract(scale_arg);

    //get the solid angle
    Handle<Object> query_arg = ObjectCast(args[5]);
    if (!StringValidate(query_arg))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateEntity function. Wrong argument: require a string for query.")) );

    String new_query(StringExtract(query_arg));

    //get the space argument
    String spaceStr;
    String errSpaceMsg = "Error in create entity decoding string corresponding to space.  ";
    bool strDecode = decodeString(args[6],spaceStr,errSpaceMsg);
    if (!strDecode )
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errSpaceMsg.c_str())));

    SpaceID toCreateIn(spaceStr);

    //parse a bunch of arguments here
    EntityCreateInfo eci;
    eci.scriptType = scriptType;
    eci.mesh = mesh;
    eci.scriptOpts = "";
    eci.scriptContents = scriptContents;


    eci.loc  = Location(pos,Quaternion(1,0,0,0),Vector3f(0,0,0),Vector3f(0,0,0),0.0);

    eci.query = new_query;

    eci.scale = scale;
    eci.space = toCreateIn;

    return jsfake->struct_createEntity(eci);
}



/**
   @param Vec3 (eg. new util.Vec3(0,0,0);).  Corresponds to position to place
   new entity in world.
   @param String.  Script option to pass in.  Almost always pass "js"
   @param String.  New script to execute.
   @param String.  Mesh uri corresponding to mesh you want to use for this
   entity.
   @param Number.  Scale of new mesh.  (Higher number means increase mesh's size.)
   @param Number.  Solid angle that entity's new presence queries with.


   Note: calling create_entity in a sandbox without the capabilities to create
   entities throws an exception.
 */
v8::Handle<v8::Value> root_createEntityNoSpace(const v8::Arguments& args)
{
    if (args.Length() != 6)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error!  Requires <position vec>,<script type>, <script to execute>, <mesh uri>,<float scale>,<float solid_angle> arguments")) );


    //decode root
    String errorMessageFRoot = "Error decoding the system object from root_createEntity.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageFRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageFRoot.c_str() )));

    // get the location from the args

    //get position
    Handle<Object> val_obj = ObjectCast(args[0]);
    if( !Vec3Validate(val_obj))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: must have a position vector as first argument")) );

    Vector3d pos(Vec3Extract(val_obj));

    //getting script type
    INLINE_STR_CONV_ERROR(args[1],createEntNoSpace,1,scriptType);


    // get the script to attach from the args
    //script is a string args
    INLINE_STR_CONV_ERROR(args[2],createEntNoSpace,2,scriptContents);

    //get the mesh to represent as
    INLINE_STR_CONV_ERROR(args[3],createEntNoSpace,3,mesh);

    //get the scale
    Handle<Object> scale_arg = ObjectCast(args[4]);
    if (!NumericValidate(scale_arg))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateEntity function. Wrong argument: require a number for scale.")) );

    float scale  =  NumericExtract(scale_arg);

    //get the solid angle
    Handle<Object> query_arg = ObjectCast(args[5]);
    if (!StringValidate(query_arg))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateEntity function. Wrong argument: require a string for query.")) );

    String new_query(StringExtract(query_arg));



    //parse a bunch of arguments here
    EntityCreateInfo eci;
    eci.scriptType = scriptType;
    eci.mesh = mesh;
    eci.scriptOpts = "";
    eci.scriptContents = scriptContents;


    eci.loc  = Location(pos,Quaternion(1,0,0,0),Vector3f(0,0,0),Vector3f(0,0,0),0.0);

    eci.query = new_query;

    eci.scale = scale;


    return jsfake->struct_createEntity(eci);
}






/**
  @param the presence that the context is associated with.  (will use this as
  sender of messages).  If this arg is null, then just passes through the parent
  context's presence

  @param a visible object that can always send messages to.  if
  null, will use same spaceobjectreference as one passed in for arg0.

  @param permission number: an integer that indicates what the newly created
  sandbox can do.
*/
v8::Handle<v8::Value> root_createContext(const v8::Arguments& args)
{
    if (args.Length() != 3)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: must have 3 arguments: <presence to send/recv messages from (null if want to push through parent's presence)>, <JSVisible or JSPresence object that can always send messages to><permission int>")));

    String errorMessageWhichArg,errorMessage,errorMessageBase;
    errorMessageBase = "Error creating sandbox.  ";


    //jssystem decode
    String errorMsgSystem  = "Error decoding system when creating new context.  ";
    JSSystemStruct* jsfake = JSSystemStruct::decodeSystemStruct(args.This(),errorMsgSystem);
    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMsgSystem.c_str())));



    //jspresstruct decode
    JSPresenceStruct* jsPresStruct = NULL;
    if (! args[0]->IsNull())
    {
        errorMessageWhichArg= " 1.  ";
        errorMessage= errorMessageBase + errorMessageWhichArg;
        jsPresStruct = JSPresenceStruct::decodePresenceStruct(args[0],errorMessage);
        if (jsPresStruct == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));
    }

    //getting who can sendTo
    SpaceObjectReference canSendTo = SpaceObjectReference::null();
    if (args[1]->IsNull())
        canSendTo = jsPresStruct->getSporef();
    else
    {
        //should try to decode as a jspositionListener.  if decoding fails, throw error
        errorMessageWhichArg= " 2.  ";
        errorMessage= errorMessageBase + errorMessageWhichArg;

        JSPositionListener* jsposlist = decodeJSPosListener(args[1],errorMessage);

        if (jsposlist == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

        canSendTo = jsposlist->getSporef();
    }

    //decode permission number
    INLINE_DECODE_UINT_32_ERROR(args[2],createSandbox,3,permNum);

    return jsfake->struct_createContext(jsPresStruct,canSendTo,permNum);
}





/** Emits an event.
    @param callback The function to invoke once "time" number of seconds have passed
 */
v8::Handle<v8::Value> root_event(const v8::Arguments& args) {
    if ((args.Length() != 1))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("event() should only take a function handler to invoke.")) );

    v8::Handle<v8::Value> cb_val = args[0];

    //just returns the ScriptTimeout function
    String errorMessage      =  "Error decoding system in root_event of JSSystem.cpp.  ";
    JSSystemStruct* jsfake = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);
    if (jsfake == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    // Function
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Argument to event() must be a function.")) );
    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);

    return jsfake->struct_event(cb_persist);
}


/**
   @param time number of seconds to wait before executing the callback
   @param callback The function to invoke once "time" number of seconds have passed

   @param {Reserved} uint32 contextId
   @param {Reserved} double timeRemaining
   @param {Reserved} bool   isSuspended
   @param {Reserved} bool   isCleared


   timeout sets a timer.  When the number of seconds specified by arg 1 have
   elapsed, executes function specified by arg2.
 */
v8::Handle<v8::Value> root_timeout(const v8::Arguments& args)
{

    if ((args.Length() != 2) && (args.Length() != 6))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to ScriptTimeout of JSSystem.cpp.  Requires either two arguments(duration, callback) or requires six arguments (same as before + <uint32: contextId><double: timeRemaining><bool: isSuspended><bool: isCleared>.  ")) );

    v8::Handle<v8::Value> dur         = args[0];
    v8::Handle<v8::Value> cb_val      = args[1];


    //just returns the ScriptTimeout function
    String errorMessage      =  "Error decoding system in root_timeout of JSSystem.cpp.  ";
    JSSystemStruct* jsfake = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));


    // Duration
    double native_dur = 0;
    if (dur->IsNumber())
        native_dur = dur->NumberValue();
    else if (dur->IsInt32())
        native_dur = dur->Int32Value();
    else
        return v8::ThrowException( v8::Exception::Error(v8::String::New("In ScriptTimeout of JSSystem.cpp.  First argument incorrect: duration cannot be cast to float.")) );


    // Function
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("In ScriptTimeout of JSSystem.cpp.  Second argument incorrect: callback isn't a function.")) );


    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);

    if (args.Length() == 2)
        return jsfake->struct_createTimeout(native_dur, cb_persist);

    //resuming an already-created timeout.
    v8::Handle<v8::Value> contIDVal         = args[2];
    v8::Handle<v8::Value> timeRemainingVal  = args[3];
    v8::Handle<v8::Value> isSuspendedVal    = args[4];
    v8::Handle<v8::Value> isClearedVal      = args[5];


    uint32 contID;
    double timeRemaining;
    bool isSuspended,isCleared;

    if (! contIDVal->IsUint32())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Context id should be a uint32.")) );

    contID = contIDVal->ToUint32()->Value();

    if (! timeRemainingVal->IsNumber())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Time remaining should be a double.")) );
    timeRemaining = timeRemainingVal->ToNumber()->Value();

    if (! isSuspendedVal->IsBoolean())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Is suspended should be a boolean.")) );

    isSuspended = isSuspendedVal->ToBoolean()->Value();

    if (! isClearedVal->IsBoolean())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Is cleared should be a boolean.")) );

    isCleared = isClearedVal->ToBoolean()->Value();


    return jsfake->struct_createTimeout(native_dur, cb_persist, contID,timeRemaining,isSuspended,isCleared);
}


/**
   @param Function to execute when a presence created within this sandbox gets
   connected to the world.  Function takes a single argument that corresponds to
   the presence that just connected to the world.
 */
v8::Handle<v8::Value> root_onPresenceConnected(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceConnected.")) );

    v8::Handle<v8::Value> cb_val = args[0];
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceConnected().  Must contain callback function.")) );

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);


    //now decode system
    String errorMessageDecodeRoot = "Error decoding the system object from root_OnPresenceConnected.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageDecodeRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageDecodeRoot.c_str())));

    return jsfake->struct_registerOnPresenceConnectedHandler(cb_persist);
}



/**
   @param Function to execute when a presence created within this sandbox gets
   disconnected to the world.  Function takes a single argument that corresponds to
   the presence that just disconnected from the world.
 */
v8::Handle<v8::Value> root_onPresenceDisconnected(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceDisconnected.")) );

    v8::Handle<v8::Value> cb_val = args[0];
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceDisconnected().  Must contain callback function.")) );

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);


    //now decode system
    String errorMessageDecodeRoot = "Error decoding the system object from root_OnPresenceDisconnected.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageDecodeRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageDecodeRoot.c_str())));

    return jsfake->struct_registerOnPresenceDisconnectedHandler(cb_persist);
}





}//end jssystem namespace
}//end js namespace
}//end sirikata
