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
    mServicePool = new IOServicePool(2);

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

void HttpManager::postCallback(IOCallback cb) {
    mServicePool->service()->post(cb);
}

void HttpManager::makeRequest(Sirikata::Network::Address addr, HTTP_METHOD method, std::string req, HttpCallback cb) {

    http_parser reqParser;
    http_parser_init(&reqParser, HTTP_REQUEST);
    size_t nparsed = http_parser_execute(&reqParser, &EMPTY_PARSER_SETTINGS, req.c_str(), req.length());
    if (nparsed != req.length()) {
        SILOG(transfer, warning, "Parsing http request failed");
        boost::system::error_code ec;
        postCallback(std::tr1::bind(cb, std::tr1::shared_ptr<HttpResponse>(), REQUEST_PARSING_FAILED, ec));
        return;
    }

    std::tr1::shared_ptr<HttpRequest> r(new HttpRequest(addr, req, method, cb));
    add_req(r);
    processQueue();
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
                        TCPResolver::query query((*req)->addr.getHostName(), (*req)->addr.getService());
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
            (req->method == GET && respPtr->mMessageComplete)) {
        //We're done

        //If we didn't get any body data, erase the DenseData pointer
        if (respPtr->mData->length() == 0) {
            respPtr->mData.reset();
        }

        SILOG(transfer, detailed, "Finished http transfer with content length of " << respPtr->getContentLength());
        req->cb(respPtr, SUCCESS, ec);

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
    if (curResponse->mLastCallback == HttpResponse::VALUE) {
        curResponse->mHeaders[curResponse->mTempHeaderField] = curResponse->mTempHeaderValue;
    }

    //Check if Content-Encoding = gzip
    std::map<std::string, std::string>::const_iterator it = curResponse->mHeaders.find("Content-Encoding");
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
        case HttpResponse::VALUE:
            //Previous header name/value is finished, so save
            curResponse->mHeaders[curResponse->mTempHeaderField] = curResponse->mTempHeaderValue;
            //Then continue on to the none case to make new values:
        case HttpResponse::NONE:
            //Clear strings and save header name
            curResponse->mTempHeaderField.clear();
            //Continue on to append:
        case HttpResponse::FIELD:
            //Field was called twice in a row, so append new data to previous name
            curResponse->mTempHeaderField.append(at, len);
            break;
    }

    curResponse->mLastCallback = curResponse->FIELD;
    return 0;
}

int HttpManager::on_header_value(http_parser* _, const char* at, size_t len) {
    //SILOG(transfer, debug, "on_header_value called");
    HttpResponse* curResponse = static_cast<HttpResponse*>(_->data);

    //See http-parser documentation for why this is necessary
    switch(curResponse->mLastCallback) {
        case HttpResponse::FIELD:
            //Field is finished, this is a new value so clear
            curResponse->mTempHeaderValue.clear();
            //Continue on to append data:
        case HttpResponse::VALUE:
            //May be a continued header value, so append
            curResponse->mTempHeaderValue.append(at, len);
            break;
        case HttpResponse::NONE:
            //Shouldn't happen
            assert(false);
            break;
    }

    curResponse->mLastCallback = curResponse->VALUE;
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
