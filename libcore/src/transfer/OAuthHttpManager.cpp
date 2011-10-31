// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/transfer/OAuthHttpManager.hpp>

namespace Sirikata {
namespace Transfer {

OAuthHttpManager::OAuthHttpManager(const String& hostname, const String& consumer_key, const String& consumer_secret)
 : mHostname(hostname),
   mConsumer(consumer_key, consumer_secret),
   mToken("", ""),
   mClient(&mConsumer)
{
}

OAuthHttpManager::OAuthHttpManager(const String& hostname, const String& consumer_key, const String& consumer_secret, const String& token_key, const String& token_secret)
 : mHostname(hostname),
   mConsumer(consumer_key, consumer_secret),
   mToken(token_key, token_secret),
   mClient(&mConsumer, &mToken)
{
}

void OAuthHttpManager::head(
    Sirikata::Network::Address addr, const String& path,
    HttpManager::HttpCallback cb,
    const HttpManager::Headers& headers, const HttpManager::QueryParameters& query_params,
    bool allow_redirects
) {
    HttpManager::Headers new_headers(headers);
    if (new_headers.find("Host") == new_headers.end())
        new_headers["Host"] = mHostname;
    new_headers["Authorization"] =
        mClient.getHttpHeader(OAuth::Http::Head, HttpManager::formatURL(mHostname, path, query_params));
    HttpManager::getSingleton().head(
        addr, path, cb, new_headers, query_params, allow_redirects
    );
}

void OAuthHttpManager::get(
    Sirikata::Network::Address addr, const String& path,
    HttpManager::HttpCallback cb,
    const HttpManager::Headers& headers, const HttpManager::QueryParameters& query_params,
    bool allow_redirects
) {
    HttpManager::Headers new_headers(headers);
    if (new_headers.find("Host") == new_headers.end())
        new_headers["Host"] = mHostname;
    new_headers["Authorization"] =
        mClient.getHttpHeader(OAuth::Http::Get, HttpManager::formatURL(mHostname, path, query_params));
    HttpManager::getSingleton().get(
        addr, path, cb, new_headers, query_params, allow_redirects
    );
}

} // namespace Transfer
} // namespace Sirikata
