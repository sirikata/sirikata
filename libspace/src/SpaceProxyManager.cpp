#include "space/SpaceProxyManager.hpp"
#include "space/Space.hpp"
namespace Sirikata {
SpaceProxyManager::SpaceProxyManager(Space::Space*space, Network::IOService*io):mQueryTracker(io),mSpace(space){
    
}
SpaceProxyManager::~SpaceProxyManager() {
    
}

void SpaceProxyManager::createObject(const ProxyObjectPtr &obj){

}
void SpaceProxyManager::destroyObject(const ProxyObjectPtr &obj){

}
void SpaceProxyManager::initialize() {

}
void SpaceProxyManager::destroy() {
  for (ProxyMap::iterator iter = mProxyMap.begin();
         iter != mProxyMap.end();
         ++iter) {
        iter->second->destroy(mSpace->now());
    }
    mProxyMap.clear();
}
ProxyObjectPtr SpaceProxyManager::getProxyObject(const Sirikata::SpaceObjectReference&id) const{
    ProxyMap::const_iterator where=mProxyMap.find(id.object());
    if (where!=mProxyMap.end()) {
        if(id.space()!=mSpace->id()){
            return where->second;
        }
    }
    return ProxyObjectPtr();
}
QueryTracker * SpaceProxyManager::getQueryTracker(const SpaceObjectReference&id) {
    return &mQueryTracker;
}
}
