/*  Sirikata
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

#ifndef _SIRIKATA_CORE_TRANSFER_RESOURCE_DOWNLOAD_TASK_HPP
#define _SIRIKATA_CORE_TRANSFER_RESOURCE_DOWNLOAD_TASK_HPP

#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/Range.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/util/SelfWeakPtr.hpp>

namespace Sirikata {
namespace Transfer {

class ResourceDownloadTask;
typedef std::tr1::shared_ptr<ResourceDownloadTask> ResourceDownloadTaskPtr;
typedef std::tr1::weak_ptr<ResourceDownloadTask> ResourceDownloadTaskWPtr;

/** ResourceDownloadTask is a helper class for downloading an entire
 *  resource. It handles getting the metadata and all the chunks, and then
 *  invokes a callback. The callback is also invoked, with empty data, if any
 *  part of the transfer failed.
 *
 *  This class should always be used as ResourceDownloadTaskPtr and you must
 *  maintain a reference for it to work properly -- the callback will not be
 *  invoked if you let the object's refcount drop to 0.
 */
class SIRIKATA_EXPORT ResourceDownloadTask : public SelfWeakPtr<ResourceDownloadTask> {
public:
    typedef std::tr1::function<void(
        ChunkRequestPtr request,
        DenseDataPtr response)> DownloadCallback;

    static ResourceDownloadTaskPtr construct(const URI& uri, TransferPoolPtr transfer_pool, double priority, DownloadCallback cb);
    virtual ~ResourceDownloadTask();

    void setRange(const Range &r) {
        mRange = r;
    }

    void mergeData(const SparseData &dataToMerge);

    void start();

    bool isStarted() {
        return mStarted;
    }

    void cancel();
protected:
    ResourceDownloadTask(const URI& uri, TransferPoolPtr transfer_pool, double priority, DownloadCallback cb);

    static void metadataFinishedWeak(ResourceDownloadTaskWPtr thiswptr,
        MetadataRequestPtr request,
        RemoteFileMetadataPtr response);
    void metadataFinished(MetadataRequestPtr request,
        RemoteFileMetadataPtr response);

    static void chunkFinishedWeak(ResourceDownloadTaskWPtr thiswptr,
        ChunkRequestPtr request,
        DenseDataPtr response);
    void chunkFinished(ChunkRequestPtr request,
        DenseDataPtr response);


    bool mStarted;
    const URI mURI;
    TransferPoolPtr mTransferPool;
    Range mRange;
    SparseData mMergeData;
    double mPriority;
    DownloadCallback cb;
};

} // namespace Transfer
} // namespace Sirikata

#endif // _SIRIKATA_CORE_TRANSFER_RESOURCE_DOWNLOAD_TASK_HPP
