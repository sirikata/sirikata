// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_HTTP_SERVER_HPP_
#define _SIRIKATA_LIBCORE_HTTP_SERVER_HPP_

#include <sirikata/core/command/Commander.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/network/Asio.hpp>

namespace Sirikata {
namespace Command {

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

typedef uint16 HttpStatus;

// Unique identifer for requests coming into the HttpServer.
typedef uint32 HttpRequestID;

class HttpServer;

class HttpRequestListener {
public:
    virtual ~HttpRequestListener() {}

    /** Invoked when a complete HttpRequest is ready for processing.
     *  \param server the HttpServer this request originated from
     *  \param id the unique ID for this request. Should be included when
     *            triggering the response
     *  \param path path part of the requested URL
     *  \param query (encoded) query string part of the requested URL
     *  \param fragment fragment part of the requested URL
     *  \param headers map of headers included with the request
     *  \param body the body of the request
     *
     *  \note Parameters are passed as mutable references -- you are permitted
     *        to modify them. They will remain valid until you call
     *        HttpServer::response().
     **/
    virtual void onHttpRequest(HttpServer* server, HttpRequestID id, String& path, String& query, String& fragment, Headers& headers, String& body) = 0;
};


typedef std::tr1::shared_ptr<Network::TCPListener> TCPListenerPtr;
typedef std::tr1::shared_ptr<Network::TCPSocket> TCPSocketPtr;

class HttpRequest;
typedef std::tr1::shared_ptr<HttpRequest> HttpRequestPtr;

class HttpServer : public Provider<HttpRequestListener*> {
public:
    HttpServer(Context* ctx, const String& host, uint16 port);
    ~HttpServer();

    void response(HttpRequestID id, HttpStatus status, const Headers& headers, const String& body);
private:

    void acceptConnection();
    void handleConnection(TCPSocketPtr socket);

    void readRequestData(HttpRequestPtr req);
    void handleReadRequestData(HttpRequestPtr req, const boost::system::error_code& ec, std::size_t bytes_transferred);

    void handleRequest(HttpRequestPtr req);

    void writeResponseData(HttpRequestPtr req, uint32 offset);
    void handleWriteResponseData(HttpRequestPtr req, uint32 offset, const boost::system::error_code& ec, std::size_t bytes_transferred);



    Context* mContext;
    String mHost;
    uint16 mPort;

    TCPListenerPtr mAcceptor;

    // Much of the common steps work on data independently, and only one
    // callback can be active at a time for any given request. This just
    // protects the shared state (set of requests);
    typedef boost::recursive_mutex Mutex;
    typedef boost::lock_guard<Mutex> Lock;
    Mutex mMutex;

    typedef std::set<HttpRequestPtr> RequestSet;
    RequestSet mRequests;

    // Source of request IDs
    HttpRequestID mRequestIDSource;

    // Requests we've assigned IDs and sent out events for, but which we're
    // waiting for response() calls for.
    typedef std::map<HttpRequestID, HttpRequestPtr> RequestMap;
    RequestMap mProcessingRequests;
}; // class HttpServer

} // namespace Command
} // namespace Sirikata


#endif //_SIRIKATA_LIBCORE_HTTP_SERVER_HPP_
