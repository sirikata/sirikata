/*  Meru
 *  ResourceDownloadTask.hpp
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
#ifndef _RESOURCE_DOWNLOAD_TASK_HPP
#define _RESOURCE_DOWNLOAD_TASK_HPP

#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/Range.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>

namespace Sirikata {

class ResourceDownloadTask
{
public:
    typedef std::tr1::function<void(
        std::tr1::shared_ptr<Transfer::ChunkRequest> request,
        std::tr1::shared_ptr<const Transfer::DenseData> response)> DownloadCallback;

    ResourceDownloadTask(const Transfer::URI& uri, Transfer::TransferPoolPtr transfer_pool, double priority, DownloadCallback cb);
  virtual ~ResourceDownloadTask();

  void setRange(const Transfer::Range &r) {
    mRange = r;
  }

  void mergeData(const Transfer::SparseData &dataToMerge);

  virtual void operator()(std::tr1::shared_ptr<ResourceDownloadTask> thisptr);

  bool isStarted() {
    return mStarted;
  }

    void cancel();
protected:

  void metadataFinished(std::tr1::shared_ptr<ResourceDownloadTask> thisptr,
                        std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                        std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response);

  void chunkFinished(std::tr1::shared_ptr<ResourceDownloadTask> thisptr,
                     std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                     std::tr1::shared_ptr<const Transfer::DenseData> response);


  bool mStarted;
    const Transfer::URI mURI;
    Transfer::TransferPoolPtr mTransferPool;
  Transfer::Range mRange;
  Transfer::SparseData mMergeData;
  double mPriority;
  DownloadCallback cb;
};
typedef std::tr1::shared_ptr<ResourceDownloadTask> ResourceDownloadTaskPtr;

} // namespace Sirikata

#endif
