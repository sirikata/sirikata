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
