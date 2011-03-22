/*  Sirikata Transfer -- Content Transfer management system
 *  TransferHandlers.hpp
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
/*  Created on: Jun 18th, 2010 */

#ifndef SIRIKATA_TransferHandlers_HPP__
#define SIRIKATA_TransferHandlers_HPP__

#include <map>
#include <queue>
#include <string>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/HttpManager.hpp>
#include <boost/asio.hpp>
#include <sirikata/core/network/Address.hpp>
#include <sirikata/core/transfer/DiskCacheLayer.hpp>
#include <sirikata/core/transfer/MemoryCacheLayer.hpp>
#include <sirikata/core/transfer/LRUPolicy.hpp>

namespace Sirikata {
namespace Transfer {

//Forward declaration
class MetadataRequest;

/*
 * Base class for an implementation that satisfies name lookups
 * The input is a MetadataRequest and output is RemoteFileMetadata
 */
class NameHandler {

public:
    typedef std::tr1::function<void(
                std::tr1::shared_ptr<RemoteFileMetadata> response
            )> NameCallback;

    virtual void resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) = 0;

	virtual ~NameHandler() {
	}

};

/*
 * Base class for an implementation that satisfies chunk downloads
 * The input is a RemoteFileMetadata and a Chunk, output is DenseData
 */
class ChunkHandler {

public:
    typedef std::tr1::function<void(
                std::tr1::shared_ptr<const DenseData> response
            )> ChunkCallback;

    virtual void get(std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) = 0;

    virtual ~ChunkHandler() {
    }

};

/*
 * Implements name lookups via HTTP
 */
class SIRIKATA_EXPORT HttpNameHandler
    : public NameHandler, public AutoSingleton<HttpNameHandler> {

private:
	//TODO: should get these from settings
    const std::string CDN_HOST_NAME;
    const std::string CDN_SERVICE;
    const std::string CDN_DNS_URI_PREFIX;
	const Network::Address mCdnAddr;

public:
	HttpNameHandler();
	~HttpNameHandler();

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

    static HttpNameHandler& getSingleton();
    static void destroy();
};

/*
 * Implements chunk downloading via HTTP
 */
class SIRIKATA_EXPORT HttpChunkHandler
    : public ChunkHandler, public AutoSingleton<HttpChunkHandler> {

private:
    //TODO: should get these from settings
    const std::string CDN_HOST_NAME;
    const std::string CDN_SERVICE;
    const std::string CDN_DOWNLOAD_URI_PREFIX;
    const Network::Address mCdnAddr;
    static const unsigned int DISK_LRU_CACHE_SIZE;
    static const unsigned int MEMORY_LRU_CACHE_SIZE;

    CachePolicy* mDiskCachePolicy;
    CachePolicy* mMemoryCachePolicy;
    std::vector<CacheLayer*> mCacheLayers;
    CacheLayer* mCache;

    void cache_check_callback(const SparseData* data, std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback);

public:
    HttpChunkHandler();
    ~HttpChunkHandler();

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

    static HttpChunkHandler& getSingleton();
    static void destroy();
};

}
}

#endif
