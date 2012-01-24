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

#include <sirikata/core/util/Platform.hpp>
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


// This is a hack around a problem created by different packages
// creating slightly different typedefs -- notably http_parser and
// v8. On windows, we need to provide typedefs and make it skip that
// part of http_parser.h.
#if defined(_WIN32) && !defined(__MINGW32__)

typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

typedef unsigned int size_t;
typedef int ssize_t;

// We turn on mingw to make it skip the include block, but then stdint
// is included, and only available via a zzip dependency. We block
// that out as well by using its include guard define
#define _ZZIP__STDINT_H 1
#define __MINGW32__
#include "http_parser.h"
#undef __MINGW32__

#else

#include "http_parser.h"

#endif

namespace Sirikata {
namespace Transfer {

/*
 * Handles managing connections to the CDN
 */
class SIRIKATA_EXPORT HttpManager
    : public AutoSingleton<HttpManager> {

protected:
	enum LAST_HEADER_CB {
		NONE,
		FIELD,
		VALUE
	};

public:
        typedef std::map<std::string, std::string> StringDictionary;
        // StringDictionary that uses case-insensitive keys, as required for
        // http headers by RFC 2616
        struct CaseInsensitiveStringLess {
            bool operator()(const std::string& lhs, const std::string& rhs) const {
                std::size_t lsize = lhs.size(), rsize = rhs.size();
                if (lsize != rsize) return (lsize < rsize);
                for(std::size_t i = 0; i < lsize; i++) {
                    char li = std::tolower(lhs[i]), ri = std::tolower(rhs[i]);
                    if (li != ri) return (li < ri);
                }
                return false;
            }
        };
        typedef std::map<std::string, std::string, CaseInsensitiveStringLess> CaseInsensitiveStringDictionary;

        typedef CaseInsensitiveStringDictionary Headers;
        typedef StringDictionary QueryParameters;

        /** Represents one field in a multipart/form-data */
        struct MultipartData {
            MultipartData(const String& _field, const String& _data)
             : field(_field), headers(), filename(""), data(_data)
            {}
            MultipartData(const String& _field, const String& _data, const String& _filename)
             : field(_field), headers(), filename(_filename), data(_data)
            {}
            MultipartData(const String& _field, const String& _data, const String& _filename, const Headers& _headers)
             : field(_field), headers(_headers), filename(_filename), data(_data)
            {}

            String field;
            Headers headers;
            String filename;
            String data;
        };
        typedef std::vector<MultipartData> MultipartDataList;

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

        Headers mHeaders;
        std::tr1::shared_ptr<DenseData> mData;
        ssize_t mContentLength;
        unsigned short mStatusCode;

        HttpResponse()
            : mLastCallback(NONE), mHeaderComplete(false), mMessageComplete(false),
              mGzip(false), mContentLength(0), mStatusCode(0) {}
    public:
        inline std::tr1::shared_ptr<DenseData> getData() { return mData; }
        inline const Headers& getHeaders() { return mHeaders; }
        inline StringDictionary getRawHeaders() {
            StringDictionary raw_headers;
            for(Headers::const_iterator it = mHeaders.begin(); it != mHeaders.end(); it++)
                raw_headers[it->first] = it->second;
            return raw_headers;
        }
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
        GET,
        POST
    };
    static String methodAsString(HTTP_METHOD m);

    /** Makes an HTTP request and calls cb when finished. This is the lowest
     *  level version exposed publicly, taking a raw HTTP request, which you
     *  should ensure is properly formatted. Usually you should use the
     *  convenience wrappers that format the request for you.
     */
    void makeRequest(Sirikata::Network::Address addr, HTTP_METHOD method, std::string req, bool allow_redirects, HttpCallback cb);

