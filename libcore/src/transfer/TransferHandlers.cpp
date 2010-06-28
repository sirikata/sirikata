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

namespace Sirikata {
namespace Transfer {

const char HttpNameHandler::CDN_HOST_NAME [] = "cdn.sirikata.com";

HttpNameHandler::HttpNameHandler()
    : mEndPointInitialized(false) {

    //Making a single thread IOService to handle requests
    mServicePool = new IOServicePool(1);

    //Add a dummy IOWork so that the IOService stays running
    mIOWork = new IOWork(*(mServicePool->service()));

    //This runs an IOService in a new thread
    mServicePool->run();

    //Used to resolve host:port names to IP addresses
    mResolver = new TCPResolver(*(mServicePool->service()));

    resolve_cdn();
}

HttpNameHandler::~HttpNameHandler() {
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

void HttpNameHandler::resolve_cdn() {
    //Resolve the CDN host:port
    TCPResolver::query query(CDN_HOST_NAME, CDN_PORT);
    mResolver->async_resolve(query, boost::bind(&HttpNameHandler::handle_resolve, this,
                            boost::asio::placeholders::error, boost::asio::placeholders::iterator));
}

void HttpNameHandler::handle_resolve(const boost::system::error_code& err, TCPResolver::iterator endpoint_iterator) {
    if (!err) {
        mTCPEndpoint = *endpoint_iterator;
        mEndPointInitialized = true;
        processRequests();
    } else {
        SILOG(transfer, error, "Failed to resolve CDN hostname. Error = " << err.message());
        //TODO: is this the best way to handle this?
        resolve_cdn();
    }
}

void HttpNameHandler::handle_connect(const boost::system::error_code& err, ReqCallbackPairType request) {
    if (!err) {

        boost::asio::streambuf reqbuff;
        std::ostream request_stream(&reqbuff);
        request_stream << "GET /dns/global" << request.first->getURI().fullpath() << " HTTP/1.0\r\n";
        request_stream << "Host: " << CDN_HOST_NAME << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        boost::asio::async_write(*mTCPSocket, reqbuff, boost::bind(
                &HttpNameHandler::handle_write_request, this,
                boost::asio::placeholders::error, request));
    } else {
        SILOG(transfer, fatal, "Failed to connect to CDN socket. Error = " << err.message());
    }
}

void HttpNameHandler::handle_write_request(const boost::system::error_code& err, ReqCallbackPairType request) {
    if (!err) {
        std::tr1::shared_ptr<boost::asio::streambuf> buf(new boost::asio::streambuf());
        boost::asio::async_read(*mTCPSocket, *buf, boost::bind(
                &HttpNameHandler::handle_read, this, request, buf,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    } else {
        SILOG(transfer, fatal, "Failed to write to CDN socket. Error = " << err.message());
    }
}

void HttpNameHandler::handle_read(ReqCallbackPairType request, std::tr1::shared_ptr<boost::asio::streambuf> buf,
        const boost::system::error_code& err, std::size_t bytes_transferred) {
    if (!err || err == boost::asio::error::eof) {
        //parse_response();
    } else {
        SILOG(transfer, fatal, "Failed to read from CDN socket. Error = " << err.message());
    }
}

void HttpNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {
	//Higher layer should ensure that we don't get duplicates
    ReqCallbackPairType p(request, callback);
    mWaitingReqs.push(p);
	processRequests();
}

void HttpNameHandler::processRequests() {
    if (mOutstandingReqs.size() >= MAX_CONCURRENT_REQUESTS ||
            mWaitingReqs.empty() ||
            !mEndPointInitialized) {
        return;
    }

    ReqCallbackPairType p = mWaitingReqs.front();
    mOutstandingReqs[p.first] = p.second;
    mWaitingReqs.pop();

    mTCPSocket = new TCPSocket(*(mServicePool->service()));
    mTCPSocket->async_connect(
            mTCPEndpoint, boost::bind(&HttpNameHandler::handle_connect, this,
            boost::asio::placeholders::error, p));
}

}
}
