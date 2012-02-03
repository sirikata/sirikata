/*  Sirikata Transfer -- Content Transfer management system
 *  TransferPool.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferHandlers.hpp>
#include <sirikata/core/transfer/MeerkatTransferHandler.hpp>
#include <sirikata/core/transfer/FileTransferHandler.hpp>
#include <sirikata/core/transfer/HttpTransferHandler.hpp>
#include <sirikata/core/transfer/DataTransferHandler.hpp>

namespace Sirikata {
namespace Transfer {

void MetadataRequest::execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb) {
    std::tr1::shared_ptr<MetadataRequest> casted =
      std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(req);
    if (casted->getURI().scheme() == "meerkat") {
        MeerkatNameHandler::getSingleton().resolve(casted, std::tr1::bind(
                    &MetadataRequest::execute_finished, this, _1, cb));
    } else if (casted->getURI().scheme() == "file") {
        FileNameHandler::getSingleton().resolve(casted, std::tr1::bind(
                    &MetadataRequest::execute_finished, this, _1, cb));
    } else if (casted->getURI().scheme() == "http") {
        HttpNameHandler::getSingleton().resolve(casted, std::tr1::bind(
                    &MetadataRequest::execute_finished, this, _1, cb));
    } else if (casted->getURI().scheme() == "data") {
        DataNameHandler::getSingleton().resolve(casted, std::tr1::bind(
                    &MetadataRequest::execute_finished, this, _1, cb));
    } else {
        SILOG(transfer, error, "Got unknown protocol in Metadata request: " << casted->getURI().scheme());
        std::tr1::shared_ptr<RemoteFileMetadata> bad;
        execute_finished(bad, cb);
    }
}


void ChunkRequest::execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb) {
    std::tr1::shared_ptr<ChunkRequest> casted =
            std::tr1::static_pointer_cast<ChunkRequest, TransferRequest>(req);

    if (casted->getMetadata().getURI().scheme() == "meerkat") {
        MeerkatChunkHandler::getSingleton().get(mMetadata, mChunk, std::tr1::bind(
                &ChunkRequest::execute_finished, this, _1, cb));
    } else if (casted->getMetadata().getURI().scheme() == "file") {
        FileChunkHandler::getSingleton().get(mMetadata, mChunk, std::tr1::bind(
                &ChunkRequest::execute_finished, this, _1, cb));
    } else if (casted->getMetadata().getURI().scheme() == "http") {
        HttpChunkHandler::getSingleton().get(mMetadata, mChunk, std::tr1::bind(
                &ChunkRequest::execute_finished, this, _1, cb));
    } else if (casted->getMetadata().getURI().scheme() == "data") {
        DataChunkHandler::getSingleton().get(mMetadata, mChunk, std::tr1::bind(
                &ChunkRequest::execute_finished, this, _1, cb));
    } else {
        SILOG(transfer, error, "Got unknown protocol in Chunk request: " << casted->getMetadata().getURI().scheme());
        std::tr1::shared_ptr<const DenseData> bad;
        execute_finished(bad, cb);
    }
}

void ChunkRequest::execute_finished(std::tr1::shared_ptr<const DenseData> response, ExecuteFinished cb) {
    SILOG(transfer, detailed, "execute_finished in ChunkRequest called");
    mDenseData = response;
    HttpManager::getSingleton().postCallback(cb, "ChunkRequest::execute_finished callback");
    SILOG(transfer, detailed, "done ChunkRequest execute_finished");
}

void ChunkRequest::notifyCaller(TransferRequestPtr me, TransferRequestPtr from) {
    ChunkRequestPtr fromC =
        std::tr1::static_pointer_cast<ChunkRequest, TransferRequest>(from);
    notifyCaller(me, from, fromC->mDenseData);
}

void ChunkRequest::notifyCaller(TransferRequestPtr me, TransferRequestPtr from, DenseDataPtr data) {
    ChunkRequestPtr meC =
        std::tr1::static_pointer_cast<ChunkRequest, TransferRequest>(me);
    ChunkRequestPtr fromC =
        std::tr1::static_pointer_cast<ChunkRequest, TransferRequest>(from);

    // We need to be careful about what we pass to the
    // callback. Currently, the only accessible information that
    // wouldn't be in our own object but is in the request we're
    // generating the callback from is the file metadata. Other things
    // (e.g. the response data) are internal only.
    if (meC->mURI == fromC->mURI) {
        meC->mMetadata = fromC->mMetadata;
    }
    else {
        // Replace the RemoteFileMetadata, but overwrite the requested URL
        // with the one from this request.
        RemoteFileMetadataPtr frommd = fromC->mMetadata;
        meC->mMetadata = RemoteFileMetadataPtr(
            new RemoteFileMetadata(
                frommd->getFingerprint(), frommd->getURI(), frommd->getSize(),
                frommd->getChunkList(), frommd->getHeaders()
            )
        );
    }

    // To pass back the original request, we need to copy data over
    HttpManager::getSingleton().postCallback(std::tr1::bind(mCallback, meC, data), "ChunkRequest::notifyCaller callback");
    SILOG(transfer, detailed, "done ChunkRequest notifyCaller");
}

void DirectChunkRequest::execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb) {
    std::tr1::shared_ptr<DirectChunkRequest> casted =
            std::tr1::static_pointer_cast<DirectChunkRequest, TransferRequest>(req);

    MeerkatChunkHandler::getSingleton().get(mChunk,
            std::tr1::bind(&DirectChunkRequest::execute_finished, this, _1, cb));
}

void DirectChunkRequest::execute_finished(std::tr1::shared_ptr<const DenseData> response, ExecuteFinished cb) {
    SILOG(transfer, detailed, "execute_finished in DirectChunkRequest called");
    mDenseData = response;
    HttpManager::getSingleton().postCallback(cb, "DirectChunkRequest::execute_finished callback");
    SILOG(transfer, detailed, "done DirectChunkRequest execute_finished");
}

void DirectChunkRequest::notifyCaller(TransferRequestPtr me, TransferRequestPtr from) {
    DirectChunkRequestPtr fromC =
        std::tr1::static_pointer_cast<DirectChunkRequest, TransferRequest>(from);
    notifyCaller(me, from, fromC->mDenseData);
}

void DirectChunkRequest::notifyCaller(TransferRequestPtr me, TransferRequestPtr from, DenseDataPtr data) {
    DirectChunkRequestPtr meC =
        std::tr1::static_pointer_cast<DirectChunkRequest, TransferRequest>(me);
    DirectChunkRequestPtr fromC =
        std::tr1::static_pointer_cast<DirectChunkRequest, TransferRequest>(from);

    // To pass back the original request, we need to copy data over
    HttpManager::getSingleton().postCallback(std::tr1::bind(mCallback, meC, data), "DirectChunkRequest::notifyCaller callback");
    SILOG(transfer, detailed, "done DirectChunkRequest notifyCaller");
}

} // namespace Transfer
} // namespace Sirikata
