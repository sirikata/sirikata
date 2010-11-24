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

#include <sirikata/oh/Platform.hpp>
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

namespace {
MonoVWObjectScript::ArgumentMap ParseArguments(const String& value) {
    MonoVWObjectScript::ArgumentMap retval;

    // Strip any whitespace around the list
    int32 list_start = 0, list_end = value.size();
    while(list_start < value.size()) {
        if (value[list_start] == ' ' || value[list_start] == '\t')
            list_start++;
        else
            break;
    }
    while(list_end >= 0) {
        if (value[list_end-1] == ' ' || value[list_end-1] == '\t')
            list_end--;
        else
            break;
    }

    if (list_end - list_start <= 0) return retval;

    int32 space = list_start, last_space = list_start-1;

    while(true) {
        space = (int32)value.find(' ', last_space+1);
        if (space > list_end || space == std::string::npos)
            space = list_end;
        std::string elem = value.substr(last_space+1, (space-(last_space+1)));
        if (elem.size() > 0) {
            // elem should be of the form --option=value
            assert(elem[0] == '-' && elem[1] == '-');
            elem = elem.substr(2);
            int32 equals = value.find('=');

            std::string opt_name = elem.substr(0, equals);
            std::string opt_val = elem.substr(equals+1);

            retval[opt_name] = opt_val;
        }

        // If we hit the end of the string, finish up
        if (space >= list_end)
            break;

        last_space = space;
    }
    return retval;
}
}

ObjectScript *MonoVWObjectScriptManager::createObjectScript(HostedObjectPtr ho, const String& args_str) {
    MonoVWObjectScript::ArgumentMap args = ParseArguments(args_str);
    // Fill in full argument list by script type
    MonoVWObjectScript::ArgumentMap full_args = args;
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
