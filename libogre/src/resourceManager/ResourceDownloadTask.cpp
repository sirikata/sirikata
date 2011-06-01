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

#include <sirikata/ogre/resourceManager/ResourceDownloadTask.hpp>
#include <stdio.h>

using namespace std;
using namespace Sirikata::Transfer;

namespace Sirikata {

ResourceDownloadTask::ResourceDownloadTask(const Transfer::URI &uri, TransferPoolPtr transfer_pool, double priority, DownloadCallback cb)
 : mURI(uri), mTransferPool(transfer_pool), mRange(true),
   mPriority(priority), cb(cb)
{
  mStarted = false;
}

ResourceDownloadTask::~ResourceDownloadTask()
{
//FIXME: How do we unsubscribe from an active download?!?!?!
}

void ResourceDownloadTask::cancel() {
    // FIXME This should fully cancel, but for now its sufficient to just not
    // perform the callback.
    cb = 0;
}

void ResourceDownloadTask::mergeData(const Transfer::SparseData &dataToMerge) {
  for (Transfer::DenseDataList::const_iterator iter =
           dataToMerge.Transfer::DenseDataList::begin();
       iter != dataToMerge.Transfer::DenseDataList::end();
       ++iter) {
    iter->addToList<Transfer::DenseDataList>(iter.std::list<Transfer::DenseDataPtr>::const_iterator::operator*(), mMergeData);
  }
}


void ResourceDownloadTask::chunkFinished(std::tr1::shared_ptr<ResourceDownloadTask> thisptr,
                                         std::tr1::shared_ptr<ChunkRequest> request,
            std::tr1::shared_ptr<const DenseData> response)
{
    // Nothing to do with no callback
    if (!cb) return;

    if (response != NULL) {
        cb(request, response);
    }
    else {
        cb(request, Transfer::DenseDataPtr());
    }
}

void ResourceDownloadTask::metadataFinished(std::tr1::shared_ptr<ResourceDownloadTask> thisptr,
                                            std::tr1::shared_ptr<MetadataRequest> request,
                                            std::tr1::shared_ptr<RemoteFileMetadata> response)
{
  if (response != NULL) {

    //TODO: Support files with more than 1 chunk
    assert(response->getChunkList().size() == 1);

    TransferRequestPtr req(new Transfer::ChunkRequest(mURI, *response,
                                                      response->getChunkList().front(), mPriority,
                                                      std::tr1::bind(&ResourceDownloadTask::chunkFinished, this, thisptr, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);
  }
  else {
      SILOG(ogre,error,"Failed metadata download");
      if (cb) cb(ChunkRequestPtr(), Transfer::DenseDataPtr());
  }
}

void ResourceDownloadTask::operator() ( std::tr1::shared_ptr<ResourceDownloadTask> thisptr)
{
  mStarted = true;

  TransferRequestPtr req(
     new MetadataRequest(mURI, mPriority, std::tr1::bind(
                                                         &ResourceDownloadTask::metadataFinished, this, thisptr, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

 mTransferPool->addRequest(req);
}

} // namespace Sirikata
