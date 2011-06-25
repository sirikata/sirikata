#include "JSSystem.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"
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


namespace Sirikata {
namespace JS {
namespace JSSystem {



/**
   @param Which presence to send from.
   @param Message object to send.
   @param Visible to send to.
   @param (Optional) Error handler function.

   Sends a message to the presence associated with this visible object from the
   presence that can see this visible object.
 */
v8::Handle<v8::Value> sendMessage (const v8::Arguments& args)
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

    //decode string argument
    v8::Handle<v8::Value> messageBody = args[1];
    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object.")) );

    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    std::string serialized_message = JSSerializer::serializeObject(v8Object);


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

    return jsfake->sendMessageNoErrorHandler(jspres,serialized_message,jspl);
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

    if (!args[0]->IsObject())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Must pass in an *object* to serialize.")));

    Local<v8::Object> v8Object = args[0]->ToObject();
    String stringifiedObject = JSSerializer::serializeObject(v8Object);

    String errorMessage = "Error decoding error message when serializing object";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));


    v8::Handle<v8::Value> returner = strToUint16Str(stringifiedObject);

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

    return jssys->deserializeObject(serString);
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
//address of visible watchingFrom;
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
    if ((args.Length() != 12) && (args.Length() != 1))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in createVisible call.  Require either a single string argument to create visible object.  Or requires three arguments: <visible from string> <visible to string> <vec3 position>")));

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
        return jssys->struct_create_vis(sporefVisWatching,NULL);


    //decode second arg: sporef watching from
    SpaceObjectReference sporefVisWatchingFrom;
    errMsg = baseErrMsg + "Could not decode second argument.  ";
    sporefDecoded = decodeSporef(args[1],sporefVisWatchingFrom,errMsg);

    if (! sporefDecoded)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //decode third-fifth args: timed motion vector
    TimedMotionVector3f location;
    errMsg = baseErrMsg + "Could not decode 3-5 arguments corresponding to position.  ";
    bool decodedTMV = decodeTimedMotionVector(args[2],args[3],args[4], location,errMsg);

    if (! decodedTMV)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));

    //decode sixth-eighth args: timed motion quaternion
    TimedMotionQuaternion orientation;
    errMsg = baseErrMsg + "Could not decode 6-8 arguments corresponding to orientation.  ";
    bool decodedTMQ = decodeTimedMotionQuat(args[5],args[6],args[7], orientation,errMsg);

    if (! decodedTMQ)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //decode ninth-tenth args: bounding sphere
    BoundingSphere3f bsph;
    errMsg = baseErrMsg + "Could not decode 9-10 arguments corresponding to bounding sphere.  ";
    bool decodedBSPH = decodeBoundingSphere3f(args[8],args[9], bsph,errMsg);

    if (! decodedBSPH)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //decode eleventh arg: mesh string
    String meshString;
    errMsg = baseErrMsg + "Could not decode 11th argument corresponding to mesh string.  ";
    bool meshDecoded = decodeString(args[10],meshString,errMsg);

    if (! meshDecoded)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));


    //decode twelfth arg: physics string
    String physicsString;
    errMsg = baseErrMsg + "Could not decode 12th argument corresponding to physics string.  ";
    bool physicsDecoded = decodeString(args[11],physicsString,errMsg);

    if (! physicsDecoded)
        return v8::ThrowException( v8::Exception::Error(v8::String::New( errMsg.c_str())));

    //fixme : lkjs

    // bool isVisible = false;
    // VisAddParams vap(&sporefVisWatchingFrom,&location,&orientation,&bsph,&meshString,&physicsString,&isVisible);
//    return jssys->struct_create_vis(sporefVisWatching,&vap);

    return v8::ThrowException( v8::Exception::Error(v8::String::New("In re-write of pos listener.  Need to re-work root_createVisible.")));
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
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Require only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];

    StringCheckAndExtract(native_filename, filename);
    String errorMessage = "Error decoding the system object from root_canSendMessage.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));

    return jsfake->struct_require(native_filename);
}



/**
  Takes no parameters.  Destroys all created objects, except presences in the
  root context.  Then executes script associated with root context.  (Use
  system.setScript to set this script.)
 */
v8::Handle<v8::Value> root_reset(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error. reset takes no arguments.")));


    String errorMessage = "Error in reset of system object.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(), errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str() )));

    return jsfake->struct_reset();
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
v8::Handle<v8::Value> root_canProx(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_canProx.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_canProx();
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




/**
   @param String corresponding to the filename to look for file to include.

   Library include mechanism.  Calling import makes it so that system searches
   for file named by argument passed in.  Regardless of whether system has
   already executed this file, it reads file, and executes it.  (See require as
   well.)
 */
v8::Handle<v8::Value> root_import(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Import only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];



    //decode the filename to import from.
    String strDecodeErrorMessage = "Error decoding string as first argument of root_import of jssystem.  ";
    String native_filename; //string to decode to.
    bool decodeStrSuccessful = decodeString(args[0],native_filename,strDecodeErrorMessage);
    if (! decodeStrSuccessful)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(strDecodeErrorMessage.c_str(), strDecodeErrorMessage.length())) );



    //decode the system object
    String errorMessage = "Error decoding the system object from root_import.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_import(native_filename);
}

