/*  Meru
 *  GraphicsResourceShader.cpp
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
#include "GraphicsResourceShader.hpp"
#include "ManualMaterialLoader.hpp"
#include "ResourceDependencyTask.hpp"
#include "ResourceLoadTask.hpp"
#include "ResourceLoadingQueue.hpp"
#include "ResourceUnloadTask.hpp"
#include <boost/bind.hpp>

namespace Meru {

class ShaderDependencyTask : public ResourceDependencyTask
{
public:
    ShaderDependencyTask(DependencyManager* mgr, WeakResourcePtr resource, const URI& uri);
  virtual ~ShaderDependencyTask();

  virtual void operator()();
};

class ShaderLoadTask : public ResourceLoadTask
{
public:
  ShaderLoadTask(DependencyManager *mgr, SharedResourcePtr resource, const URI &uri, unsigned int archiveName, unsigned int epoch);

  virtual void doRun();

protected:
  const unsigned int mArchiveName;
};

class ShaderUnloadTask : public ResourceUnloadTask
{
public:
  ShaderUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const URI &uri, unsigned int archiveName, unsigned int epoch);

  virtual void doRun();

protected:
  const unsigned int mArchiveName;
};

GraphicsResourceShader::GraphicsResourceShader(const URI &uri, Sirikata::ProxyObjectPtr proxy)
  : GraphicsResourceAsset(uri, GraphicsResource::SHADER, proxy),
  mArchiveName(CDNArchiveFactory::getSingleton().addArchive())
{

}

GraphicsResourceShader::~GraphicsResourceShader()
{
  if (mLoadState == LOAD_LOADED)
    doUnload();
}

ResourceDownloadTask* GraphicsResourceShader::createDownloadTask(DependencyManager *manager, ResourceRequestor *resourceRequestor)
{
    return new ResourceDownloadTask(manager, mURI, resourceRequestor, mProxy->priority, NULL);
}

ResourceDependencyTask* GraphicsResourceShader::createDependencyTask(DependencyManager *manager)
{
  return new ShaderDependencyTask(manager, getWeakPtr(), mURI);
}

ResourceLoadTask* GraphicsResourceShader::createLoadTask(DependencyManager *manager)
{
  return new ShaderLoadTask(manager, getSharedPtr(), mURI, mArchiveName, mLoadEpoch);
}

ResourceUnloadTask* GraphicsResourceShader::createUnloadTask(DependencyManager *manager)
{
  return new ShaderUnloadTask(manager, getWeakPtr(), mURI, mArchiveName, mLoadEpoch);
}

/***************************** SHADER DEPENDENCY TASK *************************/

ShaderDependencyTask::ShaderDependencyTask(DependencyManager *mgr, WeakResourcePtr resource, const URI& uri)
: ResourceDependencyTask(mgr, resource, uri)
{

}

ShaderDependencyTask::~ShaderDependencyTask()
{

}

void ShaderDependencyTask::operator()()
{
  SharedResourcePtr resourcePtr = mResource.lock();
  if (!resourcePtr) {
    finish(false);
    return;
  }

  resourcePtr->parsed(true);

  finish(true);
}

/***************************** SHADER LOAD TASK *************************/

ShaderLoadTask::ShaderLoadTask(DependencyManager *mgr, SharedResourcePtr resourcePtr, const URI &uri, unsigned int archiveName, unsigned int epoch)
: ResourceLoadTask(mgr, resourcePtr, uri, epoch), mArchiveName(archiveName)
{
}

void ShaderLoadTask::doRun()
{
    CDNArchiveFactory::getSingleton().addArchiveData(mArchiveName, mURI.toString(), mBuffer);
  mResource->loaded(true, mEpoch);
}

/***************************** SHADER UNLOAD TASK *************************/

ShaderUnloadTask::ShaderUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const URI &uri, unsigned int archiveName, unsigned int epoch)
: ResourceUnloadTask(mgr, resource, uri, epoch), mArchiveName(archiveName)
{

}

void ShaderUnloadTask::doRun()
{
  CDNArchiveFactory::getSingleton().clearArchive(mArchiveName);

  SharedResourcePtr resource = mResource.lock();
  if (resource) {
    resource->unloaded(true, mEpoch);
  }
}

}
