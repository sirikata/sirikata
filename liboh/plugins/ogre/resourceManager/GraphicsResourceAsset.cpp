/*  Meru
 *  GraphicsResourceAsset.cpp
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
#include "DependencyManager.hpp"
#include "GraphicsResourceAsset.hpp"
#include "GraphicsResourceManager.hpp"
#include "ResourceDependencyTask.hpp"
#include "ResourceDownloadTask.hpp"
#include "ResourceLoadTask.hpp"
#include "ResourceUnloadTask.hpp"

namespace Meru {

GraphicsResourceAsset::GraphicsResourceAsset(const RemoteFileId &resourceID, GraphicsResource::Type resourceType)
: GraphicsResource(resourceID.fingerprint().convertToHexString(), resourceType), mResourceID(resourceID),
  mLoadTask(NULL), mParseTask(NULL), mUnloadTask(NULL)
{

}

GraphicsResourceAsset::~GraphicsResourceAsset()
{

}

void GraphicsResourceAsset::parsed(bool success)
{
  mParseTask = NULL;
  GraphicsResource::parsed(success);
}

void GraphicsResourceAsset::loaded(bool success, unsigned int epoch)
{
  if (mLoadTask && mLoadTask->isStarted())
  /// FIXME: Um... yeah let's just leak the memory I guess...? -Patrick 2009-05-04
    mLoadTask = NULL;
  GraphicsResource::loaded(success, epoch);
}

void GraphicsResourceAsset::unloaded(bool success, unsigned int epoch)
{
  if (mUnloadTask && mUnloadTask->isStarted())
    mUnloadTask = NULL;
  GraphicsResource::unloaded(success, epoch);
}

void GraphicsResourceAsset::doLoad()
{
//  assert(mLoadTask == NULL);

  if (mUnloadTask) {
    mUnloadTask->cancel();
    mUnloadTask = NULL;
  }

  GraphicsResourceManager* grm = GraphicsResourceManager::getSingletonPtr();
  DependencyManager* depMgr = grm->getDependencyManager();

  mLoadTask = createLoadTask(depMgr);
  ResourceDownloadTask *downloadTask = createDownloadTask(depMgr, mLoadTask);
  depMgr->establishDependencyRelationship(mLoadTask, downloadTask);
  downloadTask->go();
}

void GraphicsResourceAsset::doUnload()
{
//  assert(mUnloadTask == NULL);

  if (mLoadTask) {
    mLoadTask->cancel();
    mLoadTask = NULL;
  }

  GraphicsResourceManager* grm = GraphicsResourceManager::getSingletonPtr();
  DependencyManager* depMgr = grm->getDependencyManager();

  mUnloadTask = createUnloadTask(depMgr);
  mUnloadTask->go();
}

void GraphicsResourceAsset::doParse()
{
  assert(mParseTask == NULL);

  GraphicsResourceManager* grm = GraphicsResourceManager::getSingletonPtr();
  DependencyManager* depMgr = grm->getDependencyManager();

  mParseTask = createDependencyTask(depMgr);
  ResourceDownloadTask *downloadTask = createDownloadTask(depMgr, mParseTask);
  depMgr->establishDependencyRelationship(mParseTask, downloadTask);
  downloadTask->go();
}

}
