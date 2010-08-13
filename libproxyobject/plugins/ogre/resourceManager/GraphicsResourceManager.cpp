/*  Meru
 *  GraphicsResourceManager.cpp
 *
 *  Copyright (c) 2009, Stanford University
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
#include "../meruCompat/DependencyManager.hpp"
#include "GraphicsResourceManager.hpp"
#include "GraphicsResourceMaterial.hpp"
#include "GraphicsResourceMesh.hpp"
#include "GraphicsResourceModel.hpp"
#include "GraphicsResourceName.hpp"
#include "GraphicsResourceEntity.hpp"
#include "GraphicsResourceSkeleton.hpp"
#include "GraphicsResourceShader.hpp"
#include "GraphicsResourceTexture.hpp"
#include "ResourceManager.hpp"
#include "../meruCompat/Event.hpp"
#include "../meruCompat/EventSource.hpp"
#include <sirikata/core/util/Thread.hpp>
#include <stdio.h>

using namespace std;

using namespace Sirikata;
using namespace Sirikata::Transfer;

using std::map;
using std::set;
using std::pair;
using std::vector;

namespace Meru {

MANUAL_SINGLETON_STORAGE(GraphicsResourceManager);

OptionValue*OPTION_VIDEO_MEMORY_RESOURCE_CACHE_SIZE = new OptionValue("video-memory-cache-size","1024",OptionValueType<int>(),"Number of megabytes to store from CDN in video memory");

InitializeGlobalOptions graphicsresourcemanageropts("ogregraphics",
    OPTION_VIDEO_MEMORY_RESOURCE_CACHE_SIZE,
    NULL);


void GraphicsResourceManager::initializeMediator() {
  mTransferMediator = &(TransferMediator::getSingleton());

  mTransferPool = mTransferMediator->registerClient("OgreGraphics");
}

TransferPoolPtr GraphicsResourceManager::transferPool()
{
    return mTransferPool;
}

GraphicsResourceManager::GraphicsResourceManager(Sirikata::Task::WorkQueue *dependencyQueue)
: mEpoch(0), mEnabled(true)
{
  initializeMediator();

  this->mTickListener = EventSource::getSingleton().subscribeId(
    EventID(EventTypes::Tick),
      EVENT_CALLBACK(GraphicsResourceManager, tick, this)
  );

  mDependencyManager = new DependencyManager(dependencyQueue);
  mBudget = OPTION_VIDEO_MEMORY_RESOURCE_CACHE_SIZE->as<int>() * 1024 * 1024;
}

GraphicsResourceManager::~GraphicsResourceManager()
{
  EventSource::getSingleton().unsubscribe(this->mTickListener,false);

  delete mDependencyManager;
}

WeakResourcePtr GraphicsResourceManager::getResource(const String &id)
{
  std::map<String, WeakResourcePtr>::iterator itr;
  itr = mIDResourceMap.find(id);
  if (itr == mIDResourceMap.end())
    return WeakResourcePtr();
  return itr->second;
}

SharedResourcePtr GraphicsResourceManager::getResourceEntity(const SpaceObjectReference &id, GraphicsEntity *graphicsEntity)
{
  WeakResourcePtr curWeakPtr = getResource(id.toString());
  SharedResourcePtr curSharedPtr = curWeakPtr.lock();
  if (curSharedPtr)
    return curSharedPtr;
  else {
    curSharedPtr = GraphicsResource::construct<GraphicsResourceEntity>(id, graphicsEntity);
    mIDResourceMap[id.toString()] = curSharedPtr;
    mEntities.insert(curSharedPtr.get());

    return curSharedPtr;
  }
}

SharedResourcePtr GraphicsResourceManager::getResourceAssetByHash(const ResourceHash &resourceId, GraphicsResource::Type resourceType)
{
  bool isMHash = true;
  WeakResourcePtr curWeakPtr = getResource(resourceId.fingerprint().convertToHexString());
  SharedResourcePtr curSharedPtr = curWeakPtr.lock();
  /*
  if (!curSharedPtr) {
    curWeakPtr = getResource(id.uri().toString());
    curSharedPtr = curWeakPtr.lock();
  }
  */
  if (curSharedPtr) {
    return curSharedPtr;
  }
  if (resourceType == GraphicsResource::MESH) {
    curSharedPtr = GraphicsResource::construct<GraphicsResourceMesh>(resourceId);
  }
  else if (resourceType == GraphicsResource::MODEL) {
    curSharedPtr = GraphicsResource::construct<GraphicsResourceModel>(resourceId);
  }
  else if (resourceType == GraphicsResource::MATERIAL) {
    curSharedPtr = GraphicsResource::construct<GraphicsResourceMaterial>(resourceId);
  }
  else if (resourceType == GraphicsResource::TEXTURE) {
    curSharedPtr = GraphicsResource::construct<GraphicsResourceTexture>(resourceId);
  }
  else if (resourceType == GraphicsResource::SHADER) {
    curSharedPtr = GraphicsResource::construct<GraphicsResourceShader>(resourceId);
  }
  else {
    assert(false);
  }

  mIDResourceMap[curSharedPtr->getID()] = curSharedPtr;
  mResources.insert(curSharedPtr.get());

  return curSharedPtr;
}
SharedResourcePtr GraphicsResourceManager::getResourceAsset(const URI &id, GraphicsResource::Type resourceType)
{
  bool isMHash = false;
  WeakResourcePtr curWeakPtr = getResource(id.toString());
  SharedResourcePtr curSharedPtr = curWeakPtr.lock();

  if (curSharedPtr) {
    return curSharedPtr;
  }
  curSharedPtr = GraphicsResource::construct<GraphicsResourceName>(id, resourceType);

  mIDResourceMap[curSharedPtr->getID()] = curSharedPtr;
  mResources.insert(curSharedPtr.get());

  return curSharedPtr;
}
/*DependencyTask * DEBUGME(std::set<DependencyTask*,DependencyTask::DependencyTaskLessThanFunctor>&x) {
    return *x.begin();
}*/
SharedResourcePtr DEBUGME(std::set<SharedResourcePtr>&x) {
    return *x.begin();
}
void GraphicsResourceManager::computeLoadedSet()
{
//  MERU_BENCH("computeLoadedSet");
  assert(mQueue.empty());

  if (!mEnabled)
    return;

  mEpoch++;
  mToUnload = mResources;

  set<GraphicsResource *>::iterator itr;
  for (itr = mEntities.begin(); itr != mEntities.end(); itr++) {
    GraphicsResource* resource = *itr;

    GraphicsResource::ParseState parseState = resource->getParseState();

    if (parseState == GraphicsResource::PARSE_INVALID) {
      resource->parse();
    }
    resource->updateValue(mEpoch);
  }

  float budgetUsed = 0;
  while (mQueue.size() > 0) {
    GraphicsResource* resource = mQueue.begin()->second;
    mQueue.erase(mQueue.begin());

    float cost = resource->cost();
    //assert(cost >= 0);
    ////////////////SILOG(resource,error,"Cost " << resource->cost() << " for "<<resource->getID()<<" is less than 0.");

    if (budgetUsed + cost <= mBudget) {
      GraphicsResource::LoadState loadState = resource->getLoadState();
      if (loadState != GraphicsResource::LOAD_LOADING
       && loadState != GraphicsResource::LOAD_LOADED
       && loadState != GraphicsResource::LOAD_WAITING_LOAD) {
        resource->load(mEpoch);
      }
      resource->removeLoadDependencies(mEpoch);
      budgetUsed += cost;
    }
  }

  set<GraphicsResource *>::iterator vitr;
  for (vitr = mToUnload.begin(); vitr != mToUnload.end(); vitr++) {
    GraphicsResource::LoadState loadState = (*vitr)->getLoadState();
    if (loadState != GraphicsResource::LOAD_UNLOADED
     && loadState != GraphicsResource::LOAD_UNLOADING
     && loadState != GraphicsResource::LOAD_NEW
     && loadState != GraphicsResource::LOAD_WAITING_UNLOAD)
      (*vitr)->unload(mEpoch);
  }
  mToUnload.clear();

//  MERU_BENCH("ecomputeLoadedSet");
}

