#include "oh/Platform.hpp"
#include "MonoVWObjectScriptManager.hpp"
#include "MonoVWObjectScript.hpp"
namespace Sirikata {
MonoVWObjectScriptManager::MonoVWObjectScriptManager(Mono::MonoSystem*system, const Sirikata::String&arguments){
    mSystem=system;
    
}
ObjectScript *MonoVWObjectScriptManager::createObjectScript(HostedObject* ho,
                                                            const Arguments &args){
    return new MonoVWObjectScript(mSystem,ho,args);
}
void MonoVWObjectScriptManager::destroyObjectScript(ObjectScript*toDestroy){
    delete toDestroy;
}
MonoVWObjectScriptManager::~MonoVWObjectScriptManager(){
    
}

}
