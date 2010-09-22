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

#include <sirikata/core/util/Thread.hpp>

#include "../meruCompat/EventSource.hpp"
#include "../meruCompat/Event.hpp"
#include "ResourceDownloadTask.hpp"
#include "ResourceTransfer.hpp"
#include "../meruCompat/DependencyManager.hpp"
#include <stdio.h>
#include "GraphicsResourceManager.hpp"

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;


namespace Meru {

ResourceRequestor::~ResourceRequestor() {
}

ResourceDownloadTask::ResourceDownloadTask(DependencyManager *mgr, const URI &uri,
        ResourceRequestor* resourceRequestor, double priority, DownloadCallback cb)
 : DependencyTask(mgr == NULL ? NULL : mgr->getQueue()), mURI(uri), mRange(true),
   mResourceRequestor(resourceRequestor), mPriority(priority), cb(cb)
{
  mStarted = false;
  if (mgr == NULL) {
      customCb = true;
  }
   else customCb = false;
}

ResourceDownloadTask::~ResourceDownloadTask()
{
//FIXME: How do we unsubscribe from an active download?!?!?!

  //EventSource::getSingleton().unsubscribe(mCurrentDownload);
}

//Old Download completion callback, used with the TransferManager, no longer
//necessary if all downloads go through the TransferMediator
/*
EventResponse ResourceDownloadTask::downloadCompleteHandler(const EventPtr& event)
{
  std::tr1::shared_ptr<DownloadCompleteEvent> transferEvent = DowncastEvent<DownloadCompleteEvent>(event);
  if (transferEvent->success()) {
    Transfer::SparseData finishedData = transferEvent->data();
    for (Transfer::DenseDataList::const_iterator iter = mMergeData.Transfer::DenseDataList::begin();
         iter != mMergeData.Transfer::DenseDataList::end();
         ++iter) {
      iter->addToList<Transfer::DenseDataList>(iter.std::list<Transfer::DenseDataPtr>::const_iterator::operator*(), finishedData);
    }
    mResourceRequestor->setResourceBuffer(finishedData);
  }
  else {
    //assert(false); // ???
  }
  finish(transferEvent->success());
  return EventResponse::del();
}
*/

void ResourceDownloadTask::mergeData(const Transfer::SparseData &dataToMerge) {
  for (Transfer::DenseDataList::const_iterator iter =
           dataToMerge.Transfer::DenseDataList::begin();
       iter != dataToMerge.Transfer::DenseDataList::end();
       ++iter) {
    iter->addToList<Transfer::DenseDataList>(iter.std::list<Transfer::DenseDataPtr>::const_iterator::operator*(), mMergeData);
  }
}


void ResourceDownloadTask::chunkFinished(std::tr1::shared_ptr<ChunkRequest> request,
            std::tr1::shared_ptr<DenseData> response)
{
    if (response != NULL) {
      if (customCb == false) {
          SparseData data = SparseData();
          data.addValidData(response);

          mResourceRequestor->setResourceBuffer(data);
      } else {
          cb(request, response);
      }
      finish(true);
  }
  else {
    finish(false);
    cout<<"Failed chunk download"<<endl;
  }
}

void ResourceDownloadTask::metadataFinished(std::tr1::shared_ptr<MetadataRequest> request,
            std::tr1::shared_ptr<RemoteFileMetadata> response)
{
  if (response != NULL) {

    //TODO: Support files with more than 1 chunk
    assert(response->getChunkList().size() == 1);

    TransferRequestPtr req(new Transfer::ChunkRequest(mURI, *response, response->getChunkList().front(), mPriority,
            std::tr1::bind(&ResourceDownloadTask::chunkFinished, this, std::tr1::placeholders::_1,
                std::tr1::placeholders::_2)));

    TransferPoolPtr pool = (GraphicsResourceManager::getSingleton()).transferPool();
    pool->addRequest(req);

  }
  else {
    finish(false);
    cout<<"Failed metadata download"<<endl;
  }
 }

void ResourceDownloadTask::operator()()
{

 mStarted = true;

 TransferRequestPtr req(
     new MetadataRequest(mURI, mPriority, std::tr1::bind(
             &ResourceDownloadTask::metadataFinished, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

  TransferPoolPtr pool = (GraphicsResourceManager::getSingleton()).transferPool();
   pool->addRequest(req);

  //Old code that launches a download through the TransferManager, no longer
  //necessary if download is triggered through the TransferMediator
 /* mCurrentDownload = Meru::ResourceManager::getSingleton().request(mHash,
       std::tr1::bind(&ResourceDownloadTask::downloadCompleteHandler, this, _1),
       mRange);*/

}
}
