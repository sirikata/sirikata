// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_OAUTH_PARAMS_HPP_
#define _SIRIKATA_LIBCORE_OAUTH_PARAMS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <liboauthcpp/liboauthcpp.h>

namespace Sirikata {
namespace Transfer {

/** Immutable collection of parameters used for signing oauth requests. */
class SIRIKATA_EXPORT OAuthParams {
  public:
    OAuthParams(const String& hostname_, const String& service_, const String& consumer_key, const String& consumer_secret)
     : hostname(hostname_), service(service_),
       consumer(consumer_key, consumer_secret),
       token("", "")
    {}

    OAuthParams(const String& hostname_, const String& service_, const String& consumer_key, const String& consumer_secret, const String& token_key, const String& token_secret)
     : hostname(hostname_), service(service_),
       consumer(consumer_key, consumer_secret),
       token(token_key, token_secret)
    {}

    const OAuth::Consumer* consumerPtr() const { return &consumer; }
    const OAuth::Token* tokenPtr() const { return (token.key().empty() && token.secret().empty()) ? NULL : &token; }

    const String hostname;
    const String service;
    const OAuth::Consumer consumer;
    const OAuth::Token token;
};
typedef std::tr1::shared_ptr<OAuthParams> OAuthParamsPtr;

} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_OAUTH_HTTP_MANAGER_HPP_
