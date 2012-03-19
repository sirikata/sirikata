// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "HttpCommander.hpp"

#define HC_LOG(lvl, msg) SILOG(http-commander, lvl, msg)

namespace Sirikata {
namespace Command {


HttpCommander::HttpCommander(Context* ctx, const String& host, uint16 port)
 : mContext(ctx),
   mServer(ctx, host, port)
{
    mContext->setCommander(this);
    mServer.addListener(this);
}

HttpCommander::~HttpCommander() {
    mServer.removeListener(this);
    mContext->setCommander(NULL);
}


void HttpCommander::onHttpRequest(HttpServer* server, HttpRequestID id, String& path, String& query, String& fragment, Headers& headers, String& body) {
    // We treat the path directly as the notation. For example, we'd expect the
    // path /space.forward.stats for the command space.forwarder.stats. We just
    // need to pick off the initial '/'.
    String command_name = path.substr(1);
    CommandHandler handler = getHandler(command_name);
    if (!handler) {
        // Generate a 404 with a nice message if we can't find a handler
        HC_LOG(detailed, "Couldn't find handler for " << command_name << ", returning 404");
        Result result= EmptyResult();
        result.put("error", String("Handler not found for ") + command_name);
        sendResponse(id, 404, result);
        return;
    }

    Command cmd;
    namespace json = json_spirit;

    // The raw POST body as JSON
    if (!body.empty()) {
        if (!json::read(body, cmd)) {
            // Failure to parse -> 400 error with message about parse failure
            HC_LOG(detailed, "Couldn't parse request body, returning 400");
            Result result = EmptyResult();
            result.put("error", "Failed to parse body of request");
            sendResponse(id, 400, result);
            return;
        }
    }
    else {
        // Empty object if nothing was sent.
        cmd = Command::Object();
    }

    CommandSetName(cmd, command_name);
    handler(cmd, this, id);
}

void HttpCommander::result(CommandID id, const Result& result) {
    sendResponse(id, 200, result);
}

void HttpCommander::sendResponse(HttpRequestID id, HttpStatus status, const Result& result) {
    namespace json = json_spirit;

    HC_LOG(insane, "Sending successful response " << status);
    String serialized_result = json::write(result);
    mServer.response(id, status, Headers(), serialized_result);
}

} // namespace Command
} // namespace Sirikata