/**
   @return String corresponding to version number of Emerson.
 */
v8::Handle<v8::Value> root_getVersion(const v8::Arguments& args)
{
    return v8::String::New( JSSystemNames::EMERSON_VERSION);
}

/**
   @return Vec3 corresponding to position of default presence sandbox is
   associated with.  Calling from root sandbox, or calling on a sandbox for
   which you do not have capabilities to query for position throws an exception.
 */
v8::Handle<v8::Value> root_getPosition(const v8::Arguments& args)
{
    String errorMessage = "Error decoding the system object from root_getPosition.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jsfake->struct_getPosition();
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

    if (args.Length() != 17)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when trying to restore presence through system object.  restore_presence requires 17 arguments")));


    v8::Handle<v8::Value> mSporefArg                       = args[0];
    v8::Handle<v8::Value> posArg                           = args[1];
    v8::Handle<v8::Value> velArg                           = args[2];
    v8::Handle<v8::Value> posTimeArg                       = args[3];
    v8::Handle<v8::Value> orientArg                        = args[4];
    v8::Handle<v8::Value> orientVelArg                     = args[5];
    v8::Handle<v8::Value> orientTimeArg                    = args[6];
    v8::Handle<v8::Value> meshArg                          = args[7];
    v8::Handle<v8::Value> scaleArg                         = args[8];
    v8::Handle<v8::Value> isClearedArg                     = args[9];
    v8::Handle<v8::Value> contextIDArg                     = args[10];
    v8::Handle<v8::Value> isConnectedArg                   = args[11];
    v8::Handle<v8::Value> connectedCallbackArg             = args[12];
    v8::Handle<v8::Value> isSuspendedArg                   = args[13];
    v8::Handle<v8::Value> suspendedVelocityArg             = args[14];
    v8::Handle<v8::Value> suspendedOrientationVelocityArg  = args[15];
    v8::Handle<v8::Value> solidAngleQueryArg               = args[16];

    //now, it's time to decode them.

    String baseErrMsg = "Error in restorePresence.  Could not decode ";

    String specificErrMsg =baseErrMsg + "sporef.";
    SpaceObjectReference mSporef;
    bool sporefDecoded = decodeSporef(mSporefArg, mSporef, specificErrMsg);
    if (! sporefDecoded)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));

    TimedMotionVector3f mPos;
    specificErrMsg = baseErrMsg + "timed motion vector.";
    bool mPosDecoded = decodeTimedMotionVector(posArg,velArg,posTimeArg,mPos,specificErrMsg);
    if (! mPosDecoded)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    TimedMotionQuaternion mOrient;
    specificErrMsg = baseErrMsg + "timed motion quaternion.";
    bool mOrientDecoded = decodeTimedMotionQuat(orientArg,orientVelArg,orientTimeArg,mOrient,specificErrMsg);
    if (! mOrientDecoded)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    String mesh;
    specificErrMsg = baseErrMsg + "mesh.";
    bool meshDecodeSuccessful = decodeString(meshArg, mesh, specificErrMsg);
    if (! meshDecodeSuccessful)
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


    uint32 contextID;
    specificErrMsg = baseErrMsg + "contextID.";
    bool contextIDDecodeSuccessful = decodeUint32(contextIDArg, contextID, specificErrMsg);
    if (! contextIDDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));

    bool isConnected;
    specificErrMsg = baseErrMsg + "isConnected.";
    bool isConnectedDecodeSuccessful = decodeBool(isConnectedArg, isConnected, specificErrMsg);
    if (! isConnectedDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    specificErrMsg = baseErrMsg + "connectedCallback.";
    v8::Handle<v8::Function>connCB;
    if (connectedCallbackArg->IsFunction())
        connCB = v8::Handle<v8::Function>::Cast(connectedCallbackArg);
    else if (! connectedCallbackArg->IsNull())
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));

    bool isSuspended;
    specificErrMsg = baseErrMsg + "isSuspended.";
    bool isSuspendedDecodeSuccessful = decodeBool(isSuspendedArg, isSuspended, specificErrMsg);
    if (! isSuspendedDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));


    Vector3f suspendedVelocity;
    specificErrMsg = baseErrMsg + "suspendedVelocity.";
    if (!Vec3ValValidate(suspendedVelocityArg))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));
    suspendedVelocity = Vec3ValExtractF(suspendedVelocityArg);

    Quaternion suspendedOrientationVelocity;
    specificErrMsg = baseErrMsg + "suspendedOrientationVelocity.";
    if (!QuaternionValValidate(suspendedOrientationVelocityArg))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(specificErrMsg.c_str())));
    suspendedOrientationVelocity = QuaternionValExtract(suspendedOrientationVelocityArg);



    INLINE_SA_CONV_ERROR(solidAngleQueryArg,restorePresence,16,queryAngle);


    //decode system.
    String errorMessageFRoot = "Error decoding the system object from restorePresence.  ";
    JSSystemStruct* jssys  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageFRoot);

    if (jssys == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageFRoot.c_str() )));

    PresStructRestoreParams restParams(
        &mSporef,
        &mPos,
        &mOrient,
        &mesh,
        &scale,
        &isCleared,
        &contextID,
        &isConnected,
        (connCB.IsEmpty() ? NULL : &connCB),
        &isSuspended,
        &suspendedVelocity,
        &suspendedOrientationVelocity,
        &queryAngle
    );


    return handle_scope.Close(jssys->restorePresence(restParams));
}


