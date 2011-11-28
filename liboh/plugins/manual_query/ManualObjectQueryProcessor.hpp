// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_
#define _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_

#include <sirikata/oh/ObjectQueryProcessor.hpp>
#include <sirikata/oh/SpaceNodeSession.hpp>
#include <sirikata/oh/ObjectNodeSession.hpp>

#include <sirikata/proxyobject/OrphanLocUpdateManager.hpp>

namespace Sirikata {
namespace OH {
namespace Manual {

/** An implementation of ObjectQueryProcessor that manually manages a cut on the
 *  space servers' data structures, creates and manages a local replica of the
 *  servers' data structure, and locally executes queries over that data
 *  structure.
 */
class ManualObjectQueryProcessor :
        public ObjectQueryProcessor,
        public SpaceNodeSessionListener,
        public ObjectNodeSessionListener,
        OrphanLocUpdateManager::Listener<OHDP::SpaceNodeID>
{
public:
    static ManualObjectQueryProcessor* create(ObjectHostContext* ctx, const String& args);

    ManualObjectQueryProcessor(ObjectHostContext* ctx);
    virtual ~ManualObjectQueryProcessor();

    virtual void start();
    virtual void stop();

    // SpaceNodeSessionListener Interface
    virtual void onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream);
    virtual void onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id);

    // ObjectNodeSessionListener Interface
    virtual void onObjectNodeSession(const SpaceID& space, const ObjectReference& oref, const OHDP::NodeID& id);

    virtual void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query);

private:

    ObjectHostContext* mContext;

    // Queries registered by objects to be resolved locally and
    // connection state
    struct ObjectState {
        ObjectState()
         : node(OHDP::NodeID::null()),
           query()
        {}

        // Checks if it is safe to destroy this ObjectState
        bool canRemove() const {
            return (node == OHDP::NodeID::null());
        }

        OHDP::NodeID node;
        String query;
    };
    typedef std::tr1::unordered_map<SpaceObjectReference, ObjectState, SpaceObjectReference::Hasher> ObjectStateMap;
    ObjectStateMap mObjectState;

    // Queries we've registered with servers so that we can resolve
    // object queries
    struct ServerQueryState {
        ServerQueryState(Context* ctx, OHDPSST::Stream::Ptr base)
         : nconnected(0),
           base_stream(base),
           prox_stream(),
           prox_stream_requested(false),
           outstanding(),
           writing(false),
           orphans(ctx, ctx->mainStrand, Duration::seconds(10))
        {
            orphans.start();
        }
        ~ServerQueryState() {
            orphans.stop();
        }

        // Returns true if the *query* can be removed (not if this
        // object can be removed, that is tracked by the lifetime of
        // the session)
        bool canRemove() const {
            return nconnected == 0;
        }

        // # of connected objects
        int32 nconnected;
        OHDPSST::Stream::Ptr base_stream;
        OHDPSST::Stream::Ptr prox_stream;
        // Whether we've requested the prox_stream
        bool prox_stream_requested;
        // Outstanding data to be sent
        std::queue<String> outstanding;
        // Whether we're in the process of sending messages
        bool writing;

        // Each server query has an independent stream of results +
        // loc updates, so each gets its own set of objects w/ properties and
        // orphan tracking
        typedef std::tr1::unordered_map<SpaceObjectReference, SequencedPresenceProperties, SpaceObjectReference::Hasher> ObjectPropertiesMap;
        ObjectPropertiesMap objects;
        OrphanLocUpdateManager orphans;
    };
    typedef std::tr1::shared_ptr<ServerQueryState> ServerQueryStatePtr;
    typedef std::tr1::unordered_map<OHDP::SpaceNodeID, ServerQueryStatePtr, OHDP::SpaceNodeID::Hasher> ServerQueryMap;
    ServerQueryMap mServerQueries;

    // Helper that marks a server with another connected object and may register
    // a query
    void incrementServerQuery(ServerQueryMap::iterator serv_it);
    // Helper that updates a server query, possibly sending an
    // initialization message to the server to setup the query
    void updateServerQuery(ServerQueryMap::iterator serv_it, bool is_new);
    // Helper that tries to remove the server query pointed at by the
    // iterator, checking if it is referenced at all yet and sending a
    // message to kill the query
    void decrementServerQuery(ServerQueryMap::iterator serv_it);


    // Proximity
    // Helpers for sending different types of basic requests
    void sendInitRequest(ServerQueryMap::iterator serv_it);
    void sendRefineRequest(ServerQueryMap::iterator serv_it, const ObjectReference& agg);
    void sendRefineRequest(ServerQueryMap::iterator serv_it, const std::vector<ObjectReference>& aggs);
    void sendCoarsenRequest(ServerQueryMap::iterator serv_it, const ObjectReference& agg);
    void sendCoarsenRequest(ServerQueryMap::iterator serv_it, const std::vector<ObjectReference>& aggs);
    void sendDestroyRequest(ServerQueryMap::iterator serv_it);

    // Send a message to prox, triggering new stream as necessary
    void sendProxMessage(ServerQueryMap::iterator serv_it, const String& msg);
    // Utility that triggers writing some more prox data. As long as more is
    // available, it'll keep looping.
    void writeSomeProxData(ServerQueryStatePtr data);
    // Callback from creating proximity substream
    void handleCreatedProxSubstream(const OHDP::SpaceNodeID& snid, int success, OHDPSST::Stream::Ptr prox_stream);
    // Data read callback for prox substreams -- translate to proximity events
    void handleProximitySubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr prox_stream, String* prevdata, uint8* buffer, int length);
    // Handle decode proximity message
    void handleProximityMessage(const OHDP::SpaceNodeID& snid, const String& payload);

    // Location
    // Handlers for substreams for space-managed updates
    void handleLocationSubstream(const OHDP::SpaceNodeID& snid, int err, OHDPSST::Stream::Ptr s);
    // Handlers for substream read events for space-managed updates
    void handleLocationSubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr s, std::stringstream* prevdata, uint8* buffer, int length);
    bool handleLocationMessage(const OHDP::SpaceNodeID& snid, const std::string& payload);

    // OrphanLocUpdateManager::Listener Interface
    virtual void onOrphanLocUpdate(const OHDP::SpaceNodeID& observer, const LocUpdate& lu);
};

} // namespace Manual
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_
