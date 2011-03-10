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
#include <deque>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
// Apple defines the check macro in AssertMacros.h, which screws up some code in boost's iostreams lib
#ifdef check
#undef check
#endif
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>
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
        // This stuff is all used internally for http-parser
        enum LAST_HEADER_CB {
            NONE,
            FIELD,
            VALUE
        };
        std::string mTempHeaderField;
        std::string mTempHeaderValue;
        LAST_HEADER_CB mLastCallback;
        bool mHeaderComplete;
        bool mMessageComplete;
        http_parser_settings mHttpSettings;
        http_parser mHttpParser;
        bool mGzip;
        std::stringstream mCompressedStream;
        //

        std::map<std::string, std::string> mHeaders;
        std::tr1::shared_ptr<DenseData> mData;
        ssize_t mContentLength;
        unsigned short mStatusCode;

        HttpResponse()
            : mLastCallback(NONE), mHeaderComplete(false), mMessageComplete(false),
              mGzip(false), mContentLength(0), mStatusCode(0) {}
    public:
        inline std::tr1::shared_ptr<DenseData> getData() { return mData; }
        inline const std::map<std::string, std::string>& getHeaders() { return mHeaders; }
        inline ssize_t getContentLength() { return mContentLength; }
        inline unsigned short getStatusCode() { return mStatusCode; }

        friend class HttpManager;
    };

    //Type of errors that can be given to callback
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

    //Methods supported
    enum HTTP_METHOD {
        HEAD,
        GET
    };

    /*
     * Makes an HTTP request and calls cb when finished
     */
    void makeRequest(Sirikata::Network::Address addr, HTTP_METHOD method, std::string req, HttpCallback cb);

protected:
    /*
     * Protect constructor and destructor so can't make an instance of this class
     * but allow AutoSingleton<HttpManager> to make a copy, and give access
     * to the destructor to the auto_ptr used by AutoSingleton
     */
    HttpManager();
    ~HttpManager();
    friend class AutoSingleton<HttpManager>;
    friend std::auto_ptr<HttpManager>::~auto_ptr();
    friend void std::auto_ptr<HttpManager>::reset(HttpManager*);

private:
    //For convenience
    typedef Sirikata::Network::IOServicePool IOServicePool;
    typedef Sirikata::Network::TCPResolver TCPResolver;
    typedef Sirikata::Network::TCPSocket TCPSocket;
    typedef Sirikata::Network::IOWork IOWork;
    typedef Sirikata::Network::IOCallback IOCallback;
    typedef boost::asio::ip::tcp::endpoint TCPEndPoint;

    //Convenience of storing request parameters together
    class HttpRequest {
    public:
        const Sirikata::Network::Address addr;
        const std::string req;
        const HttpCallback cb;
        const HTTP_METHOD method;
        HttpRequest(Sirikata::Network::Address _addr, std::string _req, HTTP_METHOD meth, HttpCallback _cb)
            : addr(_addr), req(_req), cb(_cb), method(meth) {}
    };

    //Holds a queue of requests to be made
    typedef std::list<std::tr1::shared_ptr<HttpRequest> > RequestQueueType;
    RequestQueueType mRequestQueue;
    //Lock this to access mRequestQueue
    boost::mutex mRequestQueueLock;

    //TODO: should get these from settings
    static const uint32 MAX_CONNECTIONS_PER_ENDPOINT = 2;
    static const uint32 MAX_TOTAL_CONNECTIONS = 10;
    static const uint32 SOCKET_BUFFER_SIZE = 10240;

    //Keeps track of the total number of connections currently open
    uint32 mNumTotalConnections;

    //Keeps track of the number of connections open per host:port pair
    typedef std::map<Sirikata::Network::Address, uint32> NumConnsType;
    NumConnsType mNumConnsPerAddr;
    //Lock this to access mNumTotalConnections or mNumConnsPerAddr
    boost::mutex mNumConnsLock;

    //Holds connections that are open but not being used
    typedef std::map<Sirikata::Network::Address,
        std::queue<std::tr1::shared_ptr<TCPSocket> > > RecycleBinType;
    RecycleBinType mRecycleBin;
    //Lock this to access mRecycleBin
    boost::mutex mRecycleBinLock;

    IOServicePool* mServicePool;
    TCPResolver* mResolver;

    http_parser_settings EMPTY_PARSER_SETTINGS;

    void processQueue();

    void add_req(std::tr1::shared_ptr<HttpRequest> req);
    void decrement_connection(const Sirikata::Network::Address& addr);
    void write_request(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req);

    void handle_resolve(std::tr1::shared_ptr<HttpRequest> req, const boost::system::error_code& err,
            TCPResolver::iterator endpoint_iterator);
    void handle_connect(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req,
            const boost::system::error_code& err, TCPResolver::iterator endpoint_iterator);
    void handle_write_request(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req,
            const boost::system::error_code& err, std::tr1::shared_ptr<boost::asio::streambuf> request_stream);
    void handle_read(std::tr1::shared_ptr<TCPSocket> socket, std::tr1::shared_ptr<HttpRequest> req,
            std::tr1::shared_ptr<std::vector<unsigned char> > vecbuf, std::tr1::shared_ptr<HttpResponse> respPtr,
            const boost::system::error_code& err, std::size_t bytes_transferred);

    static int on_header_field(http_parser *_, const char *at, size_t len);
    static int on_header_value(http_parser *_, const char *at, size_t len);
    static int on_headers_complete(http_parser *_);
    static int on_body(http_parser *_, const char *at, size_t len);
    static int on_message_complete(http_parser *_);

    enum HTTP_PARSER_FLAGS
      { F_CHUNKED = 1 << 0
      , F_CONNECTION_KEEP_ALIVE = 1 << 1
      , F_CONNECTION_CLOSE = 1 << 2
      , F_TRAILING = 1 << 3
      , F_UPGRADE = 1 << 4
      , F_SKIPBODY = 1 << 5
      };

    static void print_flags(std::tr1::shared_ptr<HttpResponse> resp);

public:

    /*
     * Posts a callback on the service pool
     */
    void postCallback(IOCallback cb);

};

}
}

#endif
