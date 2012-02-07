// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_OAUTH_HTTP_MANAGER_HPP_
#define _SIRIKATA_LIBCORE_OAUTH_HTTP_MANAGER_HPP_

#include <sirikata/core/transfer/HttpManager.hpp>
#include <sirikata/core/transfer/OAuthParams.hpp>

namespace Sirikata {
namespace Transfer {

/** Wrapper around HttpManager which signs requests as an OAuth consumer. Unlike
 *  HttpManager, this is not a Singleton so that you can use this multiple times
 *  to be different consumers to different sites.
 */
class SIRIKATA_EXPORT OAuthHttpManager {
public:
    OAuthHttpManager(OAuthParamsPtr oauth)
        : mOAuth(oauth),
          mClient(oauth->consumerPtr(), oauth->tokenPtr())
    {}

    void head(
        Sirikata::Network::Address addr, const String& path,
        HttpManager::HttpCallback cb,
        const HttpManager::Headers& headers = HttpManager::Headers(), const HttpManager::QueryParameters& query_params = HttpManager::QueryParameters(),
        bool allow_redirects = true
    );

    void get(
        Sirikata::Network::Address addr, const String& path,
        HttpManager::HttpCallback cb,
        const HttpManager::Headers& headers = HttpManager::Headers(), const HttpManager::QueryParameters& query_params = HttpManager::QueryParameters(),
        bool allow_redirects = true
    );

    void post(
        Sirikata::Network::Address addr, const String& path,
        const String& content_type, const String& body,
        HttpManager::HttpCallback cb, const HttpManager::Headers& headers = HttpManager::Headers(), const HttpManager::QueryParameters& query_params = HttpManager::QueryParameters(),
        bool allow_redirects = true
    );

    void postURLEncoded(
        Sirikata::Network::Address addr, const String& path,
        const HttpManager::StringDictionary& body,
        HttpManager::HttpCallback cb, const HttpManager::Headers& headers = HttpManager::Headers(), const HttpManager::QueryParameters& query_params = HttpManager::QueryParameters(),
        bool allow_redirects = true
    );

    void postMultipartForm(
        Sirikata::Network::Address addr, const String& path,
        const HttpManager::MultipartDataList& data,
        HttpManager::HttpCallback cb, const HttpManager::Headers& headers = HttpManager::Headers(), const HttpManager::QueryParameters& query_params = HttpManager::QueryParameters(),
        bool allow_redirects = true
    );

private:
    OAuthParamsPtr mOAuth;
    // The OAuth client uses mutable state, so it's not stored with the
    // params. It needs to be locked for thread safety
    boost::mutex mMutex;
    OAuth::Client mClient;
};

} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_OAUTH_HTTP_MANAGER_HPP_