void GraphicsResourceManager::updateLoadValue(GraphicsResource* resource, float oldValue)
{
  ResourcePriorityQueue::iterator itr;
  itr = mQueue.find(pair<float, GraphicsResource*>(oldValue, resource));
  if (itr != mQueue.end()) {
    mQueue.erase(itr);
    mQueue.insert(pair<float, GraphicsResource*>(resource->value(), resource));
  }
}

void GraphicsResourceManager::registerLoad(GraphicsResource* resource, float oldValue)
{
  ResourcePriorityQueue::iterator itr;
  itr = mQueue.find(pair<float, GraphicsResource*>(oldValue, resource));
  if (itr != mQueue.end()) {
    mQueue.erase(itr);
  }
  mQueue.insert(pair<float, GraphicsResource*>(resource->value(), resource));
}

void GraphicsResourceManager::unregisterLoad(GraphicsResource* resource)
{
  ResourcePriorityQueue::iterator itr;
  itr = mQueue.find(pair<float, GraphicsResource*>(resource->value(), resource));
  if (itr != mQueue.end()) {
    mQueue.erase(itr);
  }

  set<GraphicsResource*>::iterator uitr = mToUnload.find(resource);
  if (uitr != mToUnload.end())
    mToUnload.erase(uitr);
}

void GraphicsResourceManager::unregisterResource(GraphicsResource* resource)
{
  mIDResourceMap.erase(mIDResourceMap.find(resource->getID()));
  mResources.erase(resource);

  std::set<GraphicsResource *>::iterator eitr = mEntities.find(resource);
  if (eitr != mEntities.end())
    mEntities.erase(eitr);

  std::set<GraphicsResource *>::iterator qitr = mToUnload.find(resource);
  if (qitr != mToUnload.end())
    mToUnload.erase(qitr);
}

EventResponse GraphicsResourceManager::tick(const EventPtr &evtPtr)
{
  computeLoadedSet();
  return EventResponse::nop();
}

}
