/*  Sirikata
 *  JSObjectScriptManager.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/oh/Platform.hpp>

#include "JSObjectScriptManager.hpp"
#include "JSObjectScript.hpp"

#include "JSVec3.hpp"
#include "JSQuaternion.hpp"


#include "JSPattern.hpp"

#include "JS_JSMessage.pbj.hpp"


namespace Sirikata {
namespace JS {

static const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
static v8::Handle<v8::Value> Print(const v8::Arguments& args) {
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

v8::Handle<v8::Value> __ScriptGetTest(const v8::Arguments& args) {
    JSObjectScript* target = GetTargetJSObjectScript(args);
    target->test();
    return v8::Undefined();
}


//bftm
v8::Handle<v8::Value> __ScriptTestBroadcastMessage(const v8::Arguments& args)
{
   if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to broadcast(<message>)")) );

    v8::Handle<v8::Value> messageBody = args[0];


		//bhupc


		if(!messageBody->IsObject())
		{
			return v8::ThrowException(v8::Exception::Error(v8::String::New("MEssage shoudl be an object")) );
		}

		Local<v8::Object> v8Object = messageBody->ToObject();

		Local<v8::Array> properties = v8Object->GetPropertyNames();

		Protocol::JSMessage jsmessage ;
		for( unsigned int i = 0; i < properties->Length(); i++)
		{
					Local<v8::Value> value1 = properties->Get(i);


					Local<v8::Value> value2 =
					v8Object->Get(properties->Get(i));


					// create a JSField out of this




			    v8::String::Utf8Value
					msgBodyArgs1(value1);

					const char* cMsgBody1 = ToCString(msgBodyArgs1);
					std::string cStrMsgBody1(cMsgBody1);


					//std::cout << cStrMsgBody1 << " = ";

					v8::String::Utf8Value
					msgBodyArgs2(value2);

					const char* cMsgBody2 = ToCString(msgBodyArgs2);
					std::string cStrMsgBody2(cMsgBody2);


					//std::cout << cStrMsgBody2 << "\n";


					Protocol::IJSField jsf = jsmessage.add_fields();

					jsf.set_name(cStrMsgBody1);
					Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
					jsf_value.set_s_value(cStrMsgBody2);




		}


		// serialize the jsmessage

		//RoutableMessageBody body;




//    v8::String::Utf8Value msgBodyArgs(args[0]);
//    const char* cMsgBody = ToCString(msgBodyArgs);
//    std::string cStrMsgBody(cMsgBody);

//    std::cout<<"\n\n\n";
//    std::cout<<"This is messageBody:  "<<cMsgBody;
//    std::cout<<"\n\n\n";

  JSObjectScript* target = GetTargetJSObjectScript(args);
	std::string serialized_message;
	jsmessage.SerializeToString(&serialized_message);

	std::cout << "Serialized message " << "\n" <<
	serialized_message.size() << serialized_message <<
	"\n";



  target->bftm_testSendMessageBroadcast(serialized_message);


    return v8::Undefined();
}


//bftm
v8::Handle<v8::Value> __ScriptAddressable(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to addressable(index)")) );

    int numIndex = (int)(args[0]->Uint32Value());
    v8::Handle<v8::Value> messageBody = args[0];
    JSObjectScript* target = GetTargetJSObjectScript(args);

    if (numIndex >= target->getAddressableSize())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Out of range: please query again with a smaller index.")));

    //FIXME: need to figure out how to pass back a c++ reference to java
    //script.  Later, javascript will pass back that reference when sending
    //messages


    return v8::ThrowException( v8::Exception::Error(v8::String::New("FIXME: need to figure out how to pass back a c++ reference to javascript.  Later, javascript will pass back that reference when sending messages")));

}

//first argument is the destination (in the internally stored vector of
//potential destinations
//second argument is the destination sending to
v8::Handle<v8::Value> __SendMessageTo(const v8::Arguments& args)
{
    if (args.Length() != 2)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sendMessageTo(index,msg)")) );

    int numIndex = (int)(args[0]->Uint32Value());
    v8::Handle<v8::Value> indexTo = args[0];

    v8::String::Utf8Value msgBodyArgs(args[1]);
    const char* cMsgBody = ToCString(msgBodyArgs);
    std::string cStrMsgBody(cMsgBody);


    JSObjectScript* target = GetTargetJSObjectScript(args);
    target->sendMessageToEntity(numIndex,cStrMsgBody);

    return v8::Undefined();
}

//Oref template messages


void readORef(const v8::Arguments& args, JSObjectScript*& caller, ObjectReference*& oref)
{
   v8::Local<v8::Object> mRef = args.This();

   //grabs the caller
   v8::Local<v8::External> wrapJSObj;
   if (mRef->InternalFieldCount() > 0)
       wrapJSObj = v8::Local<v8::External>::Cast(
           mRef->GetInternalField(OREF_JSOBJSCRIPT_FIELD)
       );
   else
       wrapJSObj = v8::Local<v8::External>::Cast(
           v8::Handle<v8::Object>::Cast(mRef->GetPrototype())->GetInternalField(OREF_JSOBJSCRIPT_FIELD)
       );
   void* ptr = wrapJSObj->Value();
   caller = static_cast<JSObjectScript*>(ptr);


   //grabs the oref
    v8::Local<v8::External> wrapORef;
    if (mRef->InternalFieldCount() > 0)
        wrapORef = v8::Local<v8::External>::Cast(
            mRef->GetInternalField(OREF_OREF_FIELD)
        );
    else
        wrapORef = v8::Local<v8::External>::Cast(
            v8::Handle<v8::Object>::Cast(mRef->GetPrototype())->GetInternalField(OREF_OREF_FIELD)
        );
    void* ptr2 = wrapORef->Value();
    oref = static_cast<ObjectReference*>(ptr2);
}


v8::Handle<v8::Value> __orefSendMessage (const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to sendMessage(msg)")) );

    //first need to extract out the sending jsobjectscript and oref
 
    JSObjectScript* caller;
    ObjectReference* oref;
    readORef(args,caller,oref);

        
    //then need to read the message and print it
    v8::String::Utf8Value msgBodyArgs(args[0]);
    const char* cMsgBody = ToCString(msgBodyArgs);
    std::string cStrMsgBody(cMsgBody);

    caller->sendMessageToEntity(oref,cStrMsgBody);
    
    return v8::Undefined();
}



v8::Handle<v8::Value> __DebugRef(const v8::Arguments& args)
{
    std::cout<<"\n\n\n\nDEBUG: WAKA WAKA\n\n";
    return v8::Undefined();
}



/** Invokes a callback after a specified timeout.
 *  Arguments:
 *   float timeout: duration of timeout in seconds
 *   object target: target of callback (this pointer when invoked), or null for
 *                  the global (root) object
 *   function cb: callback to invoke, with no parameters
 */
