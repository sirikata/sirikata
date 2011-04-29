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


namespace Sirikata {
namespace JS {
namespace JSSystem {

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
    std::cout<<"\n\nDebug: Got into root_print\n\n";
    std::cout.flush();
    
    
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in root_print.  Requires exactly one argument: a string to print.")));
    
    String errorMessage = "Error decoding the system object from root_print.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessage);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


    v8::String::Utf8Value str(args[0]);
    String toPrint( ToCString(str));
    
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

    
    //decode root
    String errorMessageFRoot = "Error decoding the system object from root_createPresence.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageFRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageFRoot.c_str() )));
    
    return jsfake->struct_createPresence(newMesh, v8::Handle<v8::Function>::Cast(args[1]));
}



/**
   @param Vec3 (eg. new util.Vec3(0,0,0);).  Corresponds to position to place
   new entity in world.
   @param String.  Script option to pass in.  Almost always pass "js"
   @param String.  Name of file to import code for new entity from.
   @param String.  Mesh uri corresponding to mesh you want to use for this
   entity.
   @param Number.  Scale of new mesh.  (Higher number means increase mesh's size.)
   @param Number.  Solid angle that entity's new presence queries with.
   

   Note: calling create_entity in a sandbox without the capabilities to create
   entities throws an exception.
 */
v8::Handle<v8::Value> root_createEntity(const v8::Arguments& args)
{
    if (args.Length() != 6)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error!  Requires <position vec>,<script type>, <script filename>, <mesh uri>,<float scale>,<float solid_angle> arguments")) );


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
    v8::String::Utf8Value strScriptType(args[1]);
    const char* cstrType = ToCString(strScriptType);
    String scriptType(cstrType);

    // get the script to attach from the args
    //script is a string args
    v8::String::Utf8Value scriptOpters(args[2]);
    const char* cstrOpts = ToCString(scriptOpters);
    String scriptOpts (cstrOpts);
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
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

        canSendTo = jsposlist->getToListenTo();
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
   @param Number.  How long to wait before executing callback function (2nd
   arg).  (Units of seconds.)
   @param Funciton.

   timeout sets a timer.  When the number of seconds specified by arg 1 have
   elapsed, executes function specified by arg2.
 */
v8::Handle<v8::Value> root_timeout(const v8::Arguments& args)
{

    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to ScriptTimeout of JSSystem.cpp.  First arg should be duration, second argumnet is callback")) );

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

    return jsfake->struct_createTimeout(Duration::seconds(native_dur), cb_persist);
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
    if (args.Length() != 4)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().  Need exactly 4 args.  <function, callback to execute when event associated with handler fires>, <target_val (this pointer when invoked)>, <pattern: array of pattern rules to match or null if can match all>, <a sender to match even to>")) );

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

    
    //now decode system
    String errorMessageDecodeRoot = "Error decoding the system object from root_registerHandler.  ";
    JSSystemStruct* jsfake  = JSSystemStruct::decodeSystemStruct(args.This(),errorMessageDecodeRoot);

    if (jsfake == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessageDecodeRoot.c_str())));

    return jsfake->struct_makeEventHandlerObject(native_patterns, target_persist, cb_persist, sender_persist);
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


