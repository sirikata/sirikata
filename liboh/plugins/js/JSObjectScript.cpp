/*  Sirikata
 *  JSObjectScript.cpp
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

#include <oh/Platform.hpp>

#include "JS_Sirikata.pbj.hpp"

#include <util/RoutableMessageHeader.hpp>
#include <util/RoutableMessageBody.hpp>
#include <util/KnownServices.hpp>

#include "JSObjectScript.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSLogging.hpp"

#include "JSUtil.hpp"
#include "JSVec3.hpp"
#include "JSQuaternion.hpp"

#include <oh/ObjectHost.hpp>
#include <network/IOService.hpp>

using namespace v8;

namespace Sirikata {
namespace JS {

JSObjectScript::JSObjectScript(HostedObjectPtr ho, const ObjectScriptManager::Arguments& args, v8::Persistent<v8::ObjectTemplate>& global_template)
 : mParent(ho)
{
    v8::HandleScope handle_scope;
    mContext = Context::New(NULL, global_template);

    Local<Object> global_obj = mContext->Global();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
    global_proto->SetInternalField(0, External::New(this));
    // And we add an internal field to the system object as well to make it
    // easier to find the pointer in different calls. Note that in this case we
    // don't use the prototype -- non-global objects work as we would expect.
    Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New("system")));
    system_obj->SetInternalField(0, External::New(this));

    const HostedObject::SpaceSet& spaces = mParent->spaces();
    if (spaces.size() > 1)
        JSLOG(fatal,"Error: Connected to more than one space.  Only enabling scripting for one space.");

    for(HostedObject::SpaceSet::iterator space_it = spaces.begin(); space_it != spaces.end(); space_it++) {
        mScriptingPort = mParent->bindODPPort(*space_it, Services::SCRIPTING);
        if (mScriptingPort)
            mScriptingPort->receive( std::tr1::bind(&JSObjectScript::handleScriptingMessage, this, _1, _2) );
    }
}

JSObjectScript::~JSObjectScript() {
    if (mScriptingPort)
        delete mScriptingPort;

    mContext.Dispose();
}

bool JSObjectScript::forwardMessagesTo(MessageService*){
    NOT_IMPLEMENTED(js);
    return false;
}

bool JSObjectScript::endForwardingMessagesTo(MessageService*){
    NOT_IMPLEMENTED(js);
    return false;
}

bool JSObjectScript::processRPC(
    const RoutableMessageHeader &receivedHeader,
    const std::string& name,
    MemoryReference args,
    MemoryBuffer &returnValue)
{
    NOT_IMPLEMENTED(js);
    return false;
}

void JSObjectScript::processMessage(
    const RoutableMessageHeader& receivedHeader,
    MemoryReference body)
{
    NOT_IMPLEMENTED(js);
}

bool JSObjectScript::valid() const {
    return (mParent);
}

void JSObjectScript::test() const {
    const HostedObject::SpaceSet& spaces = mParent->spaces();
    assert(spaces.size() == 1);

    SpaceID space = *(spaces.begin());

    Location loc = mParent->getLocation( space );
    loc.setPosition( loc.getPosition() + Vector3<float64>(.5f, .5f, .5f) );
    loc.setOrientation( loc.getOrientation() * Quaternion(Vector3<float32>(0.0f, 0.0f, 1.0f), 3.14159/18.0) );
    loc.setAxisOfRotation( Vector3<float32>(0.0f, 0.0f, 1.0f) );
    loc.setAngularSpeed(3.14159/10.0);
    mParent->setLocation( space, loc );

    mParent->setVisual(space, Transfer::URI(" http://www.sirikata.com/content/assets/tetra.dae"));
    mParent->setVisualScale(space, Vector3f(1.f, 1.f, 2.f) );
}

void JSObjectScript::timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb) {
    // FIXME using the raw pointer isn't safe
    mParent->getObjectHost()->getSpaceIO()->post(
        dur,
        std::tr1::bind(
            &JSObjectScript::handleTimeout,
            this,
            target,
            cb
        )
    );
}

void JSObjectScript::handleTimeout(v8::Persistent<v8::Object> target, v8::Persistent<v8::Function> cb) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    TryCatch try_catch;

    const int argc = 0;
    Handle<Value> argv[argc] = { };

    Handle<Value> result;
    if (target->IsNull() || target->IsUndefined())
        result = cb->Call(mContext->Global(), argc, argv);
    else
        result = cb->Call(target, argc, argv);

    if (result.IsEmpty()) {
        // FIXME what should we do with this exception?
        v8::String::Utf8Value error(try_catch.Exception());
    }
}

#define FIXME_GET_SPACE() \
    const HostedObject::SpaceSet& spaces = mParent->spaces(); \
    assert(spaces.size() == 1);                               \
    SpaceID space = *(spaces.begin());

v8::Handle<v8::String> JSObjectScript::getVisual() {
    FIXME_GET_SPACE();

    String url_string = mParent->getVisual(space).toString();
    return v8::String::New( url_string.c_str(), url_string.size() );
}

void JSObjectScript::setVisual(v8::Local<v8::Value>& newvis) {
    // Can/should we do anything about a failure here?
    if (!newvis->IsString())
        return;

    v8::String::Utf8Value newvis_str(newvis);
    if (! *newvis_str)
        return; // FIXME failure?
    Transfer::URI vis_uri(*newvis_str);

    FIXME_GET_SPACE();
    mParent->setVisual(space, vis_uri);
}

v8::Handle<v8::Value> JSObjectScript::getVisualScale() {
    FIXME_GET_SPACE();
    return CreateJSResult(mContext, mParent->getVisualScale(space));
}

void JSObjectScript::setVisualScale(v8::Local<v8::Value>& newscale) {
    Handle<Object> scale_obj = ObjectCast(newscale);
    if (!Vec3Validate(scale_obj))
        return;

    Vector3f native_scale(Vec3Extract(scale_obj));
    FIXME_GET_SPACE();
    mParent->setVisualScale(space, native_scale);
}

#define CreateLocationAccessorHandlers(PropType, PropName, SubType, SubTypeCast, Validator, Extractor) \
    v8::Handle<v8::Value> JSObjectScript::get##PropName() {             \
        FIXME_GET_SPACE();                                              \
        return CreateJSResult(mContext, mParent->getLocation(space).get##PropName()); \
    }                                                                   \
                                                                        \
    void JSObjectScript::set##PropName(v8::Local<v8::Value>& newval) {  \
        Handle<SubType> val_obj = SubTypeCast(newval);                  \
        if (!Validator(val_obj))                                        \
            return;                                                     \
                                                                        \
        PropType native_val(Extractor(val_obj));                        \
                                                                        \
        FIXME_GET_SPACE();                                              \
        Location loc = mParent->getLocation(space);                     \
        loc.set##PropName(native_val);                                  \
        mParent->setLocation(space, loc);                               \
    }

#define NOOP_CAST(X) X

CreateLocationAccessorHandlers(Vector3d, Position, Object, ObjectCast, Vec3Validate, Vec3Extract)
CreateLocationAccessorHandlers(Vector3f, Velocity, Object, ObjectCast, Vec3Validate, Vec3Extract)
CreateLocationAccessorHandlers(Quaternion, Orientation, Object, ObjectCast, QuaternionValidate, QuaternionExtract)
CreateLocationAccessorHandlers(Vector3f, AxisOfRotation, Object, ObjectCast, Vec3Validate, Vec3Extract)
CreateLocationAccessorHandlers(double, AngularSpeed, Value, NOOP_CAST, NumericValidate, NumericExtract)

void JSObjectScript::handleScriptingMessage(const RoutableMessageHeader& hdr, MemoryReference payload) {
    // Parse (FIXME we have to parse a RoutableMessageBody here, should just be
    // in "Header")
    RoutableMessageBody body;
    body.ParseFromArray(payload.data(), payload.size());
    Protocol::ScriptingMessage scripting_msg;
    bool parsed = scripting_msg.ParseFromString(body.payload());
    if (!parsed) {
        JSLOG(fatal, "Parsing failed.");
        return;
    }

    // Start scope
    Context::Scope context_scope(mContext);

    // Handle all requests
    for(int32 ii = 0; ii < scripting_msg.requests_size(); ii++) {
        Protocol::ScriptingRequest req = scripting_msg.requests(ii);
        String script_str = req.body();

        v8::HandleScope handle_scope;

        TryCatch try_catch;

        v8::Handle<v8::String> source = v8::String::New(script_str.c_str(), script_str.size());

        // Compile
        v8::Handle<v8::Script> script = v8::Script::Compile(source);
        if (script.IsEmpty()) {
            v8::String::Utf8Value error(try_catch.Exception());
            JSLOG(error, "Compile error: " << *error);
            continue;
        }

        // Execute
        v8::Handle<v8::Value> result = script->Run();
        if (result.IsEmpty()) {
            v8::String::Utf8Value error(try_catch.Exception());
            JSLOG(error, "Uncaught exception: " << *error);
            continue;
        }

        if (!result->IsUndefined()) {
            v8::String::AsciiValue ascii(result);
            JSLOG(info, "Script result: " << *ascii);
        }
    }
}

} // namespace JS
} // namespace Sirikata
