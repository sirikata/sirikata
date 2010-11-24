/*  Sirikata
 *  JSObjectScriptManager.hpp
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

#ifndef _SIRIKATA_JS_OBJECT_SCRIPT_MANAGER_HPP_
#define _SIRIKATA_JS_OBJECT_SCRIPT_MANAGER_HPP_


#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>

#include <v8.h>

namespace Sirikata {
namespace JS {

class JSObjectScriptManager : public ObjectScriptManager {
public:
    static ObjectScriptManager* createObjectScriptManager(const Sirikata::String& arguments);

    JSObjectScriptManager(const Sirikata::String& arguments);
    virtual ~JSObjectScriptManager();

    virtual ObjectScript* createObjectScript(HostedObjectPtr ho, const String& args);
    virtual void destroyObjectScript(ObjectScript* toDestroy);

    OptionSet* getOptions() const { return mOptions; }

    v8::Persistent<v8::ObjectTemplate> mEntityTemplate;
    v8::Persistent<v8::ObjectTemplate> mHandlerTemplate;
    v8::Persistent<v8::ObjectTemplate> mGlobalTemplate;
    v8::Persistent<v8::ObjectTemplate> mAddressableTemplate;
    v8::Persistent<v8::ObjectTemplate> mPresenceTemplate;
    v8::Persistent<v8::ObjectTemplate> mContextTemplate;
    v8::Persistent<v8::ObjectTemplate> mMathTemplate;
    void testPrint();

private:

    void createAddressableTemplate();
    void createSystemTemplate();
    void createHandlerTemplate();
    void createPresenceTemplate();
    void createContextTemplate();
    void createMathTemplate();

    // The manager tracks the templates so they can be reused by all the
    // individual scripts.
    v8::Persistent<v8::FunctionTemplate> mVec3Template;
    v8::Persistent<v8::FunctionTemplate> mQuaternionTemplate;
    v8::Persistent<v8::FunctionTemplate> mPatternTemplate;

    OptionSet* mOptions;
};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_MANAGER_HPP_
