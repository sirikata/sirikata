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

using namespace v8;

namespace Sirikata {
namespace JS {

JSObjectScript::JSObjectScript(HostedObjectPtr ho, const ObjectScriptManager::Arguments& args)
 : mParent(ho),
   mContext(Context::New())
{
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

        v8::Handle<v8::String> source = v8::String::New(script_str.c_str(), script_str.size());
        v8::Handle<v8::Script> script = v8::Script::Compile(source);
        v8::Handle<v8::Value> result = script->Run();

        v8::String::AsciiValue ascii(result);
        JSLOG(info, "Script result: " << *ascii);
    }
}

} // namespace JS
} // namespace Sirikata
