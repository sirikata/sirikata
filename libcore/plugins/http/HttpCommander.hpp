// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_HTTP_COMMANDER_HPP_
#define _SIRIKATA_LIBCORE_HTTP_COMMANDER_HPP_

#include <sirikata/core/command/Commander.hpp>
#include "HttpServer.hpp"

namespace Sirikata {
namespace Command {

class HttpCommander :
        public Commander,
        HttpRequestListener
{
public:
    HttpCommander(Context* ctx, const String& host, uint16 port);
    virtual ~HttpCommander();

    // HttpRequestListener Interface
    virtual void onHttpRequest(HttpServer* server, HttpRequestID id, String& path, String& query, String& fragment, Headers& headers, String& body);

    // Commander Interface
    virtual void result(CommandID id, const Result& result);

private:
    // Encode and send a response. If something goes wrong with encoding, sends
    // an error code instead.
    void sendResponse(HttpRequestID id, HttpStatus status, const Result& result);


    Context* mContext;
    HttpServer mServer;
}; // class HttpCommander

} // namespace Command
} // namespace Sirikata


#endif //_SIRIKATA_LIBCORE_HTTP_COMMANDER_HPP_
