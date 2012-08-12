// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "HttpServerIDMap.hpp"
#include <sirikata/core/network/Address4.hpp>
#include <boost/lexical_cast.hpp>
#include <json_spirit/json_spirit.h>

namespace Sirikata {

using namespace Sirikata::Transfer;

HttpServerIDMap::HttpServerIDMap(
    Context* ctx,
    const String& internal_host, const String& internal_service, const String& internal_path,
    const String& external_host, const String& external_service, const String& external_path,
    uint32 retries)
 : ServerIDMap(ctx),
   mInternalHost(internal_host),
   mInternalService(internal_service),
   mInternalPath(internal_path),
   mExternalHost(external_host),
   mExternalService(external_service),
   mExternalPath(external_path),
   mRetries(retries)
{
}


void HttpServerIDMap::lookupInternal(const ServerID& sid, Address4LookupCallback cb) {
    startLookup(
        RequestInfo(
            sid, cb,
            mInternalHost, mInternalService, mInternalPath
        ),
        0
    );
}

void HttpServerIDMap::lookupExternal(const ServerID& sid, Address4LookupCallback cb) {
    startLookup(
        RequestInfo(
            sid, cb,
            mExternalHost, mExternalService, mExternalPath
        ),
        0
    );
}

void HttpServerIDMap::lookupRandomExternal(Address4LookupCallback cb) {
    // NullServerID indicates pick randomly
    startLookup(
        RequestInfo(
            NullServerID, cb,
            mExternalHost, mExternalService, mExternalPath
        ),
        0
    );
}

void HttpServerIDMap::startLookup(
    const RequestInfo& ri,
    const uint32 retry
){
    Network::Address addr(ri.host, ri.service);

    HttpManager::Headers request_headers;
    request_headers["Host"] = ri.host;
    HttpManager::QueryParameters query_params;
    // Empty query params if 0, i.e. random, is requested
    if (ri.sid != NullServerID)
        query_params["server"] = boost::lexical_cast<String>(ri.sid);
    HttpManager::getSingleton().get(
        addr, ri.path,
        std::tr1::bind(&HttpServerIDMap::finishLookup, this, ri, retry, _1, _2, _3),
        request_headers, query_params
    );
}

void HttpServerIDMap::finishLookup(
    const RequestInfo& ri,
    const uint32 retry,
    HttpManager::HttpResponsePtr response,
    HttpManager::ERR_TYPE error,
    const boost::system::error_code &boost_error)
{
    // Error conditions
    if (error != HttpManager::SUCCESS) {
        if (!retryOrFail(ri, retry))
            SILOG(http-serverid-map, error, "Failed to lookup server " << ri.sid);
        return;
    }

    String resp = response->getData()->asString();
    // Check response code for failure
    if (response->getStatusCode() >= 400) {
        if (!retryOrFail(ri, retry))
            SILOG(http-serverid-map, error, "Request to HTTP server failed when looking up server " << ri.sid << ": " << resp);
        return;
    }

    // Parse the response. Currently support text/plain or application/json. If
    // not provided, assume text/plain.
    String content_type = "text/plain";
    if (response->getHeaders().find("Content-Type") != response->getHeaders().end())
        content_type = response->getHeaders().find("Content-Type")->second;

    if (content_type == "text/plain") {
        // Currently we assume we have (serverid)\n(host):(service).
        String::size_type line_split_pos = resp.find("\n");
        String::size_type split_pos = resp.find(":");
        if (line_split_pos == String::npos || split_pos == String::npos) {
            if (!retryOrFail(ri, retry))
                SILOG(http-serverid-map, error, "Couldn't parse response for server lookup " << ri.sid << ": " << resp);
            return;
        }

        ServerID resolved_sid = boost::lexical_cast<ServerID>( resp.substr(0,line_split_pos) );
        assert(resolved_sid == ri.sid || ri.sid == NullServerID);
        Network::Address retval(
            resp.substr(line_split_pos+1,split_pos-line_split_pos-1),
            resp.substr(split_pos+1)
        );
        mContext->ioService->post(std::tr1::bind(ri.cb, resolved_sid, retval), "HttpServerIDMap::finishLookup");
    }
    else if (content_type == "application/json") {
        namespace json = json_spirit;
        json::Value json_resp;
        if (!json::read(resp, json_resp)) {
            if (!retryOrFail(ri, retry))
                SILOG(http-serverid-map, error, "Couldn't parse response for server lookup " << ri.sid << ": " << resp);
            return;
        }

        if (!json_resp.contains("server") ||
            !json_resp.contains("host") ||
            !json_resp.contains("port"))
        {
            if (!retryOrFail(ri, retry))
                SILOG(http-serverid-map, error, "Response from server doesn't contain all required fields: " << resp);
            return;
        }

        int32 server = json_resp.getInt("server");
        String host = json_resp.getString("host");
        String port_str = json_resp.getString("port");

        ServerID resolved_sid = boost::lexical_cast<ServerID>(server);
        Network::Address retval( host, port_str );
        mContext->ioService->post(std::tr1::bind(ri.cb, resolved_sid, retval), "HttpServerIDMap::finishLookup");
    }
    else {
        if (!retryOrFail(ri, retry))
            SILOG(http-serverid-map, error, "Unknown content type for HTTP server map response: " << content_type);
        return;
    }
}

bool HttpServerIDMap::retryOrFail(
    const RequestInfo& ri,
    const uint32 retry
) {
    if (retry < mRetries) {
        mContext->ioService->post(
            Duration::seconds(pow(2.0, (float)retry)),
            std::tr1::bind(&HttpServerIDMap::startLookup, this, ri, retry+1)
        );
        return true;
    }

    mContext->ioService->post(std::tr1::bind(ri.cb, ri.sid, Address4::Null), "HttpServerIDMap::finishLookup");
    return false;
}

}//end namespace sirikata
