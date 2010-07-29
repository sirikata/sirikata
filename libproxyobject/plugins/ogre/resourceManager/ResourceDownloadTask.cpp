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

ResourceDownloadTask::ResourceDownloadTask(DependencyManager *mgr, const RemoteFileId &hash, ResourceRequestor* resourceRequestor)
: DependencyTask(mgr->getQueue()), mHash(hash), mRange(true), mResourceRequestor(resourceRequestor)
{
  mStarted = false;
}

ResourceDownloadTask::~ResourceDownloadTask()
{
//FIXME: How do we unsubscribe from an active download?!?!?!

  //EventSource::getSingleton().unsubscribe(mCurrentDownload);
}

void ResourceDownloadTask::mergeData(const Transfer::SparseData &dataToMerge) {
  for (Transfer::DenseDataList::const_iterator iter =
           dataToMerge.Transfer::DenseDataList::begin();
       iter != dataToMerge.Transfer::DenseDataList::end();
       ++iter) {
    iter->addToList<Transfer::DenseDataList>(iter.std::list<Transfer::DenseDataPtr>::const_iterator::operator*(), mMergeData);
  }
}

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

void ResourceDownloadTask::chunkFinished(std::tr1::shared_ptr<ChunkRequest> request,
            std::tr1::shared_ptr<DenseData> response)
{
  if (response != NULL) {

    cout<<"DOUBLE YAYYYYYYYYYYYYYYYYYYYYYYYYY downloaded chunk from "<<mHash.toString()<<endl<<endl;

    SparseData data = SparseData();
    data.addValidData(response);

    mResourceRequestor->setResourceBuffer(data);
    finish(true);
  }
  else {
    finish(false);
    cout<<"failed chunk download from "<<mHash.toString()<<endl<<endl;
  }
}

void ResourceDownloadTask::metadataFinished(std::tr1::shared_ptr<MetadataRequest> request,
            std::tr1::shared_ptr<RemoteFileMetadata> response)
{

  if (response != NULL) {

    cout<<"YAYYYYYYYYYYYYYYYYYYYYYYYYYY downloaded metadata from "<<mHash.toString()<<endl;
    cout<<"predicted size: "<<response->getSize()<<endl<<endl;
    cout<<response->getSize()<<endl;


    const Range *range = new Range(true);
    const Chunk *chunk = new Chunk(mHash.fingerprint(), *range);
    const RemoteFileMetadata metadata = *response;

    TransferRequestPtr req(new Transfer::ChunkRequest(mHash.uri(), metadata, *chunk, 1.0,
						    std::tr1::bind(&ResourceDownloadTask::chunkFinished, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    TransferPoolPtr pool = (GraphicsResourceManager::getSingleton()).mTransferPool;
    pool->addRequest(req);

  }
  else {
    finish(false);
    cout<<"failed metadata download from "<<mHash.toString()<<endl<<endl;
  }
 }

void ResourceDownloadTask::operator()()
{
    /* mStarted = true;
  mCurrentDownload = Meru::ResourceManager::getSingleton().request(mHash,
       std::tr1::bind(&ResourceDownloadTask::downloadCompleteHandler, this, _1),
       mRange);

       cout<<"about to start downloading something "<<mHash.uri()<<endl<<endl;*/

/* TransferRequestPtr req(
                new MetadataRequest(mHash.uri(), 1, std::tr1::bind(
                &ResourceDownloadTask::metadataFinished, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

  TransferPoolPtr pool = (GraphicsResourceManager::getSingleton()).mTransferPool;

  pool->addRequest(req);

  cout<<endl<<"starting meta download from "<<mHash.toString()<<endl<<endl;*/
}
}