v8::Handle<v8::Value> ScriptTimeout(const v8::Arguments& args) {
    if (args.Length() != 3)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to timeout().")) );

    v8::Handle<v8::Value> dur = args[0];
    v8::Handle<v8::Value> target_val = args[1];
    v8::Handle<v8::Value> cb_val = args[2];

    // Duration
    double native_dur = 0;
    if (dur->IsNumber())
        native_dur = dur->NumberValue();
    else if (dur->IsInt32())
        native_dur = dur->Int32Value();
    else
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Duration cannot be cast to float.")) );

    // Target
    if (!target_val->IsObject() && !target_val->IsNull() && !target_val->IsUndefined())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Target is not object or null.")) );

    v8::Handle<v8::Object> target = v8::Handle<v8::Object>::Cast(target_val);
    v8::Persistent<v8::Object> target_persist = v8::Persistent<v8::Object>::New(target);

    // Function
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to timeout().")) );
    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);

    JSObjectScript* target_script = GetTargetJSObjectScript(args);
    target_script->timeout(Duration::seconds(native_dur), target_persist, cb_persist);

    return v8::Undefined();
}


/** Imports a scripts and evaluates in the object's context.
 *  Arguments:
 *   string scriptname: filename of the script to import
 */
v8::Handle<v8::Value> ScriptImport(const v8::Arguments& args) {
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Import only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];

    StringCheckAndExtract(native_filename, filename);

    JSObjectScript* target_script = GetTargetJSObjectScript(args);
    target_script->import(native_filename);

    return v8::Undefined();
}