/**
   @param String that contains a uri for a mesh for the new presence.
   @param Function to be called when presence gets connected to the world.
   (Function has form func (pres), where pres contains the presence just
   connected.)
   @return Returns nothing.

   Note: throws an exception if sandbox does not have capability to create
   presences.
   Note 2: Presence's initial position is the same as the presence that created
   it.  Scale is set to 1.
*/
v8::Handle<v8::Value> root_createPresence(const v8::Arguments& args)
{
    if (args.Length() != 4)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when trying to create presence through system object.  create_presence requires two arguments: <string mesh uri> <initialization function for presence><position><space>")));

    //check args.
    //mesh arg
    String newMesh = "";
    String errorMessage = "Error decoding first argument of create_presence.  Should be a string corresponding to mesh uri.  ";
    bool stringDecodeSuccessful = decodeString(args[0], newMesh, errorMessage);
    if (! stringDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    //callback function arg
    if (! args[1]->IsFunction())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error while creating new presence through system object.  create_presence requires that the second argument passed in be a function")));

    //position argument
    if (! Vec3ValValidate(args[2]))
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error while creating new presence through system object.  create_presence requires that third argument passed in be a vec3.")));

    Vector3d poser = Vec3ValExtract(args[2]);

    //space argument
    String spaceStr;
    String errSpaceMsg = "Error in create presence decoding string corresponding to space.  ";
    bool strDecode = decodeString(args[3],spaceStr,errSpaceMsg);
    if (!strDecode )
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errSpaceMsg.c_str())));

    SpaceID toCreateIn(spaceStr);


    //decode root
    String errorMessageFRoot = "Error decoding the system object from root_createPresence.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageFRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageFRoot.c_str() )));

    return jsfake->struct_createPresence(newMesh, v8::Handle<v8::Function>::Cast(args[1]),poser,toCreateIn);
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
    Handle<Object> qa_arg = ObjectCast(args[5]);
    if (!NumericValidate(qa_arg))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateEntity function. Wrong argument: require a number for query angle.")) );

    SolidAngle new_qa(NumericExtract(qa_arg));

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

    eci.solid_angle = new_qa;
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
    Handle<Object> qa_arg = ObjectCast(args[5]);
    if (!NumericValidate(qa_arg))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in ScriptCreateEntity function. Wrong argument: require a number for query angle.")) );

    SolidAngle new_qa(NumericExtract(qa_arg));



    //parse a bunch of arguments here
    EntityCreateInfo eci;
    eci.scriptType = scriptType;
    eci.mesh = mesh;
    eci.scriptOpts = "";
    eci.scriptContents = scriptContents;


    eci.loc  = Location(pos,Quaternion(1,0,0,0),Vector3f(0,0,0),Vector3f(0,0,0),0.0);

    eci.solid_angle = new_qa;
    eci.scale = scale;


    return jsfake->struct_createEntity(eci);
}






