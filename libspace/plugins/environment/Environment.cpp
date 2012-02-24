// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "Environment.hpp"
#include <json_spirit/json_spirit.h>

#define ENVIRONMENT_SERVICE_PORT 23
#define ENV_LOG(lvl,msg) SILOG(environment,lvl,msg);

namespace Sirikata {

namespace json = json_spirit;

using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;

Environment::Environment(SpaceContext* ctx)
 : SpaceModule(ctx),
   mEnvironment(json::Object())
{
}

void Environment::start() {
    mContext->objectSessionManager()->addListener(this);
}

void Environment::stop() {
    mContext->objectSessionManager()->removeListener(this);
}

void Environment::newSession(ObjectSession* session) {
    ODPSST::Stream::Ptr strm = session->getStream();
    strm->listenSubstream(
        ENVIRONMENT_SERVICE_PORT,
        std::tr1::bind(&Environment::handleStream, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)
    );
}

void Environment::sessionClosed(ObjectSession *session) {
    ODPSST::Stream::Ptr strm = session->getStream();
    if (strm) strm->unlistenSubstream(ENVIRONMENT_SERVICE_PORT);
}

void Environment::handleStream(int err, ODPSST::Stream::Ptr strm) {
    // Save stream and listen for requests
    ObjectReference id = strm->remoteEndPoint().endPoint.object();
    ENV_LOG(detailed, "Received connection for " << id);
    SubscriberInfoPtr sub(new SubscriberInfo());
    sub->stream = strm;
    sub->record_stream.initialize(
        strm,
        std::tr1::bind(&Environment::handleMessage, this, id, std::tr1::placeholders::_1)
    );
    mSubscribers[id] = sub;

    // Always immediately send a full update
    sendUpdate(id);
}

void Environment::handleMessage(const ObjectReference& id, MemoryReference data) {
    ENV_LOG(insane, "Received message from " << id << ": " << String((char*)data.begin(), data.size()));
    // Currently just receiving whole thing every time
    json::Value new_env;
    bool parsed = json::read( std::string((char*)data.begin(), data.size()), new_env);

    if (!parsed) {
        ENV_LOG(error, "Couldn't parse new environment: " << String((char*)data.begin(), data.size()));
        return;
    }

    mEnvironment = new_env;

    // And notifying everyone asap
    for(SubscriberMap::iterator it = mSubscribers.begin(); it != mSubscribers.end(); it++) {
        sendUpdate(it->first);
    }
}

void Environment::sendUpdate(const ObjectReference& id) {
    // Currently we just always serialize and send the whole thing
    String serialized = write(mEnvironment);
    ENV_LOG(insane, "Sending update to " << id << ": " << serialized);
    mSubscribers[id]->record_stream.write(MemoryReference(serialized));
}

} // namespace Sirikata