// Visual

v8::Handle<v8::Value> ScriptGetVisual(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getVisual();
}

void ScriptSetVisual(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setVisual(value);
}


// Scale

v8::Handle<v8::Value> ScriptGetScale(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getVisualScale();
}

void ScriptSetScale(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setVisualScale(value);
}



// Position

v8::Handle<v8::Value> ScriptGetPosition(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getPosition();
}

void ScriptSetPosition(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setPosition(value);
}


// Velocity

v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getVelocity();
}

void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setVelocity(value);
}


// Orientation

v8::Handle<v8::Value> ScriptGetOrientation(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getOrientation();
}

void ScriptSetOrientation(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setOrientation(value);
}


// AxisOfRotation

v8::Handle<v8::Value> ScriptGetAxisOfRotation(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getAxisOfRotation();
}

void ScriptSetAxisOfRotation(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setAxisOfRotation(value);
}


// AngularSpeed

v8::Handle<v8::Value> ScriptGetAngularSpeed(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getAngularSpeed();
}

void ScriptSetAngularSpeed(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setAngularSpeed(value);
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
v8::Handle<v8::Value> ScriptRegisterHandler(const v8::Arguments& args) {
    if (args.Length() != 3)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().")) );

    v8::Handle<v8::Value> pattern = args[0];
    v8::Handle<v8::Value> target_val = args[1];
    v8::Handle<v8::Value> cb_val = args[2];

    // Pattern
    PatternList native_patterns;
    if (PatternValidate(pattern)) {
        Pattern single_pattern = PatternExtract(pattern);
        native_patterns.push_back(single_pattern);
    }
    else if (pattern->IsArray()) {
        v8::Handle<v8::Array> pattern_array( v8::Handle<v8::Array>::Cast(pattern) );
        if (pattern_array->Length() == 0)
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Pattern array must contain at least one element.")) );
        for(uint32 pat_idx = 0; pat_idx < pattern_array->Length(); pat_idx++) {
            Local<Value> pattern_element = pattern_array->Get(pat_idx);
            if (!PatternValidate(pattern_element))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Found non-pattern element in array of patterns.")) );
            Pattern single_pattern = PatternExtract(pattern_element);
            native_patterns.push_back(single_pattern);
        }
    }
    else {
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Pattern argument must be pattern or array of patterns.")) );
    }

    // Target
    if (!target_val->IsObject() && !target_val->IsNull() && !target_val->IsUndefined())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Target is not object or null.")) );

    v8::Handle<v8::Object> target = v8::Handle<v8::Object>::Cast(target_val);
    v8::Persistent<v8::Object> target_persist = v8::Persistent<v8::Object>::New(target);

    // Function
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().")) );
    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);

    JSObjectScript* target_script = GetTargetJSObjectScript(args);
    target_script->registerHandler(native_patterns, target_persist, cb_persist);

    return v8::Undefined();
}




ObjectScriptManager* JSObjectScriptManager::createObjectScriptManager(const Sirikata::String& arguments) {
    return new JSObjectScriptManager(arguments);
}



