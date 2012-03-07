// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

// We don't use HttpManager Requests since they are internal and don't have the
// interface we would want. However, we also don't want to have to duplicate the
// gnarly code for including http_parser.
#include <sirikata/core/transfer/HttpManager.hpp>

#include "HttpServer.hpp"

#define HTTP_LOG(lvl, msg) SILOG(http-server, lvl, msg)

namespace Sirikata {
namespace Command {

using namespace boost::asio::ip;


namespace {
bool staticSettingsInitialized = false;
http_parser_settings EMPTY_PARSER_SETTINGS;
const char* httpStatusCodeStrings[600];

void initStaticSettings() {
    if (staticSettingsInitialized)
        return;

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

    memset(httpStatusCodeStrings, 0, sizeof(httpStatusCodeStrings));

    httpStatusCodeStrings[100] = "Continue";
    httpStatusCodeStrings[101] = "Switching Protocols";
    httpStatusCodeStrings[200] = "OK";
    httpStatusCodeStrings[201] = "Created";
    httpStatusCodeStrings[202] = "Accepted";
    httpStatusCodeStrings[203] = "Non-Authoritative Information";
    httpStatusCodeStrings[204] = "No Content";
    httpStatusCodeStrings[205] = "Reset Content";
    httpStatusCodeStrings[206] = "Partial Content";
    httpStatusCodeStrings[300] = "Multiple Choices";
    httpStatusCodeStrings[301] = "Moved Permanently";
    httpStatusCodeStrings[302] = "Found";
    httpStatusCodeStrings[303] = "See Other";
    httpStatusCodeStrings[304] = "Not Modified";
    httpStatusCodeStrings[305] = "Use Proxy";
    httpStatusCodeStrings[307] = "Temporary Redirect";
    httpStatusCodeStrings[400] = "Bad Request";
    httpStatusCodeStrings[401] = "Unauthorized";
    httpStatusCodeStrings[402] = "Payment Required";
    httpStatusCodeStrings[403] = "Forbidden";
    httpStatusCodeStrings[404] = "Not Found";
    httpStatusCodeStrings[405] = "Method Not Allowed";
    httpStatusCodeStrings[406] = "Not Acceptable";
    httpStatusCodeStrings[407] = "Proxy Authentication Required";
    httpStatusCodeStrings[408] = "Request Time-out";
    httpStatusCodeStrings[409] = "Conflict";
    httpStatusCodeStrings[410] = "Gone";
    httpStatusCodeStrings[411] = "Length Required";
    httpStatusCodeStrings[412] = "Precondition Failed";
    httpStatusCodeStrings[413] = "Request Entity Too Large";
    httpStatusCodeStrings[414] = "Request-URI Too Large";
    httpStatusCodeStrings[415] = "Unsupported Media Type";
    httpStatusCodeStrings[416] = "Requested range not satisfiable";
    httpStatusCodeStrings[417] = "Expectation Failed";
    httpStatusCodeStrings[500] = "Internal Server Error";
    httpStatusCodeStrings[501] = "Not Implemented";
    httpStatusCodeStrings[502] = "Bad Gateway";
    httpStatusCodeStrings[503] = "Service Unavailable";
    httpStatusCodeStrings[504] = "Gateway Time-out";
    httpStatusCodeStrings[505] = "HTTP Version not supported";

    staticSettingsInitialized = true;
}
} // namespace

class HttpRequest {
public:
    enum LAST_HEADER_CB {
        NONE,
        FIELD,
        VALUE
    };

    enum PARSING_STATE {
        PARSING,
        FINISHED_PARSING,
        FAILED
    };

    HttpRequest(HttpRequestID _id, TCPSocketPtr s)
     : id(_id),
       socket(s),
       state(PARSING),
       buffer(1024, '\0'),
       mLastCallback(NONE)
    {
        //Initialize http parser settings callbacks
        mHttpSettings = EMPTY_PARSER_SETTINGS;
        mHttpSettings.on_path = &HttpRequest::on_path;
        mHttpSettings.on_query_string = &HttpRequest::on_query_string;
        mHttpSettings.on_url = &HttpRequest::on_url;
        mHttpSettings.on_fragment = &HttpRequest::on_fragment;
        mHttpSettings.on_header_field = &HttpRequest::on_header_field;
        mHttpSettings.on_header_value = &HttpRequest::on_header_value;
        mHttpSettings.on_headers_complete = &HttpRequest::on_headers_complete;
        mHttpSettings.on_body = &HttpRequest::on_body;
        mHttpSettings.on_message_complete = &HttpRequest::on_message_complete;

        //Initialize the parser for parsing a response
        http_parser_init(&mHttpParser, HTTP_REQUEST);

        mHttpParser.data = static_cast<void *>(this);
    }

