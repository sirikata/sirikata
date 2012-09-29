// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MQ_SERVER_QUERY_HANDLER_HPP_
#define _SIRIKATA_OH_MQ_SERVER_QUERY_HANDLER_HPP_

#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/oh/SpaceNodeSession.hpp>
#include <sirikata/pintoloc/ManualReplicatedClient.hpp>
#include <sirikata/oh/OHSpaceTimeSynced.hpp>

namespace Sirikata {
namespace OH {
namespace Manual {

class ManualObjectQueryProcessor;

/** This class manages queries registered with servers. It accepts aggregated
 *  requests from the parent ManualObjectQueryProcessor, registers them with the
 *  server, and manages receiving results and forwarding them for further
 *  processing. It takes care of other details like orphan location updates so
 *  the parent just gets a stream of updates.
 */
class ServerQueryHandler :
        public SpaceNodeSessionListener,
        OrphanLocUpdateManager::Listener,
        Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID>::Parent
{
public:
    ServerQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, Network::IOStrandPtr strand);

    // Service Interface
    virtual void start();
    virtual void stop();


    // Helper that marks a server with another connected object and may register
    // a query
    void incrementServerQuery(OHDP::SpaceNodeID node);
    // Helper that tries to remove the server query pointed at by the
    // iterator, checking if it is referenced at all yet and sending a
    // message to kill the query
    void decrementServerQuery(OHDP::SpaceNodeID node);


    // ObjectQueryHandler callbacks - handle notifications about local queries
    // in the tree so we know how to move the cut on the space server up or down.
    void queriersAreObserving(const OHDP::SpaceNodeID& snid, const ProxIndexID indexid, const ObjectReference& objid);
    void queriersStoppedObserving(const OHDP::SpaceNodeID& snid, const ProxIndexID indexid, const ObjectReference& objid);
    void replicatedNodeRemoved(const OHDP::SpaceNodeID& snid, ProxIndexID indexid, const ObjectReference& objid);

private:
    ObjectHostContext* mContext;
    ManualObjectQueryProcessor* mParent;
    Network::IOStrandPtr mStrand;

    // Queries we've registered with servers so that we can resolve
    // object queries. Most of the work is performed by the ReplicatedClient,
    // and this just interacts with it and manages communication.
    struct ServerQueryState {
        ServerQueryState(ServerQueryHandler* parent_, const OHDP::SpaceNodeID& id_, ObjectHostContext* ctx_, Network::IOStrandPtr strand_, OHDPSST::StreamPtr base)
         : nconnected(0),
           sync(new OHSpaceTimeSynced(ctx_->objectHost, id_.space())),
           client(ctx_, strand_, parent_, sync, id_.toString(), id_),
           base_stream(base),
           prox_stream(),
           prox_stream_requested(false),
           outstanding(),
           writing(false)
        {
        }
        virtual ~ServerQueryState() {
            // DO NOT delete sync. See note about ownership below.
        }

        bool canRemove() const { return nconnected == 0; }

        int32 nconnected;
        // This sync is stored here and passed into client. client owns it, we
        // just use to to translate loc updates. Ideally we'd eventually make
        // ManualReplicatedClient work only with abstract values (i.e. Prox
        // updates would also be translated here) so we could just be the only
        // users of it.
        OHSpaceTimeSynced* sync;
        Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID> client;
        OHDPSST::StreamPtr base_stream;
        OHDPSST::StreamPtr prox_stream;
        // Whether we've requested the prox_stream
        bool prox_stream_requested;
        // Outstanding data to be sent
        std::queue<String> outstanding;
        // Whether we're in the process of sending messages
        bool writing;
    };
    typedef std::tr1::shared_ptr<ServerQueryState> ServerQueryStatePtr;
    typedef std::tr1::unordered_map<OHDP::SpaceNodeID, ServerQueryStatePtr, OHDP::SpaceNodeID::Hasher> ServerQueryMap;
    ServerQueryMap mServerQueries;

    // SpaceNodeSessionListener Interface
    virtual void onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::StreamPtr sn_stream);
    virtual void onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id);

    // ReplicatedClientWithID Interface
    virtual void onCreatedReplicatedIndex(Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID>* client, const OHDP::SpaceNodeID& snid, ProxIndexID proxid, ReplicatedLocationServiceCachePtr loccache, ServerID sid, bool dynamic_objects);
    virtual void onDestroyedReplicatedIndex(Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID>* client, const OHDP::SpaceNodeID& snid, ProxIndexID proxid);
    virtual void sendReplicatedClientProxMessage(Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID>* client, const OHDP::SpaceNodeID& snid, const String& msg);


    // Proximity

    // Send a message to prox, triggering new stream as necessary
    void sendProxMessage(ServerQueryMap::iterator serv_it, const String& msg);
    // Utility that triggers writing some more prox data. As long as more is
    // available, it'll keep looping.
    void writeSomeProxData(ServerQueryStatePtr data);
    // Callback from creating proximity substream
    void handleCreatedProxSubstream(const OHDP::SpaceNodeID& snid, int success, OHDPSST::StreamPtr prox_stream);
    // Data read callback for prox substreams -- translate to proximity events
    void handleProximitySubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::StreamPtr prox_stream, String* prevdata, uint8* buffer, int length);
    // Handle decode proximity message
    void handleProximityMessage(const OHDP::SpaceNodeID& snid, const String& payload);

    // Location
    // Handlers for substreams for space-managed updates
    void handleLocationSubstream(const OHDP::SpaceNodeID& snid, int err, OHDPSST::StreamPtr s);
    // Handlers for substream read events for space-managed updates
    void handleLocationSubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::StreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);
    bool handleLocationMessage(const OHDP::SpaceNodeID& snid, const std::string& payload);

};

} // namespace Manual
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_MQ_SERVER_QUERY_HANDLER_HPP_
