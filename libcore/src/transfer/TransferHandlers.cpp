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

HttpNameHandler::HttpNameHandler()
    : mCdnAddr(CDN_HOST_NAME, CDN_SERVICE) {

}

HttpNameHandler::~HttpNameHandler() {

}

void HttpNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {
    std::ostringstream request_stream;
    request_stream << "HEAD /dns/global" << request->getURI().fullpath() << " HTTP/1.1\r\n";
    request_stream << "Host: " << CDN_HOST_NAME << "\r\n";
    request_stream << "Accept: * /*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    HttpManager::getSingleton().makeRequest(mCdnAddr, request_stream.str(), std::tr1::bind(
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
    Range whole(true);
    Chunk chunk(fp, whole);
    ChunkList chunkList;
    chunkList.push_back(chunk);

    std::tr1::shared_ptr<RemoteFileMetadata> met(new RemoteFileMetadata(fp, request->getURI(),
            file_size, chunkList, response->getHeaders()));

    callback(met);
    SILOG(transfer, debug, "done transferhandlers request_finished");
}

}
}
