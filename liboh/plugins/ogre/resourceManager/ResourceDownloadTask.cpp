/*  Meru
 *  ResourceDownloadTask.cpp
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
#include "EventSource.hpp"
#include "Event.hpp"
#include "ResourceDownloadTask.hpp"
#include "ResourceTransfer.hpp"
#include "DependencyManager.hpp"

namespace Meru {

ResourceRequestor::~ResourceRequestor() {
}

ResourceDownloadTask::ResourceDownloadTask(DependencyManager *mgr, const RemoteFileId &hash, ResourceRequestor* resourceRequestor)
: DependencyTask(mgr->getQueue()), mHash(hash), mResourceRequestor(resourceRequestor)
{
  mStarted = false;
}

ResourceDownloadTask::~ResourceDownloadTask()
{
//FIXME: How do we unsubscribe from an active download?!?!?!

  EventSource::getSingleton().unsubscribe(mCurrentDownload);
}

EventResponse ResourceDownloadTask::downloadCompleteHandler(const EventPtr& event)
{
  std::tr1::shared_ptr<DownloadCompleteEvent> transferEvent = DowncastEvent<DownloadCompleteEvent>(event);
  if (transferEvent->success()) {
    mResourceRequestor->setResourceBuffer(transferEvent->data());
  }
  else {
    //assert(false); // ???
  }
  finish(transferEvent->success());
  return EventResponse::del();
}

void ResourceDownloadTask::operator()()
{
  mStarted = true;
  // FIXME: Daniel: the defaultProgressiveDownloadFunctor will not properly deal with textures
  mCurrentDownload = Meru::ResourceManager::getSingleton().request(mHash,
      std::tr1::bind(&ResourceDownloadTask::downloadCompleteHandler, this, _1));
}

}
