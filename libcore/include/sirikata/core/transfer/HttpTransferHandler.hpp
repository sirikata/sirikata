// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef SIRIKATA_HttpTransferHandler_HPP__
#define SIRIKATA_HttpTransferHandler_HPP__

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
class SIRIKATA_EXPORT HttpNameHandler
    : public NameHandler, public AutoSingleton<HttpNameHandler> {

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
            ChunkCallback callback);

    static HttpChunkHandler& getSingleton();
    static void destroy();
};

}
}

#endif
