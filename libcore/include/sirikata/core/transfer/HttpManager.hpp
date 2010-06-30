/*  Sirikata Transfer -- Content Transfer management system
 *  HttpManager.hpp
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

#ifndef SIRIKATA_HttpManager_HPP__
#define SIRIKATA_HttpManager_HPP__

#include <sirikata/proxyobject/Platform.hpp>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <sirikata/core/network/IOServicePool.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/network/Address.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include "http_parser.h"

namespace Sirikata {
namespace Transfer {

/*
 * Handles managing connections to the CDN
 */
class SIRIKATA_EXPORT HttpManager
    : public AutoSingleton<HttpManager> {

public:

    /*
     * Stores headers and data returned from an HTTP request
     * Also stores HTTP status code and content length in numeric format
     * for convenience. These headers are also present in raw (string) format
     *
     * Note that getData might return a null pointer even if the
     * http request was successful, e.g. for a HEAD request
     *
     * Note that getContentLength might not be a valid value. If there was no
     * content length header in the response, getContentLength is undefined.
     */
    class HttpResponse {
    protected:
        std::map<std::string, std::string> mHeaders;
        std::tr1::shared_ptr<DenseData> mData;
        unsigned short mContentLength;
        unsigned short mStatusCode;
    public:
        HttpResponse() : mContentLength(0), mStatusCode(0) {}
        inline std::tr1::shared_ptr<DenseData> getData() { return mData; }
        inline const std::map<std::string, std::string>& getHeaders() { return mHeaders; }
        inline unsigned short getContentLength() { return mContentLength; }
        inline unsigned short getStatusCode() { return mStatusCode; }

        friend class HttpManager;
    };

    enum ERR_TYPE {
        SUCCESS,
        REQUEST_PARSING_FAILED,
        RESPONSE_PARSING_FAILED,
        BOOST_ERROR
    };

    /*
     * Callback for an HTTP request. If error == SUCCESS, then
     * response is an HttpResponse object that has headers and data.
     * If error == BOOST_ERROR, then boost_error is set to the boost
     * error code. If an error is returned, response is NULL.
     */
    typedef std::tr1::function<void(
                std::tr1::shared_ptr<HttpResponse> response,
                ERR_TYPE error,
                const boost::system::error_code& boost_error
            )> HttpCallback;

    static HttpManager& getSingleton();
    static void destroy();

    /*
     * Makes an HTTP request and calls cb when finished
     */
    void makeRequest(Sirikata::Network::Address addr, std::string req, HttpCallback cb);

    HttpManager();
    ~HttpManager();

private:
    //For convenience
    typedef Sirikata::Network::IOServicePool IOServicePool;
    typedef Sirikata::Network::TCPResolver TCPResolver;
    typedef Sirikata::Network::TCPSocket TCPSocket;
    typedef Sirikata::Network::IOWork IOWork;
    typedef boost::asio::ip::tcp::endpoint TCPEndPoint;

    //Convenience of storing request parameters together
    class HttpRequest {
    public:
        Sirikata::Network::Address addr;
        std::string req;
        HttpCallback cb;
        HttpRequest(Sirikata::Network::Address _addr, std::string _req, HttpCallback _cb)
            : addr(_addr), req(_req), cb(_cb) {}
    };

    //Holds a queue of requests to be made
    std::queue<HttpRequest> mRequestQueue;

    //TODO: should get these from settings
    static const uint32 MAX_CONCURRENT_REQUESTS = 1;
    //Number of requests outstanding
    uint32 numOutstanding;

    IOServicePool* mServicePool;
    TCPResolver* mResolver;
    IOWork* mIOWork;

    http_parser_settings EMPTY_PARSER_SETTINGS;
    http_parser_settings mHttpSettings;
    http_parser mHttpParser;

    void processQueue();

    void handle_resolve(HttpRequest req, const boost::system::error_code& err,
            TCPResolver::iterator endpoint_iterator);
    void handle_connect(std::tr1::shared_ptr<TCPSocket> socket, HttpRequest req,
            const boost::system::error_code& err, TCPResolver::iterator endpoint_iterator);
    void handle_write_request(std::tr1::shared_ptr<TCPSocket> socket, HttpRequest req,
            const boost::system::error_code& err);
    void handle_read(std::tr1::shared_ptr<TCPSocket> socket, HttpRequest req,
            std::tr1::shared_ptr<boost::asio::streambuf> response, const boost::system::error_code& err,
            std::size_t bytes_transferred);


    static std::tr1::shared_ptr<HttpResponse> mTempResponse;
    static std::string mTempHeaderName;
    static int on_header_field(http_parser *_, const char *at, size_t len);
    static int on_header_value(http_parser *_, const char *at, size_t len);
    static int on_body(http_parser *_, const char *at, size_t len);

};

}
}

#endif