JSObjectScriptManager::JSObjectScriptManager(const Sirikata::String& arguments)
{
    v8::HandleScope handle_scope;
    mGlobalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    // An internal field holds the JSObjectScript*
    mGlobalTemplate->SetInternalFieldCount(1);

    // And we expose some functionality directly
    v8::Handle<v8::ObjectTemplate> system_templ = v8::ObjectTemplate::New();
    // An internal field holds the JSObjectScript*

    system_templ->SetInternalFieldCount(1);

    // Functions / types
    system_templ->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(ScriptTimeout));
    system_templ->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
    system_templ->Set(v8::String::New("import"), v8::FunctionTemplate::New(ScriptImport));
    system_templ->Set(v8::String::New("__test"), v8::FunctionTemplate::New(__ScriptGetTest));

    system_templ->Set(v8::String::New("__broadcast"),v8::FunctionTemplate::New(__ScriptTestBroadcastMessage));

    //system_templ->Set(v8::String::New("__addressable"),v8::FunctionTemplate::New(__ScriptAddressable));
    system_templ->Set(v8::String::New("__sendMessageTo"),v8::FunctionTemplate::New(__SendMessageTo));

    
    //these are mutable fields
    system_templ->SetAccessor(JS_STRING(visual), ScriptGetVisual, ScriptSetVisual);
    system_templ->SetAccessor(JS_STRING(scale), ScriptGetScale, ScriptSetScale);

    system_templ->SetAccessor(JS_STRING(position), ScriptGetPosition, ScriptSetPosition);
    system_templ->SetAccessor(JS_STRING(velocity), ScriptGetVelocity, ScriptSetVelocity);
    system_templ->SetAccessor(JS_STRING(orientation), ScriptGetOrientation, ScriptSetOrientation);
    system_templ->SetAccessor(JS_STRING(angularAxis), ScriptGetAxisOfRotation, ScriptSetAxisOfRotation);
    system_templ->SetAccessor(JS_STRING(angularVelocity), ScriptGetAngularSpeed, ScriptSetAngularSpeed);


    mVec3Template = v8::Persistent<v8::FunctionTemplate>::New(CreateVec3Template());
    system_templ->Set(v8::String::New("Vec3"), mVec3Template);

    mQuaternionTemplate = v8::Persistent<v8::FunctionTemplate>::New(CreateQuaternionTemplate());
    system_templ->Set(v8::String::New("Quaternion"), mQuaternionTemplate);

    mPatternTemplate = v8::Persistent<v8::FunctionTemplate>::New(CreatePatternTemplate());
    system_templ->Set(JS_STRING(Pattern), mPatternTemplate);

    
    system_templ->Set(JS_STRING(registerHandler), v8::FunctionTemplate::New(ScriptRegisterHandler));

    mGlobalTemplate->Set(v8::String::New("system"), system_templ);

    //take care of oref templ
    bftm_createObjRefTemplate();

}

//creating the object reference template
void JSObjectScriptManager::bftm_createObjRefTemplate()
{
    v8::HandleScope handle_scope;
    mORefTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    // An internal field holds the external address of the addressable object
    mORefTemplate->SetInternalFieldCount(2);
    mORefTemplate->Set(v8::String::New("__debugRef"),v8::FunctionTemplate::New(__DebugRef));
    mORefTemplate->Set(v8::String::New("sendMessage"),v8::FunctionTemplate::New(__orefSendMessage));
}


        
JSObjectScriptManager::~JSObjectScriptManager() {
}

ObjectScript *JSObjectScriptManager::createObjectScript(HostedObjectPtr ho,
                                                            const Arguments& args) {
    JSObjectScript* new_script = new JSObjectScript(ho, args, mGlobalTemplate,mORefTemplate);
    if (!new_script->valid()) {
        delete new_script;
        return NULL;
    }
    return new_script;
}

void JSObjectScriptManager::destroyObjectScript(ObjectScript*toDestroy){
    delete toDestroy;
}

} // namespace JS
} // namespace JS