    /** Formats and makes an HTTP request and calls cb when finished. This
     *  version is a utility for the more specific request types (i.e. head()
     *  and get()).
     *
     *  \param addr the address of the server
     *  \param method the HTTP request method, i.e. HEAD, GET, or POST
     *  \param path the path of the resource to access
     *  \param cb callback to invoke upon completion
     *  \param headers dictionary of headers to add to the request
     *  \param query_params dictionary of unencoded query parameters to add to
     *         the url
     *  \param body if non-empty, the encoded HTTP request body, i.e. the data
     *         provided after all the headers. If you provide this, you should
     *         probably include headers to specify it's format
     *  \param allow_redirects if true, redirects will be followed, triggering a
     *         new requests
     */
    void makeRequest(
        Sirikata::Network::Address addr, HTTP_METHOD method, const String& path,
        HttpCallback cb,
        const Headers& headers = Headers(), const QueryParameters& query_params = QueryParameters(),
        const String& body = "",
        bool allow_redirects = true
    );

    static String formatURLEncodedDictionary(const StringDictionary& query_params);
    static String formatPath(const String& path, const QueryParameters& query_params);
    static String formatURL(const String& host, const String& service, const String& path, const QueryParameters& query_params);

    void head(
        Sirikata::Network::Address addr, const String& path,
        HttpCallback cb, const Headers& headers = Headers(), const QueryParameters& query_params = QueryParameters(),
        bool allow_redirects = true
    );

    void get(
        Sirikata::Network::Address addr, const String& path,
        HttpCallback cb, const Headers& headers = Headers(), const QueryParameters& query_params = QueryParameters(),
        bool allow_redirects = true
    );

    /** Perform an HTTP POST using the specified content type and message
     *  body. This can be used if you want to use an unusual encoding or as a
     *  utility for other, more specific post methods.
     */
    void post(
        Sirikata::Network::Address addr, const String& path,
        const String& content_type, const String& body,
        HttpCallback cb, const Headers& headers = Headers(), const QueryParameters& query_params = QueryParameters(),
        bool allow_redirects = true
    );

    /** Perform a HTTP POST whose body is x-www-form-urlencoded parameters. */
    void postURLEncoded(
        Sirikata::Network::Address addr, const String& path,
        const StringDictionary& body,
        HttpCallback cb, const Headers& headers = Headers(), const QueryParameters& query_params = QueryParameters(),
        bool allow_redirects = true
    );

    /** Perform a HTTP POST whose body is multipart/form-data encoded body. */
    void postMultipartForm(
        Sirikata::Network::Address addr, const String& path,
        const MultipartDataList& data,
        HttpCallback cb, const Headers& headers = Headers(), const QueryParameters& query_params = QueryParameters(),
        bool allow_redirects = true
    );

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

    // Formats a URL encoded dictionary -- for form-urlencoded data or query
    // strings. NOTE: There is no ? prefixed to this.
    static void formatURLEncodedDictionary(std::ostream& os, const StringDictionary& query_params);
    // Formats the entire path portion of a URL -- path + query args
    static void formatPath(std::ostream& os, const String& path, const QueryParameters& query_params);
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
        const bool allow_redirects;
        HttpRequest(Sirikata::Network::Address _addr, std::string _req, HTTP_METHOD meth, bool _allow_redirects, HttpCallback _cb)
         : addr(_addr), req(_req), cb(_cb), method(meth), allow_redirects(_allow_redirects),
           mNumTries(0), mLastCallback(NONE), mHeaderComplete(false) {}

        friend class HttpManager;
    protected:
        uint32 mNumTries;
        http_parser_settings mHttpSettings;
        http_parser mHttpParser;
        std::string mTempHeaderField;
        std::string mTempHeaderValue;
        LAST_HEADER_CB mLastCallback;
        bool mHeaderComplete;
        Headers mHeaders;
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

    static int on_request_header_field(http_parser *_, const char *at, size_t len);
    static int on_request_header_value(http_parser *_, const char *at, size_t len);
    static int on_request_headers_complete(http_parser *_);

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
    void postCallback(IOCallback cb, const char* tag);
    void postCallback(const Duration& waitFor, IOCallback cb, const char* tag);

};

}
}

#endif
