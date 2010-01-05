/*  Meru
 *  GraphicsResourceModel.cpp
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
#include "CDNArchive.hpp"
#include "GraphicsResourceManager.hpp"
#include "GraphicsResourceModel.hpp"
#include "ResourceDependencyTask.hpp"
#include "ResourceLoadTask.hpp"
#include "ResourceLoadingQueue.hpp"
#include "ResourceUnloadTask.hpp"
#include "SequentialWorkQueue.hpp"
#include <boost/bind.hpp>
#include <OgreResourceBackgroundQueue.h>

namespace Meru {

/// this is an option in GraphicsResourceMesh
#define OPTION_ENABLE_TEXTURES true

class ModelDependencyTask : public ResourceDependencyTask
{
public:
  ModelDependencyTask(DependencyManager* mgr, WeakResourcePtr resource, const String& hash);
  virtual ~ModelDependencyTask();

  virtual void operator()();
};

class ModelLoadTask : public ResourceLoadTask
{
public:
  ModelLoadTask(DependencyManager *mgr, SharedResourcePtr resource, const SHA256 &hash, unsigned int epoch);

  virtual void doRun();
};

class ModelUnloadTask : public ResourceUnloadTask
{
public:
  ModelUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const SHA256 &hash, unsigned int epoch);

  virtual void doRun();

protected:
  //bool mainThreadUnload(String name);
};

GraphicsResourceModel::GraphicsResourceModel(const RemoteFileId &resourceID)
: GraphicsResourceAsset(resourceID, GraphicsResource::MODEL)
{

}

GraphicsResourceModel::~GraphicsResourceModel()
{
  if (mLoadState == LOAD_LOADED)
    doUnload();
}

void GraphicsResourceModel::resolveName(const URI& id, const ResourceHash& hash)
{
  mMaterialNames[id.toString()] = hash.fingerprint().convertToHexString();
  if (mLoadState == LOAD_LOADED)
    setMaterialNames(this);
}

void GraphicsResourceModel::setMaterialNames(GraphicsResourceModel* resourcePtr)
{
  Ogre::ResourcePtr meshResource = Ogre::MeshManager::getSingleton().getByName(resourcePtr->getID());
  Ogre::Mesh* meshPtr = static_cast<Ogre::Mesh *>(&*meshResource);
  Ogre::Mesh::SubMeshIterator currSubMeshIter = meshPtr->getSubMeshIterator();

  while (currSubMeshIter.hasMoreElements()) {
    Ogre::SubMesh *curSubMesh = currSubMeshIter.getNext();

    if (!OPTION_ENABLE_TEXTURES) {
      curSubMesh->setMaterialName("BaseWhiteTexture");
    }
    else {
      const Ogre::String& curMatName = curSubMesh->getMaterialName();
      int pos = curMatName.find_last_of(':');
      if (pos != -1) {
        String start = curMatName.substr(0, pos);
        String ending = curMatName.substr(pos);
        std::map<String, String>::iterator itr = resourcePtr->mMaterialNames.find(start);
        if (itr != resourcePtr->mMaterialNames.end())
          curSubMesh->setMaterialName(itr->second + ending);
      }
    }
  }
}

ResourceDownloadTask* GraphicsResourceModel::createDownloadTask(DependencyManager *manager, ResourceRequestor *resourceRequestor)
{
  return new ResourceDownloadTask(manager, mResourceID, resourceRequestor);
}

ResourceDependencyTask* GraphicsResourceModel::createDependencyTask(DependencyManager *manager)
{
  return new ModelDependencyTask(manager, getWeakPtr(), mResourceID.toString());
}

ResourceLoadTask* GraphicsResourceModel::createLoadTask(DependencyManager *manager)
{
    return new ModelLoadTask(manager, getSharedPtr(), mResourceID.fingerprint(), mLoadEpoch);
}

ResourceUnloadTask* GraphicsResourceModel::createUnloadTask(DependencyManager *manager)
{
  return new ModelUnloadTask(manager, getWeakPtr(), mResourceID.fingerprint(), mLoadEpoch);
}

/***************************** MODEL DEPENDENCY TASK *************************/

