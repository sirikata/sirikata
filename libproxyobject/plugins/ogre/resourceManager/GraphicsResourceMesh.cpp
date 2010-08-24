/*  Meru
 *  GraphicsResourceMesh.cpp
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
#include "GraphicsResourceMesh.hpp"
#include "ResourceDependencyTask.hpp"
#include "ResourceLoadTask.hpp"
#include "ResourceLoadingQueue.hpp"
#include "ResourceUnloadTask.hpp"
#include <boost/bind.hpp>
#include <OgreResourceBackgroundQueue.h>
#include <stdio.h>

using namespace std;

namespace Meru {

OptionValue*OPTION_ENABLE_TEXTURES = new OptionValue("enable-textures","true",OptionValueType<bool>(),"Enable or disable texture rendering");

InitializeGlobalOptions graphicsresourcemeshopts("ogregraphics",
    OPTION_ENABLE_TEXTURES,
    NULL);

class MeshDependencyTask : public ResourceDependencyTask
{
public:
    MeshDependencyTask(DependencyManager* mgr, WeakResourcePtr resource, const URI& uri, Sirikata::ProxyObjectPtr proxy);
  virtual ~MeshDependencyTask();

  virtual void operator()();
 Sirikata::ProxyObjectPtr mProxy;
};

class MeshLoadTask : public ResourceLoadTask
{
public:
  MeshLoadTask(DependencyManager *mgr, SharedResourcePtr resource, const URI &uri, unsigned int epoch);

  virtual void doRun();
};

class MeshUnloadTask : public ResourceUnloadTask
{
public:
  MeshUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const URI &uri, unsigned int epoch);

  virtual void doRun();

protected:
  //bool mainThreadUnload(String name);
};

GraphicsResourceMesh::GraphicsResourceMesh(const URI &uri, Sirikata::ProxyObjectPtr proxy)
  : GraphicsResourceAsset(uri, GraphicsResource::MESH, proxy)
{

}

GraphicsResourceMesh::~GraphicsResourceMesh()
{
  if (mLoadState == LOAD_LOADED)
    doUnload();
}

void GraphicsResourceMesh::resolveName(const URI& id)
{
  if (mLoadState == LOAD_LOADED)
    setMaterialNames(this);
}

void GraphicsResourceMesh::setMaterialNames(GraphicsResourceMesh* resourcePtr)
{
  Ogre::ResourcePtr meshResource = Ogre::MeshManager::getSingleton().getByName(resourcePtr->getID());
  Ogre::Mesh* meshPtr = static_cast<Ogre::Mesh *>(&*meshResource);
  Ogre::Mesh::SubMeshIterator currSubMeshIter = meshPtr->getSubMeshIterator();

  while (currSubMeshIter.hasMoreElements()) {
    Ogre::SubMesh *curSubMesh = currSubMeshIter.getNext();

    if (!OPTION_ENABLE_TEXTURES->as<bool>()) {
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

ResourceDownloadTask* GraphicsResourceMesh::createDownloadTask(DependencyManager *manager, ResourceRequestor *resourceRequestor)
{
    return new ResourceDownloadTask(manager, mURI, resourceRequestor, mProxy->priority, NULL);
}

ResourceDependencyTask* GraphicsResourceMesh::createDependencyTask(DependencyManager *manager)
{
    return new MeshDependencyTask(manager, getWeakPtr(), mURI, mProxy);
}

ResourceLoadTask* GraphicsResourceMesh::createLoadTask(DependencyManager *manager)
{

  return new MeshLoadTask(manager, getSharedPtr(), mURI, mLoadEpoch);
}

ResourceUnloadTask* GraphicsResourceMesh::createUnloadTask(DependencyManager *manager)
{
  return new MeshUnloadTask(manager, getWeakPtr(), mURI, mLoadEpoch);
}

/***************************** MESH DEPENDENCY TASK *************************/

MeshDependencyTask::MeshDependencyTask(DependencyManager *mgr, WeakResourcePtr resource, const URI& uri, Sirikata::ProxyObjectPtr proxy)
 : ResourceDependencyTask(mgr, resource, uri), mProxy(proxy)
{

}

MeshDependencyTask::~MeshDependencyTask()
{

}

void MeshDependencyTask::operator()()
{
  SharedResourcePtr resourcePtr = mResource.lock();
  if (!resourcePtr) {
    finish(false);
    return;
  }

  if (OPTION_ENABLE_TEXTURES->as<bool>()) {
    GraphicsResourceManager *grm = GraphicsResourceManager::getSingletonPtr();

    MemoryBuffer::iterator itr, iend;
    for (itr = mBuffer.begin(), iend = mBuffer.end() - 7; itr != iend; ++itr) {
        if (((*itr == 'm'
              && (*(itr + 1)) == 'e'
              && (*(itr + 2)) == 'r'
              && (*(itr + 3)) == 'u')
             ||(*itr == 'h'
              && (*(itr + 1)) == 't'
              && (*(itr + 2)) == 't'
              && (*(itr + 3)) == 'p'))
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

        SharedResourcePtr hashResource = grm->getResourceAsset(URI(matDep), GraphicsResource::MATERIAL, mProxy);
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

/***************************** MESH LOAD TASK *************************/

MeshLoadTask::MeshLoadTask(DependencyManager *mgr, SharedResourcePtr resourcePtr, const URI &uri, unsigned int epoch)
: ResourceLoadTask(mgr, resourcePtr, uri, epoch)
{
}

void MeshLoadTask::doRun()
{
    String str = mURI.toString(); //CDNArchive::canonicalMhashName(mHash);
  int archive = CDNArchiveFactory::getSingleton().addArchive(str, mBuffer);
  Ogre::MeshManager::getSingleton().load(str, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
  CDNArchiveFactory::getSingleton().removeArchive(archive);

  //Ogre::SkeletonPtr skeletonPtr = Ogre::SkeletonManager::getSingleton().getByName(meshPtr->getSkeletonName());
  //if ((!skeletonPtr.isNull()) && (skeletonPtr->isLoaded())) {
  //  meshPtr->_notifySkeleton(skeletonPtr);
  //}

  GraphicsResourceMesh::setMaterialNames(static_cast<GraphicsResourceMesh *>(mResource.get()));

  mResource->loaded(true, mEpoch);
}

/***************************** MESH UNLOAD TASK *************************/

MeshUnloadTask::MeshUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const URI &uri, unsigned int epoch)
: ResourceUnloadTask(mgr, resource, uri, epoch)
{

}
/*
bool MeshUnloadTask::mainThreadUnload(String name)
{
  Ogre::MeshManager::getSingleton().unload(name);
//  operationCompleted(Ogre::BackgroundProcessTicket(), Ogre::BackgroundProcessResult());
  return false;
}
*/
void MeshUnloadTask::doRun()
{
  /*I REALLY wish this were true*/
  // SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(&MeshUnloadTask::mainThreadUnload, this, mHash));

    String str = mURI.toString(); //CDNArchive::canonicalMhashName(mHash);
  Ogre::MeshManager* meshManager = Ogre::MeshManager::getSingletonPtr();
  meshManager->remove(str);

  Ogre::ResourcePtr meshResource = meshManager->getByName(str);
  assert(meshResource.isNull());

  SharedResourcePtr resource = mResource.lock();
  if (resource)
    resource->unloaded(true, mEpoch);
}

}
