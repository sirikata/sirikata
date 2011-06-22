/*  Sirikata Transfer -- Content Transfer management system
 *  MeerkatTransferHandler.hpp
 *
 *  Copyright (c) 2011, Jeff Terrace
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
/*  Created on: Jun 18th, 2010 */

#ifndef SIRIKATA_MeerkatTransferHandler_HPP__
#define SIRIKATA_MeerkatTransferHandler_HPP__

#include <map>
#include <queue>
#include <string>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/HttpManager.hpp>
#include <boost/asio.hpp>
#include <sirikata/core/network/Address.hpp>

namespace Sirikata {
namespace Transfer {

/*
 * Implements name lookups via HTTP
 */
class SIRIKATA_EXPORT MeerkatNameHandler
    : public NameHandler, public AutoSingleton<MeerkatNameHandler> {

private:
    //TODO: should get these from settings
    const std::string CDN_HOST_NAME;
    const std::string CDN_SERVICE;
    const std::string CDN_DNS_URI_PREFIX;
    const Network::Address mCdnAddr;

public:
    MeerkatNameHandler();
    ~MeerkatNameHandler();

    /*
     * Resolves a metadata request via HTTP and calls callback when completed
     */
    void resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback);

    /*
     * Callback from HttpManager when an http request finishes
     */
    void request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
            HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
            std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback);

    static MeerkatNameHandler& getSingleton();
    static void destroy();
};

/*
 * Implements chunk downloading via HTTP
 */
class SIRIKATA_EXPORT MeerkatChunkHandler
    : public ChunkHandler, public AutoSingleton<MeerkatChunkHandler> {

private:
    //TODO: should get these from settings
    const std::string CDN_HOST_NAME;
    const std::string CDN_SERVICE;
    const std::string CDN_DOWNLOAD_URI_PREFIX;
    const Network::Address mCdnAddr;

    void cache_check_callback(const SparseData* data, std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback);

public:
    MeerkatChunkHandler();
    ~MeerkatChunkHandler();

    /*
     * Downloads the chunk referenced and calls callback when completed
     */
    void get(std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback);

    /*
     * Callback from HttpManager when an http request finishes
     */
    void request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
            HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
            std::tr1::shared_ptr<RemoteFileMetadata> file, std::tr1::shared_ptr<Chunk> chunk,
            bool chunkReq, ChunkCallback callback);

    static MeerkatChunkHandler& getSingleton();
    static void destroy();
};

}
}

#endif
