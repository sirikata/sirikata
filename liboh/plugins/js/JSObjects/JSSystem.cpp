#include "JSSystem.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"
#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "../JSObjectStructs/JSVisibleStruct.hpp"

namespace Sirikata{
namespace JS{
namespace JSSystem{



v8::Handle<v8::Value> ScriptCreatePresence(const v8::Arguments& args)
{
    if (args.Length() != 2)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when trying to create presence through system object.  create_presence requires two arguments: <string mesh uri> <initialization function for presence>")));

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


    String jsobjErrorMessage = "Error decoding JSObjectScript from system object in ScriptCreatePresence of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), jsobjErrorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(jsobjErrorMessage.c_str(),jsobjErrorMessage.length())));

    return target_script->create_presence(newMesh,v8::Handle<v8::Function>::Cast(args[1]));
}



//fake root in context can already send messages to who instantiated it and
//receive messages from who instantiated it.
//messages sent out of it get stamped with a port number automatically

//argument 0: the presence that the context is associated with.  (will use
//this as sender of messages).
//argument 1: a visible object that can always send messages to.  if null, will
//use same spaceobjectreference as one passed in for arg0.
//argument 2: true/false.  can I send messages to everyone?
//argument 3: true/false.  can I receive messages from everyone?
//argument 4: true/false.  can I make my own prox queries
v8::Handle<v8::Value> ScriptCreateContext(const v8::Arguments& args)
{
    if (args.Length() != 5)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: must have three arguments: <presence to send/recv messages from>, <JSVisible or JSPresence object that can always send messages to><bool can I send to everyone?>, <bool can I receive from everyone?> , <bool, can I make my own proximity queries>")) );


    bool sendEveryone,recvEveryone,proxQueries;
    String errorMessageBase = "In ScriptCreateContext.  Trying to decode argument ";
    String errorMessageWhichArg,errorMessage;


    //jspresstruct decode
    errorMessageWhichArg= " 1.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    JSPresenceStruct* jsPresStruct = JSPresenceStruct::decodePresenceStruct(args[0],errorMessage);
    if (jsPresStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );


    //getting who can sendTo
    SpaceObjectReference* canSendTo = NULL;
    if (args[1]->IsNull())
        canSendTo = jsPresStruct->getSporef();
    else
    {
        //should try to decode as a jspositionListener.  if decoding fails, throw error
        errorMessageWhichArg= " 2.  ";
        errorMessage= errorMessageBase + errorMessageWhichArg;

        JSPositionListener* jsposlist = decodeJSPosListener(args[1],errorMessage);

        if (jsposlist == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );

        canSendTo = jsposlist->getToListenTo();
    }


    //send everyone decode
    errorMessageWhichArg= " 3.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[2],sendEveryone, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );

    //recv everyone decode
    errorMessageWhichArg= " 4.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[3],recvEveryone, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );


    //recv everyone decode
    errorMessageWhichArg= " 5.  ";
    errorMessage= errorMessageBase + errorMessageWhichArg;
    if (! decodeBool(args[4],proxQueries, errorMessage))
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );


    return jsPresStruct->struct_createContext(canSendTo,sendEveryone,recvEveryone,proxQueries);
}



//first argument is the position of the new entity
//second argument is the name of the file to execute scripts from
//third argument is the mesh file to use.
v8::Handle<v8::Value> ScriptCreateEntity(const v8::Arguments& args)
{
    if (args.Length() != 6)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error!  Requires <position vec>,<script type>, <script filename>, <mesh uri>,<float scale>,<float solid_angle> arguments")) );

    //try to decode the object script
    String errorMessage = "Error decoding JSObjectScript from system object in ScriptCreateEntity of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));



    // get the location from the args

    //get position
    Handle<Object> val_obj = ObjectCast(args[0]);
    if( !Vec3Validate(val_obj))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error: must have a position vector as first argument")) );

    Vector3d pos(Vec3Extract(val_obj));

    //getting script type
    v8::String::Utf8Value strScriptType(args[1]);
    const char* cstrType = ToCString(strScriptType);
    String scriptType(cstrType);

    // get the script to attach from the args
    //script is a string args
    v8::String::Utf8Value scriptOpters(args[2]);
    const char* cstrOpts = ToCString(scriptOpters);
    String scriptOpts(cstrOpts);
    scriptOpts = "--init-script="+scriptOpts;

    //get the mesh to represent as
    v8::String::Utf8Value mesh_str(args[3]);
    const char* mesh_cstr = ToCString(mesh_str);
    String mesh(mesh_cstr);

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
    eci.scriptOpts = scriptOpts;


    eci.loc  = Location(pos,Quaternion(1,0,0,0),Vector3f(0,0,0),Vector3f(0,0,0),0.0);

    eci.solid_angle = new_qa;
    eci.scale = scale;


    //target_script->create_entity(pos, script,mesh);
    target_script->create_entity(eci);


    return v8::Undefined();
}

v8::Handle<v8::Value> ScriptReboot(const v8::Arguments& args)
{
  if(args.Length() != 0)
  {
     return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to reboot()")) );
  }
   // get the c++ object out from the js object
   String errorMessage = "Error decoding JSObjectScript from system object in ScriptReboot of JSSystem.cpp.  ";
   JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
   if (target_script == NULL)
       return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));


