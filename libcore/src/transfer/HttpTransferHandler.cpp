// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <map>
#include <string>
#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferHandlers.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <boost/bind.hpp>
#include <sirikata/core/transfer/HttpTransferHandler.hpp>
#include <sirikata/core/transfer/URL.hpp>
#include <boost/lexical_cast.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::HttpNameHandler);
AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::HttpChunkHandler);

namespace Sirikata {
namespace Transfer {

HttpNameHandler& HttpNameHandler::getSingleton() {
    return AutoSingleton<HttpNameHandler>::getSingleton();
}
void HttpNameHandler::destroy() {
    AutoSingleton<HttpNameHandler>::destroy();
}


HttpChunkHandler& HttpChunkHandler::getSingleton() {
    return AutoSingleton<HttpChunkHandler>::getSingleton();
}
void HttpChunkHandler::destroy() {
    AutoSingleton<HttpChunkHandler>::destroy();
}


HttpNameHandler::HttpNameHandler()
{
}

HttpNameHandler::~HttpNameHandler() {

}

void HttpNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {
    URL url(request->getURI());
    assert(!url.empty());

    std::tr1::shared_ptr<RemoteFileMetadata> bad;
    if (url.host() == "") {
        HttpManager::getSingleton().postCallback(std::tr1::bind(callback,bad), "HttpNameHandler::resolve callback");
        return;
    }

    std::string host_name = url.hostname();
    std::string service = url.context().service();
    if (service.empty()) service = "http";
    Network::Address cdn_addr(host_name, service);

    HttpManager::Headers headers;
    headers["Host"] = host_name;
    headers["Accept-Encoding"] = "deflate, gzip";

    HttpManager::getSingleton().get(
        cdn_addr, url.fullpath(),
        std::tr1::bind(&HttpNameHandler::request_finished, this, _1, _2, _3, request, callback),
        headers
    );
}

void HttpNameHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {

    std::tr1::shared_ptr<RemoteFileMetadata> bad;

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP name lookup (" << request->getURI() << "). Boost error = " << boost_error.message());
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP name lookup. (" << request->getURI() << ")");
        callback(bad);
        return;
    }


    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }


    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    if (!response->getData()) {
        SILOG(transfer, error, "Body not present during an HTTP 'name lookup' (" << request->getURI() << ")");
        callback(bad);
        return;
    }


    Fingerprint fp = SparseData(response->getData()).computeFingerprint();

    //Just treat everything as a single chunk for now
    Range::length_type file_size = response->getData()->length();
    Range whole(0, file_size, LENGTH, true);
    Chunk chunk(fp, whole);
    ChunkList chunkList;
    chunkList.push_back(chunk);

    SharedChunkCache::getSingleton().getCache()->addToCache(fp, response->getData());
    std::tr1::shared_ptr<RemoteFileMetadata> met(new RemoteFileMetadata(fp, request->getURI(),
            file_size, chunkList, response->getRawHeaders()));
    callback(met);
}

HttpChunkHandler::HttpChunkHandler()
{
}

HttpChunkHandler::~HttpChunkHandler() {
}

void HttpChunkHandler::get(std::tr1::shared_ptr<RemoteFileMetadata> file,
        std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {

    //Check for null arguments
    std::tr1::shared_ptr<DenseData> bad;
    if (!file) {
        SILOG(transfer, error, "HttpChunkHandler get called with null file parameter");
        callback(bad);
        return;
    }
    if (!chunk) {
        SILOG(transfer, error, "HttpChunkHandler get called with null chunk parameter");
        callback(bad);
        return;
    }

    //Make sure chunk given is part of file
    bool foundIt = false;
    const ChunkList & chunks = file->getChunkList();
    for (ChunkList::const_iterator it = chunks.begin(); it != chunks.end(); it++) {
        if(*chunk == *it) {
            foundIt = true;
        }
    }
    if(!foundIt) {
        SILOG(transfer, error, "HttpChunkHandler get called with chunk not present in file metadata");
        callback(bad);
        return;
    }

    //Check to see if it's in the cache first
    SharedChunkCache::getSingleton().getCache()->getData(file->getFingerprint(), chunk->getRange(), std::tr1::bind(
            &HttpChunkHandler::cache_check_callback, this, _1, file, chunk, callback));
}

void HttpChunkHandler::cache_check_callback(const SparseData* data, std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {
    if (data) {
        std::tr1::shared_ptr<const DenseData> flattened = data->flatten();
        callback(flattened);
    } else {
        URL url(file->getURI());
        assert(!url.empty());

        std::string host_name = url.hostname();
        std::string service = url.context().service();
        if (service.empty()) service = "http";
        Network::Address cdn_addr(host_name, service);

        HttpManager::Headers headers;
        headers["Host"] = host_name;
        headers["Accept-Encoding"] = "deflate, gzip";

        HttpManager::getSingleton().get(
            cdn_addr, url.fullpath(),
            std::tr1::bind(&HttpChunkHandler::request_finished, this, _1, _2, _3, file, chunk, callback),
            headers
        );
    }
}

void HttpChunkHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        std::tr1::shared_ptr<RemoteFileMetadata> file, std::tr1::shared_ptr<Chunk> chunk,
        ChunkCallback callback) {

    std::tr1::shared_ptr<DenseData> bad;
    std::string reqType = "file request";

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP " << reqType << " (" << file->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP " << reqType << " (" << file->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP " << reqType << ". Boost error = " << boost_error.message() << " (" << file->getURI() << ")");
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP " << reqType << " (" << file->getURI() << ")");
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP " << reqType << " (" << file->getURI() << ")");
        callback(bad);
        return;
    }


    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP " << reqType << " (" << file->getURI() << ")");
        callback(bad);
        return;
    }

    if (!response->getData()) {
        SILOG(transfer, error, "Body not present during an HTTP " << reqType << " (" << file->getURI() << ")");
        callback(bad);
        return;
    }

    SILOG(transfer, detailed, "about to call addToCache with fingerprint ID = " << file->getFingerprint().convertToHexString());
    SharedChunkCache::getSingleton().getCache()->addToCache(file->getFingerprint(), response->getData());

    callback(response->getData());
    SILOG(transfer, detailed, "done http chunk handler request_finished");
}

}
}
