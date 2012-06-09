// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_HTTP_SERVERID_MAP_HPP_
#define _SIRIKATA_HTTP_SERVERID_MAP_HPP_

#include <sirikata/core/network/ServerIDMap.hpp>
#include <sirikata/core/transfer/HttpManager.hpp>

namespace Sirikata {

/** A ServerIDMap which looks up servers with Http requests.
 *
 *  Currently the implementation does a simple GET request with a query
 *  parameter "server" with a numeric value for the server ID to lookup.
 */
class HttpServerIDMap : public ServerIDMap {
    const String mInternalHost;
    const String mInternalService;
    const String mInternalPath;

    const String mExternalHost;
    const String mExternalService;
    const String mExternalPath;

    void startLookup(
        const ServerID& sid, Address4LookupCallback cb,
        const String& host,
        const String& service,
        const String& path
    );
    void finishLookup(
        const ServerID& sid, Address4LookupCallback cb,
        Transfer::HttpManager::HttpResponsePtr response,
        Transfer::HttpManager::ERR_TYPE error,
        const boost::system::error_code &boost_error);
public:
    HttpServerIDMap(
        Context* ctx,
        const String& internal_host, const String& internal_service, const String& internal_path,
        const String& external_host, const String& external_service, const String& external_path
    );
    virtual ~HttpServerIDMap() {}

    virtual void lookupInternal(const ServerID& sid, Address4LookupCallback cb);
    virtual void lookupExternal(const ServerID& sid, Address4LookupCallback cb);
    virtual void lookupRandomExternal(Address4LookupCallback cb);
};

} // namespace Sirikata

#endif //_SIRIKATA_HTTP_SERVERID_MAP_HPP_