ModelDependencyTask::ModelDependencyTask(DependencyManager *mgr, WeakResourcePtr resource, const String& hash)
: ResourceDependencyTask(mgr, resource, hash)
{

}

ModelDependencyTask::~ModelDependencyTask()
{

}

void ModelDependencyTask::operator()()
{
  SharedResourcePtr resourcePtr = mResource.lock();
  if (!resourcePtr) {
    finish(false);
    return;
  }

  if (OPTION_ENABLE_TEXTURES) {
    GraphicsResourceManager *grm = GraphicsResourceManager::getSingletonPtr();

    MemoryBuffer::iterator itr, iend;
    for (itr = mBuffer.begin(), iend = mBuffer.end() - 7; itr != iend; ++itr) {
      if (*itr == 'm'
       && (*(itr + 1)) == 'e'
       && (*(itr + 2)) == 'r'
       && (*(itr + 3)) == 'u'
       && (*(itr + 4)) == ':'
       && (*(itr + 5)) == '/'
       && (*(itr + 6)) == '/') {
        String matDep = "";

        while (itr != iend + 4) {
          if (*itr == '.' && (*(itr + 1) == 'o')  && (*(itr + 2) == 's')) {
            matDep += ".os";
             break;
          }
          matDep += *itr;
          ++itr;
        }

        SharedResourcePtr hashResource = grm->getResourceAsset(URI(matDep), GraphicsResource::MATERIAL);
        resourcePtr->addDependency(hashResource);
      }
    }
  }

  resourcePtr->setCost(mBuffer.size());
  resourcePtr->parsed(true);

  finish(true);
}
/*
  if (curResource) {
    Ogre::ResourcePtr meshResource = Ogre::MeshManager::getSingleton().getByName(CDNArchive::canonicalMhashName(mHash));
    Ogre::Mesh* meshPtr = static_cast<Ogre::Mesh*>(&*meshResource);

    // Add skeleton dependency
    Ogre::String skeletonName = meshPtr->getSkeletonName();
    if (skeletonName != "") {
//      SharedResourcePtr skeletonPtr = grm->createSkeletonResource(skeletonName);
//      curResource->addChild(skeletonPtr);
    }
*/

/***************************** MODEL LOAD TASK *************************/

ModelLoadTask::ModelLoadTask(DependencyManager *mgr, SharedResourcePtr resourcePtr, const SHA256 &hash, unsigned int epoch)
: ResourceLoadTask(mgr, resourcePtr, hash, epoch)
{
}

void ModelLoadTask::doRun() {
    /// FIXME: can we just skip this?  Maybe we need to get rid of Model completely?
//    mResource->loaded(true, mEpoch);
}

/***************************** MODEL UNLOAD TASK *************************/

ModelUnloadTask::ModelUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const SHA256 &hash, unsigned int epoch)
: ResourceUnloadTask(mgr, resource, hash, epoch)
{

}
/*
bool ModelUnloadTask::mainThreadUnload(String name)
{
  Ogre::MeshManager::getSingleton().unload(name);
//  operationCompleted(Ogre::BackgroundProcessTicket(), Ogre::BackgroundProcessResult());
  return false;
}
*/
void ModelUnloadTask::doRun()
{
  /*I REALLY wish this were true*/
  // SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(&ModelUnloadTask::mainThreadUnload, this, mHash));

  String hash = mHash.convertToHexString(); //CDNArchive::canonicalMhashName(mHash);
  Ogre::MeshManager* meshManager = Ogre::MeshManager::getSingletonPtr();
  meshManager->remove(hash);

  Ogre::ResourcePtr meshResource = meshManager->getByName(hash);
  assert(meshResource.isNull());

  SharedResourcePtr resource = mResource.lock();
  if (resource)
    resource->unloaded(true, mEpoch);
}

}
