/*  Sirikata Transfer -- Content Transfer management system
 *  HttpManager.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/transfer/HttpManager.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::HttpManager);

namespace Sirikata {
namespace Transfer {

HttpManager& HttpManager::getSingleton() {
    return AutoSingleton<HttpManager>::getSingleton();
}
void HttpManager::destroy() {
    AutoSingleton<HttpManager>::destroy();
}

std::tr1::shared_ptr<HttpManager::HttpResponse> HttpManager::mTempResponse =
        std::tr1::shared_ptr<HttpManager::HttpResponse>();
std::string HttpManager::mTempHeaderName = "";

HttpManager::HttpManager()
    : numOutstanding(0) {

    EMPTY_PARSER_SETTINGS.on_message_begin = 0;
    EMPTY_PARSER_SETTINGS.on_header_field = 0;
    EMPTY_PARSER_SETTINGS.on_header_value = 0;
    EMPTY_PARSER_SETTINGS.on_path = 0;
    EMPTY_PARSER_SETTINGS.on_url = 0;
    EMPTY_PARSER_SETTINGS.on_fragment = 0;
    EMPTY_PARSER_SETTINGS.on_query_string = 0;
    EMPTY_PARSER_SETTINGS.on_body = 0;
    EMPTY_PARSER_SETTINGS.on_headers_complete = 0;
    EMPTY_PARSER_SETTINGS.on_message_complete = 0;

    //Making a single thread IOService to handle requests
    mServicePool = new IOServicePool(1);

    //Add a dummy IOWork so that the IOService stays running
    mIOWork = new IOWork(*(mServicePool->service()));

    //This runs an IOService in a new thread
    mServicePool->run();

    //Used to resolve host:port names to IP addresses
    mResolver = new TCPResolver(*(mServicePool->service()));
}

HttpManager::~HttpManager() {
    //Cancel any pending requests to resolve names and delete resolver
    mResolver->cancel();
    delete mResolver;

    //Stop the IOService and make sure its thread exist
    mServicePool->service()->stop();
    mServicePool->join();

    //Delete dummy worker and service pool
    delete mIOWork;
    delete mServicePool;
}

void HttpManager::makeRequest(Sirikata::Network::Address addr, std::string req, HttpCallback cb) {

    http_parser_init(&mHttpParser, HTTP_REQUEST);
    size_t nparsed = http_parser_execute(&mHttpParser, &EMPTY_PARSER_SETTINGS, req.c_str(), req.length());
    if(nparsed != req.length()) {
        SILOG(transfer, warning, "Parsing http request failed");
        boost::system::error_code ec;
        cb(std::tr1::shared_ptr<HttpResponse>(), REQUEST_PARSING_FAILED, ec);
    }

    HttpRequest r(addr, req, cb);
    mRequestQueue.push(r);
    processQueue();
}

void HttpManager::processQueue() {
    SILOG(transfer, debug, "processQueue called, numOutstanding = "
            << numOutstanding << " and request queue size = " << mRequestQueue.size());
    if(numOutstanding < MAX_CONCURRENT_REQUESTS && !mRequestQueue.empty()) {
        HttpRequest r = mRequestQueue.front();
        mRequestQueue.pop();
        numOutstanding++;
        TCPResolver::query query(r.addr.getHostName(), r.addr.getService());
        mResolver->async_resolve(query, boost::bind(&HttpManager::handle_resolve, this, r,
                                boost::asio::placeholders::error, boost::asio::placeholders::iterator));
    }
}

void HttpManager::handle_resolve(HttpRequest req, const boost::system::error_code& err,
        TCPResolver::iterator endpoint_iterator) {
    if (!err) {
        TCPEndPoint endpoint = *endpoint_iterator;
        std::tr1::shared_ptr<TCPSocket> socket(new TCPSocket(*(mServicePool->service())));
        socket->async_connect(endpoint, boost::bind(
                &HttpManager::handle_connect, this, socket, req,
                boost::asio::placeholders::error, ++endpoint_iterator));
    } else {
        SILOG(transfer, error, "Failed to resolve hostname. Error = " << err.message());
        req.cb(std::tr1::shared_ptr<HttpResponse>(), BOOST_ERROR, err);
        numOutstanding--;
        processQueue();
    }
}

void HttpManager::handle_connect(std::tr1::shared_ptr<TCPSocket> socket, HttpRequest req,
        const boost::system::error_code& err, TCPResolver::iterator endpoint_iterator) {
    if (!err) {
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << req.req;
        boost::asio::async_write(*socket, request, boost::bind(
                &HttpManager::handle_write_request, this, socket, req,
                boost::asio::placeholders::error));
    } else if (endpoint_iterator != TCPResolver::iterator()) {
        socket->close();
        TCPEndPoint endpoint = *endpoint_iterator;
        socket->async_connect(endpoint, boost::bind(
                &HttpManager::handle_connect, this, socket, req,
                boost::asio::placeholders::error, ++endpoint_iterator));
    } else {
        SILOG(transfer, error, "Failed to connect. Error = " << err.message());
        req.cb(std::tr1::shared_ptr<HttpResponse>(), BOOST_ERROR, err);
        numOutstanding--;
        processQueue();
    }
}

void HttpManager::handle_write_request(std::tr1::shared_ptr<TCPSocket> socket, HttpRequest req,
        const boost::system::error_code& err) {
    if (!err) {
        std::tr1::shared_ptr<boost::asio::streambuf> response(new boost::asio::streambuf());
        boost::asio::async_read(*socket, *response, boost::bind(
                &HttpManager::handle_read, this, socket, req, response,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    } else {
        socket->close();
        SILOG(transfer, error, "Failed to write. Error = " << err.message());
        req.cb(std::tr1::shared_ptr<HttpResponse>(), BOOST_ERROR, err);
        numOutstanding--;
        processQueue();
    }
}

void HttpManager::handle_read(std::tr1::shared_ptr<TCPSocket> socket, HttpRequest req,
        std::tr1::shared_ptr<boost::asio::streambuf> response, const boost::system::error_code& err,
        std::size_t bytes_transferred) {
    if (!err || err == boost::asio::error::eof) {
        const char* resp_str = boost::asio::buffer_cast<const char*>(response->data());

        mHttpSettings = EMPTY_PARSER_SETTINGS;
        mHttpSettings.on_header_field = &HttpManager::on_header_field;
        mHttpSettings.on_header_value = &HttpManager::on_header_value;
        mHttpSettings.on_body = &HttpManager::on_body;

        http_parser_init(&mHttpParser, HTTP_RESPONSE);

        std::tr1::shared_ptr<HttpResponse> respObj(new HttpResponse());
        mTempResponse = respObj;
        size_t responseSize = response->size();

        size_t nparsed = http_parser_execute(&mHttpParser, &mHttpSettings, resp_str, responseSize);
        boost::system::error_code ec;
        if(nparsed != responseSize) {
            SILOG(transfer, warning, "Failed to parse http response. nparsed=" << nparsed << " while responsesize=" << responseSize);
            req.cb(std::tr1::shared_ptr<HttpResponse>(), RESPONSE_PARSING_FAILED, ec);
        } else {
            SILOG(transfer, debug, "Finished http transfer");
            mTempResponse->mContentLength = mHttpParser.content_length;
            mTempResponse->mStatusCode = mHttpParser.status_code;
            req.cb(mTempResponse, SUCCESS, ec);
        }
    } else {
        SILOG(transfer, error, "Failed to write. Error = " << err.message());
        req.cb(std::tr1::shared_ptr<HttpResponse>(), BOOST_ERROR, err);
    }

    socket->close();
    numOutstanding--;
    processQueue();
}

int HttpManager::on_header_field(http_parser *_, const char *at, size_t len) {
    //SILOG(transfer, debug, "on_header_field called");
    mTempHeaderName.clear();
    mTempHeaderName.append(at, len);
    return 0;
}

int HttpManager::on_header_value(http_parser *_, const char *at, size_t len) {
    //SILOG(transfer, debug, "on_header_value called");
    std::string val(at, len);
    mTempResponse->mHeaders[mTempHeaderName] = val;
    return 0;
}

int HttpManager::on_body(http_parser *_, const char *at, size_t len) {
    //SILOG(transfer, debug, "on_body called");
    std::tr1::shared_ptr<DenseData> d(new DenseData(Range(true), at, len));
    mTempResponse->mData = d;
    return 0;
}

}
}
