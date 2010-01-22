/*  Meru
 *  GraphicsResourceSkeleton.cpp
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
#include "../meruCompat/DependencyTask.hpp"
#include "GraphicsResourceSkeleton.hpp"
#include <OgreResourceBackgroundQueue.h>
#include <OgreSkeletonManager.h>
#include "ResourceLoadingQueue.hpp"

namespace Meru {

  /*
  DependencyTask *skeletonLoadTask = new SkeletonLoadTask(mManager, skeletonName);
  mManager->establishDependencyRelationship(*dependentIter, skeletonLoadTask);

  DependencyTask *skeletonDownloadTask = new ResourceDependencyTask(mManager, skeletonName, SKELETON_DOWNLOAD, mArchiveName,SkeletonDownloadTaskName);
  mManager->establishDependencyRelationship(skeletonLoadTask, skeletonDownloadTask);
  */

static const String SkeletonLoadTaskName("SkeletonLoadTask");
class SkeletonLoadTask : public DependencyTask, public Ogre::ResourceBackgroundQueue::Listener
{
  String mName;
public:
  SkeletonLoadTask(DependencyManager *mgr, const String &name)
   : DependencyTask(mgr->getQueue()), mName(name)
  {
  }

  void operationCompleted(Ogre::BackgroundProcessTicket a , const Ogre::BackgroundProcessResult &b) {
    operationCompleted(a);
  }

  void operationCompleted (Ogre::BackgroundProcessTicket)
  {
    SILOG(resource,insane, "Skeleton Load Task opComplete(), waited " << mName);
    finish(true);
  }

  void operator()()
  {
    Meru::ResourceLoadingQueue *queue = &ResourceLoadingQueue::getSingleton();

    Ogre::ResourcePtr skeletonPtr = Ogre::SkeletonManager::getSingleton().getByName(mName);
    if (skeletonPtr.isNull() || !skeletonPtr->isLoaded()) {
      SILOG(resource,insane, "Skeleton Load Task run(), waiting for " << mName << " To queue");
      SILOG(resource,debug,"Queueing " << mName << " for OGRE load ");
      queue->load(Ogre::SkeletonManager::getSingleton().getResourceType(), mName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, false, NULL, NULL, this);
    }
    else {
      SILOG(resource,insane, "Skeleton Load Task run() AND complete, waited for " << mName << " To queue");
      finish(true);
    }
  }
};

}
