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
#include <sirikata/core/transfer/URL.hpp>
#include <sirikata/core/network/Address.hpp>
#include <liboauthcpp/liboauthcpp.h>

#include <boost/lexical_cast.hpp>
#include <sirikata/core/util/UUID.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::HttpManager);

namespace Sirikata {
namespace Transfer {

HttpManager& HttpManager::getSingleton() {
    return AutoSingleton<HttpManager>::getSingleton();
}
void HttpManager::destroy() {
    AutoSingleton<HttpManager>::destroy();
}

HttpManager::HttpManager()
    : mNumTotalConnections(0) {

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
    mServicePool = new IOServicePool("HttpManager", 2);

    //Add a dummy IOWork so that the IOService stays running
    mServicePool->startWork();

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
    mServicePool->join();

    //Delete dummy worker and service pool
    mServicePool->stopWork();
    delete mServicePool;
}

void HttpManager::postCallback(IOCallback cb, const char* tag) {
    mServicePool->service()->post(cb, tag);
}

void HttpManager::postCallback(const Duration& waitFor, IOCallback cb, const char* tag) {
    mServicePool->service()->post(waitFor, cb, tag);
}

String HttpManager::methodAsString(HTTP_METHOD m) {
    switch(m) {
      case HEAD: return "HEAD";
      case GET: return "GET";
      case POST: return "POST";
      default: return "";
    }
}

void HttpManager::makeRequest(Sirikata::Network::Address addr, HTTP_METHOD method, std::string req, bool allow_redirects, HttpCallback cb) {

    std::tr1::shared_ptr<HttpRequest> r(new HttpRequest(addr, req, method, allow_redirects, cb));

    //Initialize http parser settings callbacks
    r->mHttpSettings = EMPTY_PARSER_SETTINGS;
    r->mHttpSettings.on_header_field = &HttpManager::on_request_header_field;
    r->mHttpSettings.on_header_value = &HttpManager::on_request_header_value;
    r->mHttpSettings.on_headers_complete = &HttpManager::on_request_headers_complete;

    //Initialize the parser for parsing a response
    http_parser_init(&(r->mHttpParser), HTTP_REQUEST);

    /*
     * http-parser library uses this void * parameter to callbacks for user-defined data
     * Store a pointer to the HttpRequest object so we can access it during static callbacks
     */
    r->mHttpParser.data = static_cast<void *>(r.get());

    size_t nparsed = http_parser_execute(&(r->mHttpParser), &(r->mHttpSettings), req.c_str(), req.length());
    if (nparsed != req.length()) {
        SILOG(transfer, warning, "Parsing http request failed");
        boost::system::error_code ec;
        postCallback(
            std::tr1::bind(cb, std::tr1::shared_ptr<HttpResponse>(), REQUEST_PARSING_FAILED, ec),
            "HttpManager::makeRequest callback"
        );
        return;
    }

    add_req(r);
    processQueue();
}

void HttpManager::makeRequest(
    Sirikata::Network::Address addr, HTTP_METHOD method, const String& path,
    HttpCallback cb,
    const Headers& headers, const QueryParameters& query_params,
    const String& body,
    bool allow_redirects)
{
    std::ostringstream request_stream;

    // Request line
    request_stream << methodAsString(method) << " ";
    formatPath(request_stream, path, query_params);
    request_stream << " HTTP/1.1\r\n";

    // Headers
    for(Headers::const_iterator it = headers.begin(); it != headers.end(); it++)
        request_stream << it->first << ": " << it->second << "\r\n";
    if (headers.find("Accept") == headers.end())
        request_stream << "Accept: */*\r\n";

    // Required blank line
    request_stream << "\r\n";

    // Body
    if (!body.empty())
        request_stream << body;

    // FIXME This is actually kind of round-about as we are formatting and then
    // reparsing the request by going through the other makeRequest call. We
    // could dispatch this ourselves and not waste the time reparsing.
    makeRequest(addr, method, request_stream.str(), allow_redirects, cb);
}

void HttpManager::formatURLEncodedDictionary(std::ostream& os, const StringDictionary& query_params) {
    bool first_param = true;
    for(QueryParameters::const_iterator it = query_params.begin(); it != query_params.end(); it++) {
        if (!first_param) os << "&";
        os << OAuth::URLEncode(it->first) << "=" << OAuth::URLEncode(it->second);
        first_param = false;
    }
}

String HttpManager::formatURLEncodedDictionary(const StringDictionary& query_params) {
    std::ostringstream formatted;
    formatURLEncodedDictionary(formatted, query_params);
    return formatted.str();
}

void HttpManager::formatPath(std::ostream& os, const String& path, const QueryParameters& query_params) {
    os << path;
    if (!query_params.empty()) {
        os << "?";
        formatURLEncodedDictionary(os, query_params);
    }

}

String HttpManager::formatPath(const String& path, const QueryParameters& query_params) {
    std::ostringstream formatted;
    formatPath(formatted, path, query_params);
    return formatted.str();
}

String HttpManager::formatURL(const String& host, const String& service, const String& path, const QueryParameters& query_params) {
    String service_part = "";
    // FIXME sanity check service is a port number?
    if (!service.empty() && service != "http" && service != "80")
        service_part = ":" + service;
    return "http://" + host + service_part + formatPath(path, query_params);
}

void HttpManager::head(
    Sirikata::Network::Address addr, const String& path,
    HttpCallback cb, const Headers& headers, const QueryParameters& query_params, bool allow_redirects)
{
    makeRequest(addr, HEAD, path, cb, headers, query_params, "", allow_redirects);
}

void HttpManager::get(
    Sirikata::Network::Address addr, const String& path,
    HttpCallback cb, const Headers& headers, const QueryParameters& query_params, bool allow_redirects)
{
    makeRequest(addr, GET, path, cb, headers, query_params, "", allow_redirects);
}

void HttpManager::post(
    Sirikata::Network::Address addr, const String& path,
    const String& content_type, const String& body,
    HttpCallback cb, const Headers& headers, const QueryParameters& query_params,
    bool allow_redirects)
{
    Headers new_headers(headers);

    if (!body.empty()) {
        if (new_headers.find("Content-Type") == new_headers.end())
            new_headers["Content-Type"] = content_type;
        if (new_headers.find("Content-Length") == new_headers.end())
            new_headers["Content-Length"] = boost::lexical_cast<String>(body.size());
    }

    makeRequest(addr, POST, path, cb, new_headers, query_params, body, allow_redirects);
}

void HttpManager::postURLEncoded(
    Sirikata::Network::Address addr, const String& path,
    const StringDictionary& body,
    HttpCallback cb, const Headers& headers, const QueryParameters& query_params,
    bool allow_redirects)
{
    post(
        addr, path, "application/x-www-form-urlencoded", formatURLEncodedDictionary(body),
        cb, headers, query_params, allow_redirects
    );
}

void HttpManager::postMultipartForm(
    Sirikata::Network::Address addr, const String& path,
    const MultipartDataList& data,
    HttpCallback cb, const Headers& headers, const QueryParameters& query_params,
    bool allow_redirects)
{
    String boundary = UUID::random().rawHexData();
    String content_type = "multipart/form-data; boundary=" + boundary;

    std::ostringstream request_body;
    for(MultipartDataList::const_iterator it = data.begin(); it != data.end(); it++) {
        const MultipartData& mp = *it;

        request_body << "--" << boundary << "\r\n";
        // Content disposition line
        request_body << "Content-Disposition: form-data; name=\"" << mp.field << "\"";
        if (!mp.filename.empty()) {
            request_body << "; filename=\"" << mp.filename << "\"";
        }
        request_body << "\r\n";
        // Other headers
        for(Headers::const_iterator head_it = mp.headers.begin(); head_it != mp.headers.end(); head_it++)
            request_body << head_it->first << ": " << head_it->second << "\r\n";
        // Default content-type header for files
        if (!mp.filename.empty() && mp.headers.find("Content-Type") == mp.headers.end())
            request_body << "Content-Type: application/octet-stream\r\n";
        if (!mp.filename.empty() && mp.headers.find("Content-Transfer-Encoding") == mp.headers.end())
            request_body << "Content-Transfer-Encoding: binary\r\n";

        // Data
        request_body << "\r\n";
        request_body << mp.data;
        request_body << "\r\n";
    }
    request_body << "--" << boundary << "--\r\n";

    // Perform the post
    post(
        addr, path, content_type, request_body.str(),
        cb, headers, query_params, allow_redirects
    );
}



void HttpManager::processQueue() {
    SILOG(transfer, insane, "processQueue called, mNumTotalConnections = "
            << mNumTotalConnections << " and recycle bin size = " << mRecycleBin.size()
            << " and size of hosts = " << mNumConnsPerAddr.size()
            << " and request queue size = " << mRequestQueue.size());

    boost::unique_lock<boost::mutex> lockQueue(mRequestQueueLock);
    boost::unique_lock<boost::mutex> lockNumConns(mNumConnsLock, boost::defer_lock);

    for (RequestQueueType::iterator req = mRequestQueue.begin(); req != mRequestQueue.end(); ) {

        std::tr1::shared_ptr<TCPSocket> sock;

        //First check the recycle bin to see if there's a connection already open we can use
        boost::unique_lock<boost::mutex> lockRB(mRecycleBinLock); {
            RecycleBinType::iterator findRec = mRecycleBin.find((*req)->addr);
            if (findRec != mRecycleBin.end()) {
                sock = findRec->second.front();
                findRec->second.pop();
                if (findRec->second.size() == 0) {
                    mRecycleBin.erase(findRec);
                }
            }
        }
        lockRB.unlock();

        if (sock) {
            //SILOG(transfer, debug, "Reusing a connection for " << (*req)->addr.toString());
            write_request(sock, *req);
            req = mRequestQueue.erase(req);
        } else {

            lockNumConns.lock(); {
                //If nothing in the recycle bin, let's see if we can open a new connection
                if (mNumTotalConnections < MAX_TOTAL_CONNECTIONS) {
                    NumConnsType::iterator findNumC = mNumConnsPerAddr.find((*req)->addr);
                    if (mNumTotalConnections < MAX_TOTAL_CONNECTIONS &&
                            (findNumC == mNumConnsPerAddr.end() || findNumC->second < MAX_CONNECTIONS_PER_ENDPOINT)) {

                        //We are safe to open a new connection, but increase counts first
                        mNumTotalConnections++;
                        if (findNumC == mNumConnsPerAddr.end()) {
                           mNumConnsPerAddr[(*req)->addr] = 1;
                        } else {
                            mNumConnsPerAddr[(*req)->addr] = findNumC->second + 1;
                        }

                        //SILOG(transfer, debug, "Creating a new connection for " << (*req)->addr.toString());
                        TCPResolver::query query((*req)->addr.getHostName(), (*req)->addr.getService(), Network::TCPResolver::query::all_matching);
                        mResolver->async_resolve(query, boost::bind(&HttpManager::handle_resolve, this, *req,
                                                boost::asio::placeholders::error, boost::asio::placeholders::iterator));

                        req = mRequestQueue.erase(req);
                    } else {
                        //No available recycled connections, can't open a new one, so do nothing
                        req++;
                    }
                } else {
                    //No available recycled connections, can't open a new one, so do nothing
                    req++;
                }
            } lockNumConns.unlock();
        }
    }

    lockQueue.unlock();
}

void HttpManager::decrement_connection(const Sirikata::Network::Address& addr) {
    //SILOG(transfer, debug, "Reducing number of connections by 1 for " << addr.toString());
    boost::unique_lock<boost::mutex> lockNumConns(mNumConnsLock); {
        mNumTotalConnections--;
        NumConnsType::iterator findNumC = mNumConnsPerAddr.find(addr);
        if (findNumC != mNumConnsPerAddr.end()) {
            if (findNumC->second == 1) {
                mNumConnsPerAddr.erase(findNumC);
            } else {
                mNumConnsPerAddr[addr] = findNumC->second - 1;
            }
        }
    }
    lockNumConns.unlock();
}

void HttpManager::add_req(std::tr1::shared_ptr<HttpRequest> req) {
    boost::unique_lock<boost::mutex> lockQueue(mRequestQueueLock);
    mRequestQueue.push_back(req);
    lockQueue.unlock();
}

void HttpManager::handle_resolve(std::tr1::shared_ptr<HttpRequest> req, const boost::system::error_code& err,
        TCPResolver::iterator endpoint_iterator) {
    if (!err) {
        TCPEndPoint endpoint = *endpoint_iterator;
        std::tr1::shared_ptr<TCPSocket> socket(new TCPSocket(*(mServicePool->service())));
        socket->async_connect(endpoint, boost::bind(
                &HttpManager::handle_connect, this, socket, req,
                boost::asio::placeholders::error, ++endpoint_iterator));
    } else {
        SILOG(transfer, error, "Failed to resolve hostname. Error = " << err.message());
        decrement_connection(req->addr);

        req->mNumTries++;
        if (req->mNumTries > 10) {
            //This means this connection has gotten an error over 10 times. Let's stop trying
            //TODO: this should probably be configurable
            req->cb(std::tr1::shared_ptr<HttpResponse>(), BOOST_ERROR, boost::asio::error::host_not_found);
        } else {
            add_req(req);
        }

        add_req(req);
        processQueue();
    }
}

void HttpManager::write_request(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req) {
    std::tr1::shared_ptr<boost::asio::streambuf> request_ptr(new boost::asio::streambuf());
    std::ostream request_stream(request_ptr.get());
    request_stream << req->req;
    boost::asio::async_write(*socket, *request_ptr, boost::bind(
            &HttpManager::handle_write_request, this, socket, req,
            boost::asio::placeholders::error, request_ptr));
}

void HttpManager::handle_connect(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req,
        const boost::system::error_code& err, TCPResolver::iterator endpoint_iterator) {
    if (!err) {
        write_request(socket, req);
    } else if (endpoint_iterator != TCPResolver::iterator()) {
        socket->close();
        TCPEndPoint endpoint = *endpoint_iterator;
        socket->async_connect(endpoint, boost::bind(
                &HttpManager::handle_connect, this, socket, req,
                boost::asio::placeholders::error, ++endpoint_iterator));
    } else {
        SILOG(transfer, error, "Failed to connect. Error = " << err.message());
        decrement_connection(req->addr);

        req->mNumTries++;
        if (req->mNumTries > 10) {
            //This means this connection has gotten an error over 10 times. Let's stop trying
            //TODO: this should probably be configurable
            req->cb(std::tr1::shared_ptr<HttpResponse>(), BOOST_ERROR, boost::asio::error::host_unreachable);
        } else {
            add_req(req);
        }

        processQueue();
    }
}

void HttpManager::handle_write_request(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req,
        const boost::system::error_code& err, std::tr1::shared_ptr<boost::asio::streambuf> request_stream) {

    if (err) {
        socket->close();
        SILOG(transfer, error, "Failed to write. Error = " << err.message());
        decrement_connection(req->addr);
        add_req(req);
        processQueue();
        return;
    }

    //Create a new response object
    std::tr1::shared_ptr<HttpResponse> respPtr(new HttpResponse());

    //Initiate an empty DenseData
    std::tr1::shared_ptr<DenseData> emptyData(new DenseData(Range(true)));
    respPtr->mData = emptyData;

    //Initialize http parser settings callbacks
    respPtr->mHttpSettings = EMPTY_PARSER_SETTINGS;
    respPtr->mHttpSettings.on_header_field = &HttpManager::on_header_field;
    respPtr->mHttpSettings.on_header_value = &HttpManager::on_header_value;
    respPtr->mHttpSettings.on_body = &HttpManager::on_body;
    respPtr->mHttpSettings.on_headers_complete = &HttpManager::on_headers_complete;
    respPtr->mHttpSettings.on_message_complete = &HttpManager::on_message_complete;

    //Initialize the parser for parsing a response
    http_parser_init(&(respPtr->mHttpParser), HTTP_RESPONSE);

    /*
     * http-parser library uses this void * parameter to callbacks for user-defined data
     * Store a pointer to the HttpResponse object so we can access it during static callbacks
     */
    respPtr->mHttpParser.data = static_cast<void *>(respPtr.get());

    //Create a buffer for receiving from socket
    std::tr1::shared_ptr<std::vector<unsigned char> > vecbuf(new std::vector<unsigned char>(SOCKET_BUFFER_SIZE));

    //Read some data
    socket->async_read_some(boost::asio::buffer(*vecbuf), boost::bind(
            &HttpManager::handle_read, this, socket, req, vecbuf, respPtr,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void HttpManager::handle_read(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req,
        std::tr1::shared_ptr<std::vector<unsigned char> > vecbuf, std::tr1::shared_ptr<HttpResponse> respPtr,
        const boost::system::error_code& err, std::size_t bytes_transferred) {

    SILOG(transfer, insane, "handle_read triggered with bytes_transferred = " << bytes_transferred << " EOF? "
            << (err == boost::asio::error::eof ? "Y" : "N"));

    if ((err || bytes_transferred == 0) && err != boost::asio::error::eof) {
        SILOG(transfer, error, "Failed to write. Error = " << err.message());
        socket->close();
        decrement_connection(req->addr);
        add_req(req);
        processQueue();
        return;
    }

    //Parse the data we just got back from the socket
    size_t nparsed = http_parser_execute(&(respPtr->mHttpParser), &(respPtr->mHttpSettings),
            (const char *)(&((*vecbuf)[0])), bytes_transferred);

    boost::system::error_code ec;
    if (nparsed != bytes_transferred) {
        SILOG(transfer, warning, "Failed to parse http response. nparsed=" << nparsed << " while bytes_transferred=" << bytes_transferred);
        req->cb(std::tr1::shared_ptr<HttpResponse>(), RESPONSE_PARSING_FAILED, ec);
        socket->close();
        decrement_connection(req->addr);
        processQueue();
        return;
    }

    if (err == boost::asio::error::eof && bytes_transferred != 0) {
        //Pass 0 as fourth parameter to parser to tell it that we got EOF
        size_t nparsed = http_parser_execute(&(respPtr->mHttpParser), &(respPtr->mHttpSettings),
                (const char *)(&((*vecbuf)[0])), 0);
        if (nparsed != 0) {
            SILOG(transfer, warning, "Failed to parse http response when giving EOF. nparsed=" << nparsed);
            req->cb(std::tr1::shared_ptr<HttpResponse>(), RESPONSE_PARSING_FAILED, ec);
            socket->close();
            decrement_connection(req->addr);
            processQueue();
            return;
        }

    }

    if ((req->method == HEAD && respPtr->mHeaderComplete) ||
        ((req->method == GET || req->method == POST) && respPtr->mMessageComplete)) {
        //We're done

        //If we didn't get any body data, erase the DenseData pointer
        if (respPtr->mData->length() == 0) {
            respPtr->mData.reset();
        }

        SILOG(transfer, detailed, "Finished http transfer with content length of " << respPtr->getContentLength());
        Headers::const_iterator findLocation;
        findLocation = respPtr->mHeaders.find("Location");
        if (respPtr->getStatusCode() == 301 && findLocation != respPtr->mHeaders.end() && req->allow_redirects) {
            SILOG(transfer, detailed, "Got a 301 redirect reply and location = " << findLocation->second);
            std::ostringstream request_stream;
            std::string request_method = methodAsString(req->method);
            URL newURI(findLocation->second.c_str());
            request_stream << request_method << " " << newURI.fullpath() << " HTTP/1.1\r\n";
            Headers::const_iterator it;
            for (it = req->mHeaders.begin(); it != req->mHeaders.end(); it++) {
            	if (it->first == "Host") {
            		request_stream << "Host: " << newURI.host() << "\r\n";
            	} else {
            		request_stream << it->first << ": " << it->second << "\r\n";
            	}
            }
            request_stream << "\r\n";
            Network::Address newaddr(newURI.host(), newURI.proto());
            makeRequest(newaddr, req->method, request_stream.str(), req->allow_redirects, req->cb);
        } else {
            req->cb(respPtr, SUCCESS, ec);
        }

        //If this is Connection: Close or we reached EOF, then close connection, otherwise recycle
        if ((respPtr->mHttpParser.flags & F_CONNECTION_CLOSE) || err == boost::asio::error::eof) {
            socket->close();
            decrement_connection(req->addr);
        } else {
            boost::unique_lock<boost::mutex> lockRB(mRecycleBinLock);
            mRecycleBin[req->addr].push(socket);
            lockRB.unlock();
        }
        processQueue();

    } else if(err == boost::asio::error::eof || bytes_transferred == 0) {
        SILOG(transfer, warning, "EOF was true or bytes_transferred==0 and the parser wasn't finished, so connection is broken");
        socket->close();
        decrement_connection(req->addr);

        req->mNumTries++;
        if (req->mNumTries > 10) {
            //This means this connection has gotten an error over 10 times. Let's stop trying
            //TODO: this should probably be configurable
            req->cb(std::tr1::shared_ptr<HttpResponse>(), BOOST_ERROR, boost::asio::error::eof);
        } else {
            add_req(req);
        }

        processQueue();
        return;
    } else {
        //Read some more data
        socket->async_read_some(boost::asio::buffer(*vecbuf), boost::bind(
                &HttpManager::handle_read, this, socket, req, vecbuf, respPtr,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

}

int HttpManager::on_headers_complete(http_parser* _) {
    //SILOG(transfer, debug, "headers complete. content length = " << _->content_length);
    HttpResponse* curResponse = static_cast<HttpResponse*>(_->data);
    curResponse->mContentLength = _->content_length;
    curResponse->mStatusCode = _->status_code;

    //Check for last header that might not have been saved
    if (curResponse->mLastCallback == VALUE) {
        curResponse->mHeaders[curResponse->mTempHeaderField] = curResponse->mTempHeaderValue;
    }

    //Check if Content-Encoding = gzip
    Headers::const_iterator it = curResponse->mHeaders.find("Content-Encoding");
    if(it != curResponse->mHeaders.end() && it->second == "gzip") {
        curResponse->mGzip = true;
    }

    curResponse->mHeaderComplete = true;
    return 0;
}

int HttpManager::on_header_field(http_parser* _, const char* at, size_t len) {
    HttpResponse* curResponse = static_cast<HttpResponse*>(_->data);

    //See http-parser documentation for why this is necessary
    switch (curResponse->mLastCallback) {
        case VALUE:
            //Previous header name/value is finished, so save
            curResponse->mHeaders[curResponse->mTempHeaderField] = curResponse->mTempHeaderValue;
            //Then continue on to the none case to make new values:
        case NONE:
            //Clear strings and save header name
            curResponse->mTempHeaderField.clear();
            //Continue on to append:
        case FIELD:
            //Field was called twice in a row, so append new data to previous name
            curResponse->mTempHeaderField.append(at, len);
            break;
    }

    curResponse->mLastCallback = FIELD;
    return 0;
}

int HttpManager::on_header_value(http_parser* _, const char* at, size_t len) {
    //SILOG(transfer, debug, "on_header_value called");
    HttpResponse* curResponse = static_cast<HttpResponse*>(_->data);

    //See http-parser documentation for why this is necessary
    switch(curResponse->mLastCallback) {
        case FIELD:
            //Field is finished, this is a new value so clear
            curResponse->mTempHeaderValue.clear();
            //Continue on to append data:
        case VALUE:
            //May be a continued header value, so append
            curResponse->mTempHeaderValue.append(at, len);
            break;
        case NONE:
            //Shouldn't happen
            assert(false);
            break;
    }

    curResponse->mLastCallback = VALUE;
    return 0;
}

int HttpManager::on_request_headers_complete(http_parser* _) {
    //SILOG(transfer, debug, "headers complete. content length = " << _->content_length);
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);

    //Check for last header that might not have been saved
    if (curRequest->mLastCallback == VALUE) {
        curRequest->mHeaders[curRequest->mTempHeaderField] = curRequest->mTempHeaderValue;
    }

    curRequest->mHeaderComplete = true;
    return 0;
}

int HttpManager::on_request_header_field(http_parser* _, const char* at, size_t len) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);

    //See http-parser documentation for why this is necessary
    switch (curRequest->mLastCallback) {
        case VALUE:
            //Previous header name/value is finished, so save
            curRequest->mHeaders[curRequest->mTempHeaderField] = curRequest->mTempHeaderValue;
            //Then continue on to the none case to make new values:
        case NONE:
            //Clear strings and save header name
            curRequest->mTempHeaderField.clear();
            //Continue on to append:
        case FIELD:
            //Field was called twice in a row, so append new data to previous name
            curRequest->mTempHeaderField.append(at, len);
            break;
    }

    curRequest->mLastCallback = FIELD;
    return 0;
}

int HttpManager::on_request_header_value(http_parser* _, const char* at, size_t len) {
    //SILOG(transfer, debug, "on_header_value called");
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);

    //See http-parser documentation for why this is necessary
    switch(curRequest->mLastCallback) {
        case FIELD:
            //Field is finished, this is a new value so clear
            curRequest->mTempHeaderValue.clear();
            //Continue on to append data:
        case VALUE:
            //May be a continued header value, so append
            curRequest->mTempHeaderValue.append(at, len);
            break;
        case NONE:
            //Shouldn't happen
            assert(false);
            break;
    }

    curRequest->mLastCallback = VALUE;
    return 0;
}

int HttpManager::on_body(http_parser* _, const char* at, size_t len) {
    //SILOG(transfer, debug, "on_body called with length = " << len);
    HttpResponse* curResponse = static_cast<HttpResponse*>(_->data);

    if(curResponse->mGzip) {
        //Gzip encoding, so pass this buffer through a decoder
        curResponse->mCompressedStream.write(at, len);
    } else {
        //Raw encoding, so append the bytes in current body pointer directly to the DenseData pointer in our response
        curResponse->mData->append(at, len, true);
    }

    return 0;
}

int HttpManager::on_message_complete(http_parser* _) {
    //SILOG(transfer, debug, "message complete. content length = " << _->content_length);
    HttpResponse* curResponse = static_cast<HttpResponse*>(_->data);

    if(curResponse->mGzip) {
        std::stringstream decompressed;
        boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
        out.push(boost::iostreams::gzip_decompressor());
        out.push(curResponse->mCompressedStream);
        boost::iostreams::copy(out, decompressed);
        curResponse->mCompressedStream.str("");
        curResponse->mData->append(decompressed.str().c_str(), decompressed.str().length(), true);
        curResponse->mContentLength = decompressed.str().length();
    }

    curResponse->mMessageComplete = true;
    return 0;
}

void HttpManager::print_flags(std::tr1::shared_ptr<HttpResponse> resp) {
    char flags = resp->mHttpParser.flags;
    SILOG(transfer, detailed, "Flags are: "
            << (flags & F_CHUNKED ? "F_CHUNKED " : "")
            << (flags & F_CONNECTION_KEEP_ALIVE ? "F_CONNECTION_KEEP_ALIVE " : "")
            << (flags & F_CONNECTION_CLOSE ? "F_CONNECTION_CLOSE " : "")
            << (flags & F_TRAILING ? "F_TRAILING " : "")
            << (flags & F_UPGRADE ? "F_UPGRADE " : "")
            << (flags & F_SKIPBODY ? "F_SKIPBODY " : "")
            << (resp->mMessageComplete ? "MESSAGE_COMPLETE " : "")
            << (resp->mHeaderComplete ? "HEADER_COMPLETE " : "")
            );
}

}
}
