// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_ENVIRONMENT_SPACE_MODULE_HPP_
#define _SIRIKATA_ENVIRONMENT_SPACE_MODULE_HPP_

#include <sirikata/space/SpaceModule.hpp>
#include <sirikata/space/ObjectSessionManager.hpp>

#include <json_spirit/value.h>

#include <sirikata/core/network/RecordSSTStream.hpp>

namespace Sirikata {

/** Environment is a SpaceModule which provides a simple service for maintaining
 *  shared, global state. It's useful for creating 'environment' settings for
 *  the space, e.g. a skybox, background music, fixed global lighting, etc. It
 *  represents the entire environment as a JSON-style object and handles
 *  subscriptions to keys within that object on a per-object basis. It isn't
 *  proactive -- it will only send updates to objects that have subscribed.
 *
 *  Beware: currently there are no controls on setting values!
 */
class Environment : public SpaceModule, public ObjectSessionListener {
public:
    Environment(SpaceContext* ctx);

    virtual void start();
    virtual void stop();

private:
    // ObjectSessionListener Interface
    virtual void newSession(ObjectSession* session);
    virtual void sessionClosed(ObjectSession *session);

    void handleStream(int err, ODPSST::Stream::Ptr strm);
    void handleMessage(const ObjectReference& id, MemoryReference data);

    void sendUpdate(const ObjectReference& id);

    // The environment data
    json_spirit::Value mEnvironment;
    // Subscribers
    struct SubscriberInfo {
        ODPSST::Stream::Ptr stream;
        RecordSSTStream<ODPSST::Stream::Ptr> record_stream;
    };
    typedef std::tr1::shared_ptr<SubscriberInfo> SubscriberInfoPtr;
    typedef std::tr1::unordered_map<ObjectReference, SubscriberInfoPtr, ObjectReference::Hasher> SubscriberMap;
    SubscriberMap mSubscribers;
};

} // namespace Sirikata

#endif //_SIRIKATA_ENVIRONMENT_SPACE_MODULE_HPP_
