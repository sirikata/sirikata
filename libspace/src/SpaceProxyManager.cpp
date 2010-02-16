#include "space/SpaceProxyManager.hpp"
#include "space/Space.hpp"
#include "proxyobject/ProxyCameraObject.hpp"
namespace Sirikata {
SpaceProxyManager::SpaceProxyManager(Space::Space*space, Network::IOService*io):mQueryTracker(io),mSpace(space){
}
SpaceProxyManager::~SpaceProxyManager() {

}

void SpaceProxyManager::createObject(const ProxyObjectPtr &newObj, QueryTracker*tracker){
    if (tracker!=NULL&&tracker!=&mQueryTracker) {
        SILOG(space,error,"Trying to add interest to a new object with an incorrect QueryTracker");
        return;
    }
    ProxyMap::iterator iter = mProxyMap.find(newObj->getObjectReference().object());
    if (iter == mProxyMap.end()) {
        mProxyMap[newObj->getObjectReference().object()]=newObj;
        notify(&ProxyCreationListener::onCreateProxy,newObj);
        ProxyCameraObject* cam = dynamic_cast<ProxyCameraObject*>(&*newObj);
        if (cam) {
            cam->attach("",0,0);
        }
    }
}
void SpaceProxyManager::destroyObject(const ProxyObjectPtr &obj, QueryTracker*tracker){
    if (tracker!=NULL&&tracker!=&mQueryTracker) {
        SILOG(space,error,"Trying to add interest to a new object with an incorrect QueryTracker");
        return;
    }
    if (mSpace->id()==obj->getObjectReference().space()) {
        ProxyMap::iterator where=mProxyMap.find(obj->getObjectReference().object());
        if (where!=mProxyMap.end()) {
            obj->destroy(mOffsetManager.now(*obj));
            notify(&ProxyCreationListener::onDestroyProxy,obj);
            mProxyMap.erase(where);
        }else {
            SILOG(space,error,"Trying to remove query interest in object "<<obj->getObjectReference()<<" from space when it is not in Proxy Map");
        }
    }else {
        SILOG(space,error,"Trying to remove query interest in space "<<obj->getObjectReference().space()<<" from space ID "<<mSpace->id());
    }

}
void SpaceProxyManager::addQueryInterest(uint32 query_id, const SpaceObjectReference&id) {
    mQueryMap[id.object()].insert(query_id);
}
void SpaceProxyManager::removeQueryInterest(uint32 query_id, const ProxyObjectPtr& obj, const SpaceObjectReference&id) {
    std::tr1::unordered_map<ObjectReference,std::set<uint32> >::iterator where=mQueryMap.find(id.object());
    if (where!=mQueryMap.end()) {
        if (where->second.find(query_id)!=where->second.end()) {
            where->second.erase(query_id);
            if (where->second.empty()) {
                mQueryMap.erase(where);
                destroyObject(obj,&mQueryTracker);
            }
        }else {
            SILOG(space,error,"Trying to erase query "<<query_id<<" for object "<<id<<" query_id invalid");
        }
    }else {
        SILOG(space,error,"Trying to erase query for object "<<id<<" not in query set");
    }
}
void SpaceProxyManager::clearQuery(uint32 query_id){
    std::vector<ObjectReference>mfd;
    for (std::tr1::unordered_map<ObjectReference,std::set<uint32> >::iterator i=mQueryMap.begin(),
             ie=mQueryMap.end();
         i!=ie;
         ++i) {
        if (i->second.find(query_id)!=i->second.end()) {
            mfd.push_back(i->first);
        }
    }
    {
        for (std::vector<ObjectReference>::iterator i=mfd.begin(),ie=mfd.end();i!=ie;++i) {
            SpaceObjectReference id(mSpace->id(),*i);
            removeQueryInterest(query_id,getProxyObject(id),id);
        }
    }
}
bool SpaceProxyManager::isLocal(const SpaceObjectReference&id) const{
    return false;
}
ProxyManager* SpaceProxyManager::getProxyManager(const SpaceID&space) {
    if (mSpace->id()==space) return this;
    return NULL;
}
QueryTracker* SpaceProxyManager::getTracker() {
    return &mQueryTracker;
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
        if(id.space()==mSpace->id()){
            return where->second;
        }
    }
    return ProxyObjectPtr();
}
QueryTracker * SpaceProxyManager::getQueryTracker(const SpaceObjectReference&id) {
    return &mQueryTracker;
}


// ODP::Service Interface

ODP::Port* SpaceProxyManager::bindODPPort(SpaceID space, ODP::PortID port) {
    NOT_IMPLEMENTED(SpaceProxyManager);
    return NULL;
}

ODP::Port* SpaceProxyManager::bindODPPort(SpaceID space) {
    NOT_IMPLEMENTED(SpaceProxyManager);
    return NULL;
}

void SpaceProxyManager::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    NOT_IMPLEMENTED(SpaceProxyManager);
}

}