   // invoke the reboot in the script
   target_script->reboot();

   return v8::Undefined();
}

v8::Handle<v8::Value> ScriptTimeout(const v8::Arguments& args)
{
    return ScriptTimeoutContext(args,NULL);
}



v8::Handle<v8::Value> ScriptTimeoutContext(const v8::Arguments& args,JSContextStruct* jscont)
{

    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to ScriptTimeout of JSSystem.cpp.  First arg should be duration, second is callback")) );

    v8::Handle<v8::Value> dur         = args[0];
    v8::Handle<v8::Value> cb_val      = args[1];

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
        return v8::ThrowException( v8::Exception::Error(v8::String::New("In ScriptTimeout of JSSystem.cpp.  Third argument incorrect: callback isn't a function.")) );


    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);


    if (jscont == NULL)
    {
        String errorMessage = "Error decoding JSObjectScript from system object in ScriptTimeoutContext of JSSystem.cpp.  ";
        JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
        if (target_script == NULL)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

        return target_script->create_timeout(Duration::seconds(native_dur), cb_persist, jscont);
    }

    //means that this is the
    return jscont->jsObjScript->create_timeout(Duration::seconds(native_dur), cb_persist, jscont);
}



v8::Handle<v8::Value> ScriptImport(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Import only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];

    StringCheckAndExtract(native_filename, filename);
    String errorMessage = "Error decoding JSObjectScript from system object in ScriptImport of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));



    target_script->import(native_filename);

    return v8::Undefined();
}

v8::Handle<v8::Value> ScriptRequire(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Require only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];

    StringCheckAndExtract(native_filename, filename);
    String errorMessage = "Error decoding JSObjectScript from system object in ScriptImport of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    target_script->require(native_filename);
    return v8::Undefined();
}


v8::Handle<v8::Value> ScriptEval(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Eval only takes one parameter: the program text to evaluate.")) );

    v8::Handle<v8::Value> contents = args[0];

    StringCheckAndExtract(native_contents, contents);
    String errorMessage = "Error decoding JSObjectScript from system object in ScriptEval of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    ScriptOrigin origin = args.Callee()->GetScriptOrigin();
    return target_script->eval(native_contents, &origin);
}




// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args)
{

    String errorMessage = "Error decoding JSObjectScript from system object in Print of JSSystem.cpp.  ";
    JSObjectScript* target = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));


    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope;
        if (first) {
            first = false;
        } else {
            target->print(" ");
        }
        v8::String::Utf8Value str(args[i]);
        const char* cstr = ToCString(str);
        target->print(cstr);
    }
    return v8::Undefined();
}



/** Registers a handler to be invoked for events that match the
 *  specified pattern, where the pattern is a list of individual
 *  rules.
 *  Arguments:
 *   pattern[] pattterns: Array of Pattern rules to match
 *   object target: target of callback (this pointer when invoked), or null for
 *                  the global (root) object
 *   function cb: callback to invoke, with event as parameter
 */
v8::Handle<v8::Value> ScriptRegisterHandler(const v8::Arguments& args)
{
    if (args.Length() != 4)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().")) );

    // Changing the sequence of the arguments so as to get the same
    // as is generated in emerson

    v8::Handle<v8::Value> cb_val     = args[0];
    v8::Handle<v8::Value> target_val = args[1];
    v8::Handle<v8::Value> pattern    = args[2];
    v8::Handle<v8::Value> sender_val = args[3];


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


    // Target
    if (!target_val->IsObject() && !target_val->IsNull() && !target_val->IsUndefined())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Target is not object or null.")) );

    v8::Handle<v8::Object> target = v8::Handle<v8::Object>::Cast(target_val);
    v8::Persistent<v8::Object> target_persist = v8::Persistent<v8::Object>::New(target);

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

    String errorMessage = "Error decoding JSObjectScript from system object in ScriptRegisterHandler of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));


    JSEventHandlerStruct* evHand = target_script->registerHandler(native_patterns, target_persist, cb_persist, sender_persist);

    return target_script->makeEventHandlerObject(evHand);
}

/** Registers a handler for presence connection events.
 *  Arguments:
 *   function cb: callback to invoke, with event as parameter
 */
v8::Handle<v8::Value> ScriptOnPresenceConnected(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceConnected.")) );

    v8::Handle<v8::Value> cb_val = args[0];
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceConnected().  Must contain callback function.")) );

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);


    String errorMessage = "Error decoding JSObjectScript from system object in ScriptOnPresenceConnected of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));


    target_script->registerOnPresenceConnectedHandler(cb_persist);

    return v8::Undefined();
}

v8::Handle<v8::Value> ScriptOnPresenceDisconnected(const v8::Arguments& args) {
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceDisconnected.")) );

    v8::Handle<v8::Value> cb_val = args[0];
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onPresenceDisconnected().  Must contain callback function.")) );

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);


    String errorMessage = "Error decoding JSObjectScript from system object in ScriptOnPresenceDisconnected of JSSystem.cpp.  ";
    JSObjectScript* target_script = JSObjectScript::decodeSystemObject(args.This(), errorMessage);
    if (target_script == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    target_script->registerOnPresenceDisconnectedHandler(cb_persist);

    return v8::Undefined();
}


}
}
}//sirikata namespace
