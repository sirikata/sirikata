// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/transfer/OAuthHttpManager.hpp>

namespace Sirikata {
namespace Transfer {

void OAuthHttpManager::head(
    Sirikata::Network::Address addr, const String& path,
    HttpManager::HttpCallback cb,
    const HttpManager::Headers& headers, const HttpManager::QueryParameters& query_params,
    bool allow_redirects
) {
    HttpManager::Headers new_headers(headers);
    if (new_headers.find("Host") == new_headers.end())
        new_headers["Host"] = mOAuth->hostname;
    {
        boost::unique_lock<boost::mutex> lockQueue(mMutex);
        new_headers["Authorization"] =
            mClient.getHttpHeader(OAuth::Http::Head, HttpManager::formatURL(mOAuth->hostname, mOAuth->service, path, query_params));
    }
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
        new_headers["Host"] = mOAuth->hostname;
    {
        boost::unique_lock<boost::mutex> lockQueue(mMutex);
        new_headers["Authorization"] =
            mClient.getHttpHeader(OAuth::Http::Get, HttpManager::formatURL(mOAuth->hostname, mOAuth->service, path, query_params));
    }
    HttpManager::getSingleton().get(
        addr, path, cb, new_headers, query_params, allow_redirects
    );
}


void OAuthHttpManager::post(
    Sirikata::Network::Address addr, const String& path,
    const String& content_type, const String& body,
    HttpManager::HttpCallback cb, const HttpManager::Headers& headers, const HttpManager::QueryParameters& query_params,
    bool allow_redirects)
{
    HttpManager::Headers new_headers(headers);
    if (new_headers.find("Host") == new_headers.end())
        new_headers["Host"] = mOAuth->hostname;

    // We always compute over the query args, but only compute over
    // the POST body if it is x-www-form-urlencoded
    String post_params_str = "";
    if (new_headers.find("Content-Type") != new_headers.end() && new_headers["Content-Type"] == "application/x-www-form-urlencoded")
        post_params_str = body;

    {
        boost::unique_lock<boost::mutex> lockQueue(mMutex);
        new_headers["Authorization"] =
            mClient.getHttpHeader(OAuth::Http::Post, HttpManager::formatURL(mOAuth->hostname, mOAuth->service, path, query_params), post_params_str);
    }
    HttpManager::getSingleton().post(
        addr, path, content_type, body, cb, new_headers, query_params, allow_redirects
    );
}

void OAuthHttpManager::postURLEncoded(
    Sirikata::Network::Address addr, const String& path,
    const HttpManager::StringDictionary& body,
    HttpManager::HttpCallback cb, const HttpManager::Headers& headers, const HttpManager::QueryParameters& query_params,
    bool allow_redirects)
{
    HttpManager::Headers new_headers(headers);
    if (new_headers.find("Host") == new_headers.end())
        new_headers["Host"] = mOAuth->hostname;
    // We know this is going to end up x-www-form-urlencoded. We just
    // need to get the data into the signature (either add to
    // query_params *just for the signing* or generate the urlencoded
    // form of the dictionary that will become the body of the
    // request). Here, we just generate the encoded body contents.
    {
        boost::unique_lock<boost::mutex> lockQueue(mMutex);
        new_headers["Authorization"] =
            mClient.getHttpHeader(OAuth::Http::Post, HttpManager::formatURL(mOAuth->hostname, mOAuth->service, path, query_params), HttpManager::formatURLEncodedDictionary(body));
    }
    HttpManager::getSingleton().postURLEncoded(
        addr, path, body, cb, new_headers, query_params, allow_redirects
    );
}

void OAuthHttpManager::postMultipartForm(
    Sirikata::Network::Address addr, const String& path,
    const HttpManager::MultipartDataList& data,
    HttpManager::HttpCallback cb, const HttpManager::Headers& headers, const HttpManager::QueryParameters& query_params,
    bool allow_redirects)
{
    HttpManager::Headers new_headers(headers);
    if (new_headers.find("Host") == new_headers.end())
        new_headers["Host"] = mOAuth->hostname;
    // Signed only over query arguments -- because this is multipart
    // OAuth doesn't do any signing over its contents.
    // FIXME we could support the body_hash oauth extension
    {
        boost::unique_lock<boost::mutex> lockQueue(mMutex);
        new_headers["Authorization"] =
            mClient.getHttpHeader(OAuth::Http::Post, HttpManager::formatURL(mOAuth->hostname, mOAuth->service, path, query_params));
    }
    HttpManager::getSingleton().postMultipartForm(
        addr, path, data, cb, new_headers, query_params, allow_redirects
    );
}


} // namespace Transfer
} // namespace Sirikata
