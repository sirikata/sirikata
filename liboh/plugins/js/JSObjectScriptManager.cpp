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

#include "oh/Platform.hpp"

#include "JSObjectScriptManager.hpp"
#include "JSObjectScript.hpp"

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

JSObjectScript* GetTargetJSObjectScript(const v8::Arguments& args) {
    v8::Local<v8::Object> self = args.Holder();
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
    system_templ->Set(v8::String::New("timeout"), v8::FunctionTemplate::New(ScriptTimeout));
    system_templ->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
    system_templ->Set(v8::String::New("__test"), v8::FunctionTemplate::New(__ScriptGetTest));
    system_templ->SetInternalFieldCount(1);

    mGlobalTemplate->Set(v8::String::New("system"), system_templ);

}

JSObjectScriptManager::~JSObjectScriptManager() {
}

ObjectScript *JSObjectScriptManager::createObjectScript(HostedObjectPtr ho,
                                                            const Arguments& args) {
    JSObjectScript* new_script = new JSObjectScript(ho, args, mGlobalTemplate);
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
