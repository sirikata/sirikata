// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_
#define _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_

#include <sirikata/oh/ObjectQueryProcessor.hpp>
#include <sirikata/oh/ObjectNodeSession.hpp>

#include "OHLocationServiceCache.hpp"
#include "ServerQueryHandler.hpp"
#include "ObjectQueryHandler.hpp"

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
        public ObjectNodeSessionListener
{
public:
    static ManualObjectQueryProcessor* create(ObjectHostContext* ctx, const String& args);

    ManualObjectQueryProcessor(ObjectHostContext* ctx);
    virtual ~ManualObjectQueryProcessor();

    virtual void start();
    virtual void stop();

    // ObjectNodeSessionListener Interface
    virtual void onObjectNodeSession(const SpaceID& space, const ObjectReference& oref, const OHDP::NodeID& id);


    // ObjectQueryProcessor Overrides
    virtual void presenceConnected(HostedObjectPtr ho, const SpaceObjectReference& sporef);
    virtual void presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, SSTStreamPtr strm);
    virtual void presenceDisconnected(HostedObjectPtr ho, const SpaceObjectReference& sporef);
    virtual String connectRequest(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& query);
    virtual void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query);


    // ServerQueryHandler callbacks - Handle new/deleted queries
    // Notification when a new server query is setup. This occurs whether or not
    // a query is registered -- it just indicates that there's a connection that
    // we might care about, specifically that the OHLocationServiceCachePtr
    // exists, which allows us to setup the local query processor.
    void createdServerQuery(const OHDP::SpaceNodeID& snid, OHLocationServiceCachePtr loc_cache);
    void removedServerQuery(const OHDP::SpaceNodeID& snid);


    // ObjectQueryProcessor callbacks - Handle results coming back for queries
    void deliverProximityResult(const SpaceObjectReference& sporef, const Sirikata::Protocol::Prox::ProximityUpdate& update);
    void deliverLocationResult(const SpaceObjectReference& sporef, const LocUpdate& lu);
private:

    // Helper that actually registers a query with the underlying query
    // processor. Factored out since we may need to defer registration until
    // after connection completes, so we can hit this from multiple code paths.
    void registerOrUpdateObjectQuery(const SpaceObjectReference& sporef);
    void unregisterObjectQuery(const SpaceObjectReference& sporef);

    ObjectHostContext* mContext;
    Network::IOStrandPtr mStrand;

    // Queries registered by objects to be resolved locally and
    // connection state
    struct ObjectState {
        ObjectState()
         : who(),
           node(OHDP::NodeID::null()),
           query(),
           registered(false)
        {}

        // Checks if it is safe to destroy this ObjectState
        bool canRemove() const {
            return (node == OHDP::NodeID::null());
        }

        // Checks whether registration is needed, i.e. we have a query and
        // haven't registered it yet
        bool needsRegistration() const {
            return (!registered && !query.empty());
        }

        // Checks whether registration is currently possible, i.e. we're
        // actually connected.
        bool canRegister() const {
            return (node != OHDP::NodeID::null() && !query.empty());
        }


        HostedObjectWPtr who;
        OHDP::NodeID node;
        String query;
        // Whether we've registered this query with the underlying query
        // processor. This requires the query processor and a fully connected
        // object (including connected space SST stream).
        bool registered;
    };
    // This stores state about requests from objects we are hosting
    typedef std::tr1::unordered_map<SpaceObjectReference, ObjectState, SpaceObjectReference::Hasher> ObjectStateMap;
    ObjectStateMap mObjectState;

    // This actually performs the queries from each object
    typedef std::tr1::shared_ptr<ObjectQueryHandler> ObjectQueryHandlerPtr;
    typedef std::tr1::unordered_map<OHDP::SpaceNodeID, ObjectQueryHandlerPtr, OHDP::SpaceNodeID::Hasher> QueryHandlerMap;
    QueryHandlerMap mObjectQueryHandlers;

    // And this performs a single query against each server, getting results
    // locally that allow the above to process local queries.
    ServerQueryHandler mServerQueryHandler;


};

} // namespace Manual
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_
