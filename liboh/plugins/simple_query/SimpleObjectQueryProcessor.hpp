// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_SIMPLE_OBJECT_QUERY_PROCESSOR_HPP_
#define _SIRIKATA_OH_SIMPLE_OBJECT_QUERY_PROCESSOR_HPP_

#include <sirikata/oh/ObjectQueryProcessor.hpp>

#include <sirikata/pintoloc/OrphanLocUpdateManager.hpp>

namespace Sirikata {
namespace OH {
namespace Simple {

/** A simple implementation of ObjectQueryProcessor: it mostly acts as a pass
 *  through, only managing the coordination with the space server. Query updates
 *  are passed directly through and it only manages listening for new result
 *  streams and parsing the results.
 */
class SimpleObjectQueryProcessor :
        public ObjectQueryProcessor,
        OrphanLocUpdateManager::Listener
{
public:
    static SimpleObjectQueryProcessor* create(ObjectHostContext* ctx, const String& args);

    SimpleObjectQueryProcessor(ObjectHostContext* ctx);
    virtual ~SimpleObjectQueryProcessor();

    virtual void start();
    virtual void stop();

    virtual void presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, HostedObject::SSTStreamPtr strm);
    virtual void presenceDisconnected(HostedObjectPtr ho, const SpaceObjectReference& sporef);

    virtual void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query);


    // OrphanLocUpdateManager::Listener Interface
    virtual void onOrphanLocUpdate(const LocUpdate& lu, const SpaceObjectReference& observer);

private:
    void handleStop();

    // Proximity
    void handleProximitySubstream(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s);
    void handleProximitySubstreamRead(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, SSTStreamPtr s, String* prevdata, uint8* buffer, int length);
    bool handleProximityMessage(HostedObjectPtr self, const SpaceObjectReference& spaceobj, const std::string& payload);

    // Location
    // Handlers for substreams for space-managed updates
    void handleLocationSubstream(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s);
    // Handlers for substream read events for space-managed updates
    void handleLocationSubstreamRead(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);
    bool handleLocationMessage(const HostedObjectPtr& self, const SpaceObjectReference& spaceobj, const std::string& paylod);


    // BaseProxCommandable
    virtual void commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);


    ObjectHostContext* mContext;

    // We resolve ordering issues here instead of leaving it up to the
    // object. To do so, we track a bit of state for each query -- the
    // object so we can check it's ProxyManager for proxies (i.e. the
    // current query result state) and an OrphanLocUpdateManager for
    // fixing the ordering problems.
    struct ObjectState {
        ObjectState(Context* ctx, HostedObjectPtr _ho)
         : ho(_ho),
           orphans(ctx, ctx->mainStrand, Duration::seconds(10)),
           stopped(false)
        {
            orphans.start();
        }
        ~ObjectState() {
            stop();
        }

        void stop() {
            if (!stopped) {
                orphans.stop();
                stopped = true;
            }
        }

        HostedObjectWPtr ho;
        OrphanLocUpdateManager orphans;
        bool stopped;
    };
    typedef std::tr1::shared_ptr<ObjectState> ObjectStatePtr;
    typedef std::tr1::unordered_map<SpaceObjectReference, ObjectStatePtr, SpaceObjectReference::Hasher> ObjectStateMap;
    ObjectStateMap mObjectStateMap;
};

} // namespace Simple
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_SIMPLE_OBJECT_QUERY_PROCESSOR_HPP_
