#include "JSSystem.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"


namespace Sirikata{
namespace JS{
namespace JSSystem{




v8::Handle<v8::Value> ScriptReboot(const v8::Arguments& args)
{
  if(args.Length() != 0)
  {
     return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to reboot()")) );
  }
   // get the c++ object out from the js object
   JSObjectScript* target_script = GetTargetJSObjectScript(args);
   // invoke the reboot in the script
   target_script->reboot();

   return v8::Undefined();
}

v8::Handle<v8::Value> ScriptTimeout(const v8::Arguments& args)
{
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


v8::Handle<v8::Value> ScriptImport(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Import only takes one parameter: the name of the file to import.")) );

    v8::Handle<v8::Value> filename = args[0];

    StringCheckAndExtract(native_filename, filename);

    JSObjectScript* target_script = GetTargetJSObjectScript(args);
    target_script->import(native_filename);

    return v8::Undefined();
}


v8::Handle<v8::Value> __ScriptGetTest(const v8::Arguments& args)
{

    JSObjectScript* target = GetTargetJSObjectScript(args);
    

    target->test();
    return v8::Undefined();
}



// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args)
{
    //std::cout<<"\n\n\nDEBUG PRINT\n\n";
    
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




v8::Handle<v8::Value> __ScriptTestBroadcastMessage(const v8::Arguments& args)
{
    //check args to make sure they're okay.
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to broadcast(<object>)")) );

    v8::Handle<v8::Value> messageBody = args[0];

    if(!messageBody->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Message should be an object")) );


    //serialize the object to send
    Local<v8::Object> v8Object = messageBody->ToObject();
    std::string serialized_message = JSSerializer::serializeObject(v8Object);

    
    //sender
    JSObjectScript* target = GetTargetJSObjectScript(args);
    target->bftm_testSendMessageBroadcast(serialized_message);

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

v8::Handle<v8::Value> ScriptGetScale(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getVisualScale();
}

void ScriptSetScale(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setVisualScale(value);
}



// Position

v8::Handle<v8::Value> ScriptGetPosition(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getPosition();
}

void ScriptSetPosition(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setPosition(value);
}


// Velocity

v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getVelocity();
}

void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setVelocity(value);
}


// Orientation

v8::Handle<v8::Value> ScriptGetOrientation(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getOrientation();
}

void ScriptSetOrientation(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setOrientation(value);
}


// AxisOfRotation

v8::Handle<v8::Value> ScriptGetAxisOfRotation(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getAxisOfRotation();
}

void ScriptSetAxisOfRotation(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    target_script->setAxisOfRotation(value);
}


// AngularSpeed

v8::Handle<v8::Value> ScriptGetAngularSpeed(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    JSObjectScript* target_script = GetTargetJSObjectScript(info);
    return target_script->getAngularSpeed();
}

void ScriptSetAngularSpeed(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
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
//v8::Handle<v8::Object> ScriptRegisterHandler(const v8::Arguments& args)
v8::Handle<v8::Value> ScriptRegisterHandler(const v8::Arguments& args)
{
    if (args.Length() != 4)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().")) );
    

    v8::Handle<v8::Value> pattern = args[0];
    v8::Handle<v8::Value> target_val = args[1];
    v8::Handle<v8::Value> cb_val = args[2];
	v8::Handle<v8::Value> sender_val = args[3];

    // Pattern
    PatternList native_patterns;
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
    else
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Pattern argument must be pattern or array of patterns.")) );

    
    // Target
    if (!target_val->IsObject() && !target_val->IsNull() && !target_val->IsUndefined())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Target is not object or null.")) );

    
   // Sender
    if (!sender_val->IsObject() && !sender_val->IsNull() && !sender_val->IsUndefined())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Sender is not object or null.")) );


    v8::Handle<v8::Object> target = v8::Handle<v8::Object>::Cast(target_val);
    v8::Persistent<v8::Object> target_persist = v8::Persistent<v8::Object>::New(target);

    v8::Handle<v8::Object> sender = v8::Handle<v8::Object>::Cast(sender_val);
    v8::Persistent<v8::Object> sender_persist = v8::Persistent<v8::Object>::New(sender);




    // Function
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to registerHandler().  Must contain callback function.")) );

    
    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);
    v8::Persistent<v8::Function> cb_persist = v8::Persistent<v8::Function>::New(cb);

    JSObjectScript* target_script = GetTargetJSObjectScript(args);

    JSEventHandler* evHand = target_script->registerHandler(native_patterns, target_persist, cb_persist, sender_persist);


    /*
      FIXME: Target script is causing a bug here.
     */
    
    //target_script->makeEventHandlerObject(evHand);
//    return target_script->makeEventHandlerObject(evHand);
    return v8::Undefined();

}



}}}
