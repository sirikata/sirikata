/*  Sirikata liboh -- Object Host
 *  MonoVWObjectScriptManager.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include "MonoVWObjectScriptManager.hpp"
#include "MonoVWObjectScript.hpp"
namespace Sirikata {

ObjectScriptManager* MonoVWObjectScriptManager::createObjectScriptManager(Mono::MonoSystem *monosystem, const Sirikata::String& arguments, MonoScriptType script_type) {
    return new MonoVWObjectScriptManager(monosystem, arguments, script_type);
}

MonoVWObjectScriptManager::MonoVWObjectScriptManager(Mono::MonoSystem* system, const Sirikata::String& arguments, MonoScriptType script_type)
 : mSystem(system),
   mScriptType(script_type)
{
}

ObjectScript *MonoVWObjectScriptManager::createObjectScript(HostedObjectPtr ho,
                                                            const Arguments &args) {
    // Fill in full argument list by script type
    Arguments full_args = args;
    if (mScriptType == IronPythonScript) {
        if (full_args.find("Assembly") != full_args.end())
            SILOG(mono,error,"[MONO] Overwriting Assembly argument to create IronPython object script.");
        if (full_args.find("Namespace") != full_args.end())
            SILOG(mono,error,"[MONO] Overwriting Namespace argument to create IronPython object script.");
        if (full_args.find("Class") != full_args.end())
            SILOG(mono,error,"[MONO] Overwriting Class argument to create IronPython object script.");

        full_args["Assembly"] = "Sirikata.Runtime";
        full_args["Namespace"] = "Sirikata.Runtime";
        full_args["Class"] = "PythonObject";
    }

    MonoVWObjectScript* new_script = new MonoVWObjectScript(mSystem, ho, full_args);
    if (!new_script->valid()) {
        delete new_script;
        return NULL;
    }
    return new_script;
}
void MonoVWObjectScriptManager::destroyObjectScript(ObjectScript*toDestroy){
    delete toDestroy;
}

MonoVWObjectScriptManager::~MonoVWObjectScriptManager() {
}

}
