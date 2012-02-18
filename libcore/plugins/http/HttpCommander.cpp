// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "HttpCommander.hpp"

#include <boost/property_tree/json_parser.hpp>

#define HC_LOG(lvl, msg) SILOG(http-commander, lvl, msg)

namespace Sirikata {
namespace Command {


HttpCommander::HttpCommander(Context* ctx, const String& host, uint16 port)
 : mContext(ctx),
   mServer(ctx, host, port)
{
    mContext->commander = this;
    mServer.addListener(this);
}

HttpCommander::~HttpCommander() {
    mServer.removeListener(this);
    mContext->commander = NULL;
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
        Result result;
        result.put("error", String("Handler not found for ") + command_name);
        sendResponse(id, 404, result);
        return;
    }

    Command cmd;
    using namespace boost::property_tree;
    try {
        // The raw POST body as JSON
        if (!body.empty()) {
            std::stringstream cmd_json(body);
            read_json(cmd_json, cmd);
        }
    }
    catch(json_parser::json_parser_error exc) {
        // Failure to parse -> 400 error with message about parse failure
        HC_LOG(detailed, "Couldn't parse request body, returning 400");
        Result result;
        result.put("error", "Failed to parse body of request");
        sendResponse(id, 400, result);
        return;
        return;
    }

    CommandSetName(cmd, command_name);
    handler(cmd, this, id);
}

void HttpCommander::result(CommandID id, const Result& result) {
    sendResponse(id, 200, result);
}

void HttpCommander::sendResponse(HttpRequestID id, HttpStatus status, const Result& result) {
    using namespace boost::property_tree;
    try {
        std::stringstream data_json;
        write_json(data_json, result);
        HC_LOG(insane, "Sending successful response " << status);
        mServer.response(id, status, Headers(), data_json.str());
    }
    catch(json_parser::json_parser_error exc) {
        // Failed to encode, log the error and send a 500 instead
        HC_LOG(detailed, "Failed to encode response, returning 500");
        mServer.response(id, 500, Headers(), "");
    }
}

} // namespace Command
} // namespace Sirikata