/**
  @param the presence that the context is associated with.  (will use this as
  sender of messages).  If this arg is null, then just passes through the parent
  context's presence

  @param a visible object that can always send messages to.  if
  null, will use same spaceobjectreference as one passed in for arg0.

  @param Boolean.  can I send messages to everyone?

  @param Boolean.  can I receive messages from everyone?

  @param Boolean.  can I make my own prox queries argument

  @param Boolean.  can I import argument

  @param Boolean.  can I create presences.

  @param Boolean.  can I create entities.

  @param Boolean.  can I call eval directly through system object.
*/
v8::Handle<v8::Value> root_createContext(const v8::Arguments& args)
{
    if (args.Length() != 9)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: must have three arguments: <presence to send/recv messages from (null if want to push through parent's presence)>, <JSVisible or JSPresence object that can always send messages to><bool can I send to everyone?>, <bool can I receive from everyone?> , <bool, can I make my own proximity queries>, <bool, can I import code?>, <bool, can I create presences?>,<bool, can I create entities?>,<bool, can I call eval directly?>")) );


    bool sendEveryone,recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval;
    String errorMessageBase = "In ScriptCreateContext.  Trying to decode argument ";
    String errorMessageWhichArg,errorMessage;

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


    //send everyone decode
    errorMessageWhichArg= " 3.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[2],sendEveryone, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    //recv everyone decode
    errorMessageWhichArg= " 4.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[3],recvEveryone, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));


    //recv everyone decode
    errorMessageWhichArg= " 5.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[4],proxQueries, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    //import decode
    errorMessageWhichArg= " 6.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[5],canImport, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));


    //can create presences
    errorMessageWhichArg= " 7.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[6],canCreatePres, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    //can create entities
    errorMessageWhichArg= " 8.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[7],canCreateEnt, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    //can eval
    errorMessageWhichArg= " 9.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[8],canEval, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsfake->struct_createContext(canSendTo,sendEveryone,recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval,jsPresStruct);
}



/**
   @param String corresponding to valid Emerson code to execute.

   Executes string within current sandbox.
 */
v8::Handle<v8::Value> root_scriptEval(const v8::Arguments& args)
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

    return jsfake->struct_eval(native_contents,&origin);
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


/** Registers a handler to be invoked for events that match the
 *  specified pattern, where the pattern is a list of individual
 *  rules.
 *
 *   @param cb: callback to invoke, with event as parameter
 *   @param  object target: target of callback (this pointer when invoked), or null for the global (root) object
 *   @param pattern[] pattterns: Array of Pattern rules to match
 *   @param sender: a visible object if want to match only messages from a
 *   particular sender, or null if want to match messages from any sender.
 */
v8::Handle<v8::Value> root_registerHandler(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;

    if ((args.Length() != 3) && (args.Length() != 4))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().  Need exactly 3 or 4 args.  <function, callback to execute when event associated with handler fires>, <pattern: array of pattern rules to match or null if can match all>, <a sender to match even to><(optional) bool: whether handler is suspended>")) );

    // Changing the sequence of the arguments so as to get the same
    // as is generated in emerson

    v8::Handle<v8::Value> cb_val     = args[0];
    v8::Handle<v8::Value> pattern    = args[1];
    v8::Handle<v8::Value> sender_val = args[2];

    // Pattern
    PatternList native_patterns;
    if (! pattern->IsNull())
    {
        if (PatternValidate(pattern))
        {
            Pattern single_pattern = PatternExtract(pattern);
            native_patterns.push_back(single_pattern);
        }
        else if (pattern->IsArray())
        {
            v8::Handle<v8::Array> pattern_array( v8::Handle<v8::Array>::Cast(pattern) );
            if (pattern_array->Length() == 0)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Pattern array must contain at least one element.")) );
            for(uint32 pat_idx = 0; pat_idx < pattern_array->Length(); pat_idx++)
            {
            Local<Value> pattern_element = pattern_array->Get(pat_idx);
            if (!PatternValidate(pattern_element))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Found non-pattern element in array of patterns.")) );
            Pattern single_pattern = PatternExtract(pattern_element);
            native_patterns.push_back(single_pattern);
            }
        }
    }
    else
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Pattern argument must be pattern or array of patterns.")) );


    // Sender
    if (! sender_val->IsNull())  //means that it's a valid sender
    {
        String errorMessage = "[JS] Error in ScriptRegisterHandler of JSSystem.cpp.  Having trouble decoding sender.  ";
        JSPositionListener* jsposlist = decodeJSPosListener(sender_val,errorMessage);

        if (jsposlist == NULL)
            return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));
    }


    v8::Handle<v8::Object> sender = v8::Handle<v8::Object>::Cast(sender_val);
    v8::Persistent<v8::Object> sender_persist = v8::Persistent<v8::Object>::New(sender);


    // Function
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().  Must contain callback function.")) );


    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);


    //now decode system
    String errorMessageDecodeRoot = "Error decoding the system object from root_registerHandler.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageDecodeRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageDecodeRoot.c_str())));

    if (args.Length() == 3)
        return handle_scope.Close(jsfake->struct_makeEventHandlerObject(native_patterns, cb_persist, sender_persist, false));


    v8::Handle<v8::Value> suspVal = args[3];
    String errMsgDecodeSusp = "Error decoding suspended argument when registering handler with system.  ";
    bool isSuspended;
    bool decodeSusp = decodeBool(suspVal, isSuspended, errMsgDecodeSusp);

    if (! decodeSusp)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsgDecodeSusp.c_str())));

    return handle_scope.Close(jsfake->struct_makeEventHandlerObject(native_patterns, cb_persist, sender_persist, isSuspended));
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
