#include "oh/Platform.hpp"
#include "MonoVWObjectScriptManager.hpp"
#include "MonoVWObjectScript.hpp"
namespace Sirikata {

ObjectScriptManager* MonoVWObjectScriptManager::createObjectScriptManager(Mono::MonoSystem *monosystem,const Sirikata::String&arguments) {
    return new MonoVWObjectScriptManager(monosystem,arguments);
}

MonoVWObjectScriptManager::MonoVWObjectScriptManager(Mono::MonoSystem*system, const Sirikata::String&arguments){
    mSystem=system;

}
ObjectScript *MonoVWObjectScriptManager::createObjectScript(HostedObjectPtr ho,
                                                            const Arguments &args){
    MonoVWObjectScript* new_script = new MonoVWObjectScript(mSystem,ho,args);
    if (!new_script->valid()) {
        delete new_script;
        return NULL;
    }
    return new_script;
}
void MonoVWObjectScriptManager::destroyObjectScript(ObjectScript*toDestroy){
    delete toDestroy;
}
MonoVWObjectScriptManager::~MonoVWObjectScriptManager(){

}

}