    // Add more data to the stream as it is streamed in. Reads nbytes out of buffer.
    void data(std::size_t nbytes) {
        size_t nparsed = http_parser_execute(&mHttpParser, &mHttpSettings, buffer.c_str(), nbytes);
        if (nparsed != nbytes)
            state = FAILED;
        // We'll set FINISHED if we get the message complete callback
    }

    void setFinished() { state = FINISHED_PARSING; }


    // http_parser callbacks
    static int on_path(http_parser* _, const char* at, size_t len);
    static int on_query_string(http_parser* _, const char* at, size_t len);
    static int on_url(http_parser* _, const char* at, size_t len);
    static int on_fragment(http_parser* _, const char* at, size_t len);
    static int on_header_field(http_parser* _, const char* at, size_t len);
    static int on_header_value(http_parser* _, const char* at, size_t len);
    static int on_headers_complete(http_parser* _);
    static int on_body(http_parser* _, const char* at, size_t len);
    static int on_message_complete(http_parser* _);

    HttpRequestID id;

    TCPSocketPtr socket;
    PARSING_STATE state;

    // Temp buffer for reading data from network
    String buffer;
    // Parsed request data
    String path;
    String queryString;
    String url;
    String fragment;
    Headers headers;
    String body;

    // Response data. Filled in by HttpServer
    String response;
protected:
    http_parser_settings mHttpSettings;
    http_parser mHttpParser;
    std::string mTempHeaderField;
    std::string mTempHeaderValue;
    LAST_HEADER_CB mLastCallback;
};


int HttpRequest::on_path(http_parser* _, const char* at, size_t len) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);
    curRequest->path.append(at, len);
    return 0;
}

int HttpRequest::on_query_string(http_parser* _, const char* at, size_t len) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);
    curRequest->queryString.append(at, len);
    return 0;
}

int HttpRequest::on_url(http_parser* _, const char* at, size_t len) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);
    curRequest->url.append(at, len);
    return 0;
}

int HttpRequest::on_fragment(http_parser* _, const char* at, size_t len) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);
    curRequest->fragment.append(at, len);
    return 0;
}

int HttpRequest::on_header_field(http_parser* _, const char* at, size_t len) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);

    //See http-parser documentation for why this is necessary
    switch (curRequest->mLastCallback) {
        case VALUE:
            //Previous header name/value is finished, so save
            curRequest->headers[curRequest->mTempHeaderField] = curRequest->mTempHeaderValue;
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

int HttpRequest::on_header_value(http_parser* _, const char* at, size_t len) {
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

int HttpRequest::on_headers_complete(http_parser* _) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);

    //Check for last header that might not have been saved
    if (curRequest->mLastCallback == VALUE) {
        curRequest->headers[curRequest->mTempHeaderField] = curRequest->mTempHeaderValue;
    }

    return 0;
}

int HttpRequest::on_body(http_parser* _, const char* at, size_t len) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);
    curRequest->body.append(at, len);
    return 0;
}

int HttpRequest::on_message_complete(http_parser* _) {
    HttpRequest* curRequest = static_cast<HttpRequest*>(_->data);
    curRequest->setFinished();
    HTTP_LOG(detailed, "Finished parsing HTTP request: " << curRequest->path);
    HTTP_LOG(insane, "  Path: " << curRequest->path);
    HTTP_LOG(insane, "  URL: " << curRequest->url);
    HTTP_LOG(insane, "  Query: " << curRequest->queryString);
    HTTP_LOG(insane, "  Fragment: " << curRequest->fragment);
    HTTP_LOG(insane, "  Headers: ");
    for(Headers::iterator it = curRequest->headers.begin(); it != curRequest->headers.end(); it++)
        HTTP_LOG(insane, "    " << it->first << ": " << it->second);
    HTTP_LOG(insane, "  Body: " << curRequest->body.size() << " bytes");
    return 0;
}




HttpServer::HttpServer(Context* ctx, const String& host, uint16 port)
 : mContext(ctx),
   mHost(host),
   mPort(port),
   mRequestIDSource(1)
{
    initStaticSettings();

    mAcceptor =
        TCPListenerPtr(new Network::TCPListener(*(mContext->ioService), tcp::endpoint(tcp::v4(), mPort)));
    acceptConnection();
}

