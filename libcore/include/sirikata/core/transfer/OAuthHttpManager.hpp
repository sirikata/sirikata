// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_OAUTH_HTTP_MANAGER_HPP_
#define _SIRIKATA_LIBCORE_OAUTH_HTTP_MANAGER_HPP_

#include <sirikata/core/transfer/HttpManager.hpp>
#include <liboauthcpp/liboauthcpp.h>

namespace Sirikata {
namespace Transfer {

/** Wrapper around HttpManager which signs requests as an OAuth consumer. Unlike
 *  HttpManager, this is not a Singleton so that you can use this multiple times
 *  to be different consumers to different sites.
 */
class SIRIKATA_EXPORT OAuthHttpManager {
public:
    OAuthHttpManager(const String& hostname, const String& consumer_key, const String& consumer_secret);
    OAuthHttpManager(const String& hostname, const String& consumer_key, const String& consumer_secret, const String& token_key, const String& token_secret);

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

private:
    const String mHostname;
    OAuth::Consumer mConsumer;
    OAuth::Token mToken;
    OAuth::Client mClient;
};

} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_OAUTH_HTTP_MANAGER_HPP_
