/*  Sirikata Transfer -- Content Transfer management system
 *  TransferHandlers.cpp
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

#include <map>
#include <string>
#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferHandlers.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <boost/bind.hpp>

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
const char HttpNameHandler::CDN_HOST_NAME [] = "cdn.sirikata.com";
const char HttpNameHandler::CDN_SERVICE [] = "http";


HttpChunkHandler& HttpChunkHandler::getSingleton() {
    return AutoSingleton<HttpChunkHandler>::getSingleton();
}
void HttpChunkHandler::destroy() {
    AutoSingleton<HttpChunkHandler>::destroy();
}
const char HttpChunkHandler::CDN_HOST_NAME [] = "cdn.sirikata.com";
const char HttpChunkHandler::CDN_SERVICE [] = "http";


HttpNameHandler::HttpNameHandler()
    : mCdnAddr(CDN_HOST_NAME, CDN_SERVICE) {

}

HttpNameHandler::~HttpNameHandler() {

}

void HttpNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {
    std::ostringstream request_stream;
    request_stream << "HEAD /dns/global" << request->getURI().fullpath() << " HTTP/1.1\r\n";
    request_stream << "Host: " << CDN_HOST_NAME << "\r\n";
    request_stream << "Accept: * /*\r\n\r\n";

    HttpManager::getSingleton().makeRequest(mCdnAddr, Transfer::HttpManager::HEAD, request_stream.str(), std::tr1::bind(
            &HttpNameHandler::request_finished, this, _1, _2, _3, request, callback));
}

void HttpNameHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {

    std::tr1::shared_ptr<RemoteFileMetadata> bad;

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP name lookup");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP name lookup");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP name lookup. Boost error = " << boost_error.message());
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP name lookup.");
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP name lookup");
        callback(bad);
        return;
    }

    std::map<std::string, std::string>::const_iterator it;
    it = response->getHeaders().find("Content-Length");
    if (it != response->getHeaders().end()) {
        SILOG(transfer, error, "Content-Length header was present when it shouldn't be during an HTTP name lookup");
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP name lookup");
        callback(bad);
        return;
    }

    it = response->getHeaders().find("File-Size");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected File-Size header not present during an HTTP name lookup");
        callback(bad);
        return;
    }
    std::string file_size_str = it->second;

    it = response->getHeaders().find("Hash");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected Hash header not present during an HTTP name lookup");
        callback(bad);
        return;
    }
    std::string hash = it->second;

    if (response->getData()) {
        SILOG(transfer, error, "Body present during an HTTP name lookup");
        callback(bad);
        return;
    }

    Fingerprint fp;
    try {
        fp = Fingerprint::convertFromHex(hash);
    } catch(std::invalid_argument e) {
        SILOG(transfer, error, "Hash header didn't contain a valid Fingerprint string");
        callback(bad);
        return;
    }

    std::istringstream istream(file_size_str);
    uint64 file_size;
    istream >> file_size;
    std::ostringstream ostream;
    ostream << file_size;
    if(ostream.str() != file_size_str) {
        SILOG(transfer, error, "Error converting File-Size header string to integer");
        callback(bad);
        return;
    }

    //Just treat everything as a single chunk for now
    Range whole(0, file_size, LENGTH, true);
    Chunk chunk(fp, whole);
    ChunkList chunkList;
    chunkList.push_back(chunk);

    std::tr1::shared_ptr<RemoteFileMetadata> met(new RemoteFileMetadata(fp, request->getURI(),
            file_size, chunkList, response->getHeaders()));

    callback(met);
    SILOG(transfer, debug, "done http name handler request_finished");
}

HttpChunkHandler::HttpChunkHandler()
    : mCdnAddr(CDN_HOST_NAME, CDN_SERVICE) {

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

    std::ostringstream request_stream;
    bool chunkReq = false;
    request_stream << "GET /files/global/" << file->getFingerprint().convertToHexString() << " HTTP/1.1\r\n";
    if(!chunk->getRange().goesToEndOfFile() && chunk->getRange().size() < file->getSize()) {
        chunkReq = true;
        request_stream << "Range: bytes=" << chunk->getRange().startbyte() << "-" << chunk->getRange().endbyte() << "\r\n";
    }
    request_stream << "Host: " << CDN_HOST_NAME << "\r\n";
    request_stream << "Accept: * /*\r\n\r\n";
    //TODO: Uncomment this. Breaks things.
    //request_stream << "Accept-Encoding: deflate, gzip\r\n\r\n";

    HttpManager::getSingleton().makeRequest(mCdnAddr, Transfer::HttpManager::GET, request_stream.str(), std::tr1::bind(
            &HttpChunkHandler::request_finished, this, _1, _2, _3, file, chunk, chunkReq, callback));
}

void HttpChunkHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        std::tr1::shared_ptr<RemoteFileMetadata> file, std::tr1::shared_ptr<Chunk> chunk,
        bool chunkReq, ChunkCallback callback) {

    std::tr1::shared_ptr<DenseData> bad;
    std::string reqType = "file request";
    if(chunkReq) reqType = "chunk request";

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP " << reqType);
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP " << reqType);
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP " << reqType << ". Boost error = " << boost_error.message());
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP " << reqType);
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP " << reqType);
        callback(bad);
        return;
    }

    std::map<std::string, std::string>::const_iterator it;
    it = response->getHeaders().find("Content-Length");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Content-Length header was not present when it should be during an HTTP " << reqType);
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP " << reqType);
        callback(bad);
        return;
    }

    if (!response->getData()) {
        SILOG(transfer, error, "Body not present during an HTTP " << reqType);
        callback(bad);
        return;
    }

    it = response->getHeaders().find("Range");
    if (chunkReq && it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected Range header not present during an HTTP " << reqType);
        callback(bad);
        return;
    } else if (chunkReq) {
        std::string range_str = it->second;
        bool range_parsed = false;

        if (range_str.substr(0,6) == "bytes=") {
            range_str = range_str.substr(6);
            size_type dashPos = range_str.find('-');
            if (dashPos != std::string::npos) {
                range_str.replace(dashPos, 1, " ");
                std::istringstream instream(range_str);
                uint64 range_start;
                uint64 range_end;
                instream >> range_start;
                instream >> range_end;

                if (range_start == chunk->getRange().startbyte() &&
                        range_end == chunk->getRange().endbyte() &&
                        response->getData()->length() == chunk->getRange().length()) {
                    range_parsed = true;
                }
            }
        }

        if(!range_parsed) {
            SILOG(transfer, error, "Range header has invalid format during an HTTP " << reqType << " header='" << it->second << "'");
            callback(bad);
            return;
        }
    } else if (!chunkReq && response->getData()->length() != file->getSize()) {
        SILOG(transfer, error, "Data retrieved not expected size during an HTTP "
                << reqType << " response=" << response->getData()->length() << " expected=" << file->getSize());
        callback(bad);
        return;
    }

    callback(response->getData());
    SILOG(transfer, debug, "done http chunk handler request_finished");
}

}
}