HttpServer::~HttpServer() {
}


void HttpServer::acceptConnection() {
    TCPSocketPtr socket(new Network::TCPSocket(*(mContext->ioService)));
    mAcceptor->async_accept(
        *socket,
        boost::bind(&HttpServer::handleConnection, this, socket)
    );
}

void HttpServer::handleConnection(TCPSocketPtr socket) {
    // Always start listening for a new connection
    acceptConnection();

    // And start parsing the request
    HttpRequestPtr req;
    {
        Lock lck(mMutex);
        HttpRequestID id = mRequestIDSource++;
        req = HttpRequestPtr(new HttpRequest(id, socket));
        mRequests.insert(req);
    }

    // Start reading data + parsing
    readRequestData(req);
}

void HttpServer::readRequestData(HttpRequestPtr req) {
    req->socket->async_read_some(
        boost::asio::buffer(&(req->buffer[0]), req->buffer.size()),
        boost::bind(&HttpServer::handleReadRequestData, this, req,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
    );
}

void HttpServer::handleReadRequestData(HttpRequestPtr req, const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
        HTTP_LOG(error, "Error reading HTTP request");
        Lock lck(mMutex);
        mRequests.erase(req);
        return;
    }

    req->data(bytes_transferred);
    if (req->state == HttpRequest::FAILED) {
        HTTP_LOG(error, "Parsing http request failed");
        Lock lck(mMutex);
        mRequests.erase(req);
        return;
    }
    else if (req->state == HttpRequest::FINISHED_PARSING) {
        HTTP_LOG(detailed, "Parsed request");
        handleRequest(req);
        return;
    }

    // Otherwise, need more data to continue parsing
    readRequestData(req);
}


void HttpServer::handleRequest(HttpRequestPtr req) {
    Lock lck(mMutex);
    mProcessingRequests[req->id] = req;
    mRequests.erase(req);
    // Notify listeners. They need to make sure that only one person responds.
    notify(&HttpRequestListener::onHttpRequest, this, req->id, req->path, req->queryString, req->fragment, req->headers, req->body);
}

void HttpServer::response(HttpRequestID id, HttpStatus status, const Headers& headers, const String& body) {
    HttpRequestPtr req;
    {
        Lock lck(mMutex);

        RequestMap::iterator req_it = mProcessingRequests.find(id);
        if (req_it == mProcessingRequests.end()) {
            HTTP_LOG(error, "Request " << id << " doesn't exist. Another handler may have already responded to the request.");
            return;
        }
        req = req_it->second;
    }

    // In case we have multiple listeners that respond, make sure we're not
    // already responding to this
    if (!req->response.empty()) {
        HTTP_LOG(error, "Already sending a response for request " << id);
        return;
    }

    // Format the response
    std::stringstream response;
    assert(status < 600);
    response << "HTTP/1.1 " << status << " " << httpStatusCodeStrings[status] << "\r\n";
    for(Headers::const_iterator head_it = headers.begin(); head_it != headers.end(); head_it++)
        response << head_it->first << ": " << head_it->second << "\r\n";
    // If Content-Length wasn't manually specified, add it in
    if(headers.find("Content-Length") == headers.end())
        response << "Content-Length: " << body.size() << "\r\n";
    // Currently don't support multiple requests on the same connection.
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    req->response = response.str();

    HTTP_LOG(insane, "Generated response:");
    HTTP_LOG(insane, response.str());

    // Start sending the response. We'll clear things out of storage when we're
    // done sending.
    writeResponseData(req, 0);
}

void HttpServer::writeResponseData(HttpRequestPtr req, uint32 offset) {
    req->socket->async_write_some(
        boost::asio::buffer(&(req->response[offset]), req->response.size()-offset),
        boost::bind(&HttpServer::handleWriteResponseData, this,
            req, offset,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
    );
}

void HttpServer::handleWriteResponseData(HttpRequestPtr req, uint32 offset, const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
        HTTP_LOG(error, "Error writing HTTP request, aborting");
        Lock lck(mMutex);
        mProcessingRequests.erase(req->id);
        return;
    }

    offset += bytes_transferred;
    if (offset < req->response.size()) {
        // More to write
        writeResponseData(req, offset);
        return;
    }

    // Otherwise, we're done
    Lock lck(mMutex);
    mProcessingRequests.erase(req->id);
}


} // namespace Command
} // namespace Sirikata
