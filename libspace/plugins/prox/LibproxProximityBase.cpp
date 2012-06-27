// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "LibproxProximityBase.hpp"

#include <sirikata/core/network/Frame.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/space/AggregateManager.hpp>

#include <sirikata/core/command/Commander.hpp>

#define PROXLOG(level,msg) SILOG(prox,level,msg)

namespace Sirikata {

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::readFramesFromStream(Ptr prox_stream, FrameReceivedCallback cb) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    assert(iostream);
    read_frame_cb = cb;
    iostream->registerReadCallback(
        std::tr1::bind(
            &LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::handleRead,
            WPtr(prox_stream), _1, _2
        )
    );
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::handleRead(WPtr w_prox_stream, uint8* data, int size) {
    Ptr prox_stream = w_prox_stream.lock();
    if (!prox_stream) return;

    prox_stream->partial_frame.append((const char*)data, size);
    while(true) {
        String parsed = Network::Frame::parse(prox_stream->partial_frame);
        if (parsed.empty()) return;
        prox_stream->read_frame_cb(parsed);
    }
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::writeSomeObjectResults(Context* ctx, WPtr w_prox_stream) {
    static Duration retry_rate = Duration::milliseconds((int64)1);

    Ptr prox_stream = w_prox_stream.lock();
    if (!prox_stream) return;

    prox_stream->writing = true;

    if (!prox_stream->iostream) {
        // We're still waiting on the iostream, proxSubstreamCallback will call
        // us when it gets the iostream.
        prox_stream->writing = false;
        return;
    }

    // Otherwise, keep sending until we run out or
    while(!prox_stream->outstanding.empty()) {
        std::string& framed_prox_msg = prox_stream->outstanding.front();
        int bytes_written = prox_stream->iostream->write((const uint8*)framed_prox_msg.data(), framed_prox_msg.size());
        if (bytes_written < 0) {
            // FIXME
            break;
        }
        else if (bytes_written < (int)framed_prox_msg.size()) {
            framed_prox_msg = framed_prox_msg.substr(bytes_written);
            break;
        }
        else {
            prox_stream->outstanding.pop();
        }
    }

    if (prox_stream->outstanding.empty())
        prox_stream->writing = false;
    else
        ctx->mainStrand->post(
            retry_rate, prox_stream->writecb,
            "LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::writeSomeObjectResults"
        );
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::requestProxSubstream(LibproxProximityBase* parent, Context* ctx, const EndpointType& ep, Ptr prox_stream) {
    using std::tr1::placeholders::_1;

    // Always mark this up front. This keeps further requests from
    // occuring since the first time this method is entered, even if
    // the request is deferred, should eventually result in a stream.
    prox_stream->iostream_requested = true;

    // We need to check for a valid session here. This can be necessary because
    // we may have gotten a base session, registered a query, gotten results,
    // started to try returning them and had to wait for the base stream to the
    // object, and then had it disconnect before it ever got there. Then we'd be
    // stuck in an infinite loop, checking for the base stream and failing to
    // find it, then posting a retry.  validSession should only check for a
    // still-active connection, not the stream, so it should only kill this
    // process in this unusual case of a very short lived connection to the
    // space.
    if (!parent->validSession(ep)) return;

    StreamTypePtr base_stream = parent->getBaseStream(ep);
    if (!base_stream) {
        ctx->mainStrand->post(
            Duration::milliseconds((int64)5),
            std::tr1::bind(&LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::requestProxSubstream, parent, ctx, ep, prox_stream),
            "LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::requestProxSubstream"
        );
        return;
    }

    base_stream->createChildStream(
        ctx->mainStrand->wrap(
            std::tr1::bind(&LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::proxSubstreamCallback, parent, ctx, _1, ep, base_stream, _2, prox_stream)
        ),
        (void*)NULL, 0,
        OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
    );
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::proxSubstreamCallback(LibproxProximityBase* parent, Context* ctx, int x, const EndpointType& ep, StreamTypePtr parent_stream, StreamTypePtr substream, Ptr prox_stream_info) {
    if (!substream) {
        // If they disconnected, ignore
        if (!parent->validSession(ep)) return;

        // Retry
        PROXLOG(warn,"Error opening Prox substream, retrying...");

        parent_stream->createChildStream(
            ctx->mainStrand->wrap(
                std::tr1::bind(&LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::proxSubstreamCallback, parent, ctx, _1, ep, parent_stream, _2, prox_stream_info)
            ),
            (void*)NULL, 0,
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
        );

        return;
    }

    prox_stream_info->iostream = substream;
    assert(!prox_stream_info->writing);
    ProxStreamInfo::writeSomeObjectResults(ctx, prox_stream_info);
}




LibproxProximityBase::LibproxProximityBase(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : Proximity(ctx, locservice, cseg, net, aggmgr, Duration::milliseconds((int64)100)),
   mServerQuerier(NULL),
   mProxStrand(ctx->ioService->createStrand("LibproxProximityBase Prox Strand")),
   mLocCache(NULL)
{
    mProxServerMessageService = mContext->serverRouter()->createServerMessageService("proximity");

    // Location cache, for both types of queries
    mLocCache = new CBRLocationServiceCache(mProxStrand, locservice, true);

    // Server Querier (discover other servers)
    String pinto_type = GetOptionValue<String>(OPT_PINTO);
    String pinto_options = GetOptionValue<String>(OPT_PINTO_OPTIONS);
    mServerQuerier = PintoServerQuerierFactory::getSingleton().getConstructor(pinto_type)(mContext, pinto_options);
    mServerQuerier->addListener(this);

    // Deal with static/dynamic split
    mSeparateDynamicObjects = GetOptionValue<bool>(OPT_PROX_SPLIT_DYNAMIC);
    mNumQueryHandlers = (mSeparateDynamicObjects ? 2 : 1);
    mMoveToStaticDelay = Duration::minutes(1);

    // Implementations may add more commands, but these should always be
    // available. They get dispatched to the prox strand so implementations only
    // need to worry about processing them.
    if (mContext->commander()) {
        // Get basic properties (both fixed and dynamic debugging
        // state) about this query processor.
        mContext->commander()->registerCommand(
            "space.prox.properties",
            mProxStrand->wrap(
                std::tr1::bind(&LibproxProximityBase::commandProperties, this, _1, _2, _3)
            )
        );

        // Get a list of the handlers by name and their basic properties. The
        // particular names and properties may be implementation dependent.
        mContext->commander()->registerCommand(
            "space.prox.handlers",
            mProxStrand->wrap(
                std::tr1::bind(&LibproxProximityBase::commandListHandlers, this, _1, _2, _3)
            )
        );

        // Get a list of nodes within one of the handlers. Must specify the
        // handler name as part of the request
        mContext->commander()->registerCommand(
            "space.prox.nodes",
            mProxStrand->wrap(
                std::tr1::bind(&LibproxProximityBase::commandListNodes, this, _1, _2, _3)
            )
        );

        // Force a rebuild on one of the handlers. Must specify the handler name
        // as part of the request.
        mContext->commander()->registerCommand(
            "space.prox.rebuild",
            mProxStrand->wrap(
                std::tr1::bind(&LibproxProximityBase::commandForceRebuild, this, _1, _2, _3)
            )
        );
    }
}

LibproxProximityBase::~LibproxProximityBase() {
    delete mServerQuerier;
    delete mProxServerMessageService;
    delete mLocCache;
    delete mProxStrand;
}

// Service Interface overrides
void LibproxProximityBase::start() {
    Proximity::start();

    // Always initialize with CSeg's current size
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = aggregateBBoxes(bboxes);
    mServerQuerier->updateRegion(bbox);
}

void LibproxProximityBase::stop() {
    Proximity::stop();
}


const String& LibproxProximityBase::ObjectClassToString(ObjectClass c) {
    static String static_ = "static";
    static String dynamic_ = "dynamic";
    static String unknown_ = "unknown";

    switch(c) {
      case OBJECT_CLASS_STATIC: return static_; break;
      case OBJECT_CLASS_DYNAMIC: return dynamic_; break;
      default: return unknown_; break;
    }
}

BoundingBox3f LibproxProximityBase::aggregateBBoxes(const BoundingBoxList& bboxes) {
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);
    return bbox;
}

bool LibproxProximityBase::velocityIsStatic(const Vector3f& vel) {
    // These values are arbitrary, just meant to indicate that the object is,
    // for practical purposes, not moving.
    return (vel.lengthSquared() < (0.01f*0.01f));
}


void LibproxProximityBase::coalesceEvents(QueryEventList& evts, uint32 per_event) {
    if (evts.empty()) return;

    // We keep two maps from UUID to QueryEvent:
    // One for additions and one for removals. For each object, we check if we
    // have a record of it, in the opposite map. If we have a record, this new
    // one negates it and it's removed. If we don't, we add it into the
    // map. By processing in order, this leaves us with just the new additions
    // and removals.
    typedef std::map<UUID, QueryEvent::Addition> AdditionMap;
    typedef std::map<UUID, QueryEvent::Removal> RemovalMap;
    AdditionMap additions;
    RemovalMap removals;
    Prox::QueryHandlerIndexID qhiid = evts.front().indexID();
    while(!evts.empty()) {
        const QueryEvent& evt = evts.front();
        assert(qhiid == evt.indexID());

        for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
            UUID objid = evt.additions()[aidx].id();
            if (removals.find(objid) != removals.end())
                removals.erase(objid);
            else
                additions.insert(std::make_pair(objid, evt.additions()[aidx]));
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            UUID objid = evt.removals()[ridx].id();
            if (additions.find(objid) != additions.end())
                additions.erase(objid);
            else
                removals.insert(std::make_pair(objid, evt.removals()[ridx]));
        }
        evts.pop_front();
    }
    // Now we just need to repack them.
    QueryEvent next_evt(qhiid);
    while(!additions.empty() || !removals.empty()) {
        // Get next addition or removal and remove it from our list
        if (!additions.empty()) {
            next_evt.additions().push_back(additions.begin()->second);
            additions.erase(additions.begin());
        }
        else {
            next_evt.removals().push_back(removals.begin()->second);
            removals.erase(removals.begin());
        }

        // Push it onto the queue if this event is full or we're about to finish
        if (next_evt.size() == per_event ||
            (additions.empty() && removals.empty()))
        {
            evts.push_back(next_evt);
            next_evt = QueryEvent(qhiid);
        }
    }
}

// Setup all known servers for a server query update
void LibproxProximityBase::addAllServersForUpdate() {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    for(ServerSet::const_iterator it = mServersQueried.begin(); it != mServersQueried.end(); it++)
        mNeedServerQueryUpdate.insert(*it);
}



void LibproxProximityBase::getServersForAggregateQueryUpdate(ServerSet* servers_out) {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    servers_out->swap(mNeedServerQueryUpdate);
}

void LibproxProximityBase::addServerForAggregateQueryUpdate(ServerID sid) {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    mNeedServerQueryUpdate.insert(sid);
}

void LibproxProximityBase::updateAggregateQuery(const SolidAngle sa, uint32 max_count) {
    PROXLOG(debug,"Query addition initiated server query request.");
    addAllServersForUpdate();
    mServerQuerier->updateQuery(sa, max_count);
}

void LibproxProximityBase::updateAggregateStats(float32 max_radius) {
    mServerQuerier->updateLargestObject(max_radius);
}

uint32 LibproxProximityBase::numServersQueried() {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    return mServersQueried.size();
}

// MAIN Thread

// PintoServerQuerierListener Interface

void LibproxProximityBase::addRelevantServer(ServerID sid) {
    if (sid == mContext->id()) return;

    // Potentially invoked from PintoServerQuerier IO thread
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    mServersQueried.insert(sid);
    mNeedServerQueryUpdate.insert(sid);
}

void LibproxProximityBase::removeRelevantServer(ServerID sid) {
    if (sid == mContext->id()) return;

    // Potentially invoked from PintoServerQuerier IO thread
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    mServersQueried.erase(sid);
}


// SpaceNetworkConnectionListener Interface

void LibproxProximityBase::onSpaceNetworkConnected(ServerID sid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximityBase::handleConnectedServer, this, sid),
        "LibproxProximityBase::handleConnectedServer"
    );
}

void LibproxProximityBase::onSpaceNetworkDisconnected(ServerID sid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximityBase::handleDisconnectedServer, this, sid),
        "LibproxProximityBase::handleDisconnectedServer"
    );
}


// CoordinateSegmentation::Listener Interface

void LibproxProximityBase::updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg) {
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = aggregateBBoxes(bboxes);
    mServerQuerier->updateRegion(bbox);
}


void LibproxProximityBase::readFramesFromObjectStream(const ObjectReference& oref, ProxObjectStreamInfo::FrameReceivedCallback cb) {
    ObjectProxStreamMap::iterator prox_stream_it = mObjectProxStreams.find(oref.getAsUUID());
    assert(prox_stream_it != mObjectProxStreams.end());
    prox_stream_it->second->readFramesFromStream(prox_stream_it->second, cb);
}

void LibproxProximityBase::readFramesFromObjectHostStream(const OHDP::NodeID& node, ProxObjectHostStreamInfo::FrameReceivedCallback cb) {
    ObjectHostProxStreamMap::iterator prox_stream_it = mObjectHostProxStreams.find(node);
    assert(prox_stream_it != mObjectHostProxStreams.end());
    prox_stream_it->second->readFramesFromStream(prox_stream_it->second, cb);
}

void LibproxProximityBase::sendObjectResult(Sirikata::Protocol::Object::ObjectMessage* msg) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // Try to find stream info for the object
    ObjectProxStreamMap::iterator prox_stream_it = mObjectProxStreams.find(msg->dest_object());
    if (prox_stream_it == mObjectProxStreams.end()) {
        prox_stream_it = mObjectProxStreams.insert( ObjectProxStreamMap::value_type(msg->dest_object(), ProxObjectStreamInfoPtr(new ProxObjectStreamInfo())) ).first;
        prox_stream_it->second->writecb = std::tr1::bind(
            &LibproxProximityBase::ProxObjectStreamInfo::writeSomeObjectResults, mContext, ProxObjectStreamInfo::WPtr(prox_stream_it->second)
        );
    }
    ProxObjectStreamInfoPtr prox_stream = prox_stream_it->second;

    // If we don't have a stream yet, try to build it
    if (!prox_stream->iostream_requested)
        ProxObjectStreamInfo::requestProxSubstream(this, mContext, ObjectReference(msg->dest_object()), prox_stream);

    // Build the result and push it into the stream
    // FIXME this is an infinite sized queue, but we don't really want to drop
    // proximity results....
    prox_stream->outstanding.push(Network::Frame::write(msg->payload()));

    // If writing isn't already in progress, start it up
    if (!prox_stream->writing)
        ProxObjectStreamInfo::writeSomeObjectResults(mContext, prox_stream);
}

void LibproxProximityBase::sendObjectHostResult(const OHDP::NodeID& node, Sirikata::Protocol::Object::ObjectMessage* msg) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // Try to find stream info for the object
    ObjectHostProxStreamMap::iterator prox_stream_it = mObjectHostProxStreams.find(node);
    if (prox_stream_it == mObjectHostProxStreams.end()) {
        prox_stream_it = mObjectHostProxStreams.insert( ObjectHostProxStreamMap::value_type(node, ProxObjectHostStreamInfoPtr(new ProxObjectHostStreamInfo())) ).first;
        prox_stream_it->second->writecb = std::tr1::bind(
            &LibproxProximityBase::ProxObjectHostStreamInfo::writeSomeObjectResults, mContext, ProxObjectHostStreamInfo::WPtr(prox_stream_it->second)
        );
    }
    ProxObjectHostStreamInfoPtr prox_stream = prox_stream_it->second;

    // If we don't have a stream yet, try to build it
    if (!prox_stream->iostream_requested)
        ProxObjectHostStreamInfo::requestProxSubstream(this, mContext, node, prox_stream);

    // Build the result and push it into the stream
    // FIXME this is an infinite sized queue, but we don't really want to drop
    // proximity results....
    prox_stream->outstanding.push(Network::Frame::write(msg->payload()));

    // If writing isn't already in progress, start it up
    if (!prox_stream->writing)
        ProxObjectHostStreamInfo::writeSomeObjectResults(mContext, prox_stream);
}


bool LibproxProximityBase::validSession(const ObjectReference& oref) const {
    return (mContext->objectSessionManager()->getSession(oref) != NULL);
}

bool LibproxProximityBase::validSession(const OHDP::NodeID& node) const {
    return mContext->ohSessionManager()->getSession(node);
}

LibproxProximityBase::ProxObjectStreamPtr LibproxProximityBase::getBaseStream(const ObjectReference& oref) const {
    ObjectSession* session = mContext->objectSessionManager()->getSession(oref);
    return (session != NULL ? session->getStream() : ProxObjectStreamPtr());
}

LibproxProximityBase::ProxObjectHostStreamPtr LibproxProximityBase::getBaseStream(const OHDP::NodeID& node) const {
    ObjectHostSessionPtr session = mContext->ohSessionManager()->getSession(node);
    return (session ? session->stream() : ProxObjectHostStreamPtr());
}

void LibproxProximityBase::addObjectProxStreamInfo(ODPSST::Stream::Ptr strm) {
    UUID objid = strm->remoteEndPoint().endPoint.object().getAsUUID();
    assert(mObjectProxStreams.find(objid) == mObjectProxStreams.end());

    mObjectProxStreams.insert(
        ObjectProxStreamMap::value_type(
            objid,
            ProxObjectStreamInfoPtr(new ProxObjectStreamInfo(strm))
        )
    );
}

void LibproxProximityBase::addObjectHostProxStreamInfo(OHDPSST::Stream::Ptr strm) {
    OHDP::NodeID nodeid = strm->remoteEndPoint().endPoint.node();
    assert(mObjectHostProxStreams.find(nodeid) == mObjectHostProxStreams.end());

    mObjectHostProxStreams.insert(
        ObjectHostProxStreamMap::value_type(
            nodeid,
            ProxObjectHostStreamInfoPtr(new ProxObjectHostStreamInfo(strm))
        )
    );
}




void LibproxProximityBase::handleAddObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed);
}

void LibproxProximityBase::handleAddObjectLocSubscriptionWithID(const UUID& subscriber, const UUID& observed, ProxIndexID index_id) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed, index_id);
}

void LibproxProximityBase::handleRemoveObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveObjectLocSubscriptionWithID(const UUID& subscriber, const UUID& observed, ProxIndexID index_id) {
    mLocService->unsubscribe(subscriber, observed, index_id);
}

void LibproxProximityBase::handleRemoveAllObjectLocSubscription(const UUID& subscriber) {
    mLocService->unsubscribe(subscriber);
}

void LibproxProximityBase::handleAddOHLocSubscription(const OHDP::NodeID& subscriber, const UUID& observed) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed);
}

void LibproxProximityBase::handleAddOHLocSubscriptionWithID(const OHDP::NodeID& subscriber, const UUID& observed, ProxIndexID index_id) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed, index_id);
}

void LibproxProximityBase::handleRemoveOHLocSubscription(const OHDP::NodeID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveOHLocSubscriptionWithID(const OHDP::NodeID& subscriber, const UUID& observed, ProxIndexID index_id) {
    mLocService->unsubscribe(subscriber, observed, index_id);
}

void LibproxProximityBase::handleRemoveAllOHLocSubscription(const OHDP::NodeID& subscriber) {
    mLocService->unsubscribe(subscriber);
}

void LibproxProximityBase::handleAddServerLocSubscription(const ServerID& subscriber, const UUID& observed, SeqNoPtr seqPtr) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed, seqPtr);
}

void LibproxProximityBase::handleAddServerLocSubscriptionWithID(const ServerID& subscriber, const UUID& observed, ProxIndexID index_id, SeqNoPtr seqPtr) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed, index_id, seqPtr);
}

void LibproxProximityBase::handleRemoveServerLocSubscription(const ServerID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveServerLocSubscriptionWithID(const ServerID& subscriber, const UUID& observed, ProxIndexID index_id) {
    mLocService->unsubscribe(subscriber, observed, index_id);
}

void LibproxProximityBase::handleRemoveAllServerLocSubscription(const ServerID& subscriber) {
    mLocService->unsubscribe(subscriber);
}



void LibproxProximityBase::checkObjectClass(bool is_local, const UUID& objid, const TimedMotionVector3f& newval) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximityBase::handleCheckObjectClass, this, is_local, objid, newval),
        "LibproxProximityBase::handleCheckObjectClass"
    );
}


// PROX Thread

void LibproxProximityBase::handleConnectedServer(ServerID sid) {
    // Sometimes we may get forcefully disconnected from a server. To
    // reestablish our previous setup, if that server appears in our queried
    // set (we were told it was relevant to us by some higher level pinto
    // service), we mark it as needing another update. In the case that we
    // are just getting an initial connection, this shouldn't change anything
    // since it would already be markedas needing an update if we had been told
    // by pinto that it needed it.
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    if (mServersQueried.find(sid) != mServersQueried.end())
        mNeedServerQueryUpdate.insert(sid);
}

void LibproxProximityBase::handleDisconnectedServer(ServerID sid) {
    // When we lose a connection, we need to clear out everything related to
    // that server.
    PROXLOG(debug, "Handling unexpected disconnection from " << sid << " by clearing out proximity state.");
    handleForcedDisconnection(sid);
}

void LibproxProximityBase::handleForcedDisconnection(ServerID server) {
    // Nothing for base -- implementations should override to clean up
    // queries and clear out state
}


void LibproxProximityBase::aggregateCreated(const UUID& objid) {
    // On addition, an "aggregate" will have no children, i.e. its zero sized.

    mContext->mainStrand->post(
        std::tr1::bind(
            &LocationService::addLocalAggregateObject, mLocService,
            objid,
            TimedMotionVector3f(mContext->simTime(), MotionVector3f()),
            TimedMotionQuaternion(mContext->simTime(), MotionQuaternion()),
            AggregateBoundingInfo(),
            "",
            ""
        ),
        "LocationService::addLocalAggregateObject"
    );

    mAggregateManager->addAggregate(objid);
}

void LibproxProximityBase::aggregateChildAdded(const UUID& objid, const UUID& child, const Vector3f& pos, const AggregateBoundingInfo& bnds) {
    // FIXME the AggregateBoundingInfo is wrong here because we don't get all
    // the information we need about the aggregate.
    mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximityBase::updateAggregateLoc, this,
            objid, pos, bnds
        ),
        "LibproxProximityBase::updateAggregateLoc"
    );

    mAggregateManager->addChild(objid, child);
}

void LibproxProximityBase::aggregateChildRemoved(const UUID& objid, const UUID& child, const Vector3f& pos, const AggregateBoundingInfo& bnds) {
    // Loc cares only about this chance to update state of aggregate
    // FIXME the AggregateBoundingInfo is wrong here because we don't get all
    // the information we need about the aggregate.
    mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximityBase::updateAggregateLoc, this,
            objid, pos, bnds
        ),
        "LibproxProximityBase::updateAggregateLoc"
    );

    mAggregateManager->removeChild(objid, child);
}

void LibproxProximityBase::aggregateBoundsUpdated(const UUID& objid, const Vector3f& pos, const AggregateBoundingInfo& bnds) {
    mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximityBase::updateAggregateLoc, this,
            objid, pos, bnds
        ),
        "LibproxProximityBase::updateAggregateLoc"
    );

    mAggregateManager->generateAggregateMesh(objid, Duration::seconds(300.0+rand()%300));
}

void LibproxProximityBase::aggregateDestroyed(const UUID& objid) {
    mContext->mainStrand->post(
        std::tr1::bind(
            &LocationService::removeLocalAggregateObject, mLocService, objid
        ),
        "LocationService::removeLocalAggregateObject"
    );
    mAggregateManager->removeAggregate(objid);
}

void LibproxProximityBase::aggregateObserved(const UUID& objid, uint32 nobservers) {
    mAggregateManager->aggregateObserved(objid, nobservers);
}



void LibproxProximityBase::removeStaticObjectTimeout(const UUID& objid) {
    StaticObjectsByID& by_id = mStaticObjectTimeouts.get<objid_tag>();
    StaticObjectsByID::iterator it = by_id.find(objid);
    if (it == by_id.end()) return;
    by_id.erase(it);
}


void LibproxProximityBase::processExpiredStaticObjectTimeouts() {
    Time curt = mLocService->context()->recentSimTime();
    StaticObjectsByExpiration& by_expires = mStaticObjectTimeouts.get<expires_tag>();
    while(!by_expires.empty() &&
        by_expires.begin()->expires < curt) {
        trySwapHandlers(by_expires.begin()->local, by_expires.begin()->objid, true);
        by_expires.erase(by_expires.begin());
    }
}

void LibproxProximityBase::handleCheckObjectClass(bool is_local, const UUID& objid, const TimedMotionVector3f& newval) {
    assert(mSeparateDynamicObjects == true);

    // Basic approach: we need to check if the object has switched between
    // static/dynamic. We need to do this for both the local (object query) and
    // global (server query) handlers.
    bool is_static = velocityIsStatic(newval.velocity());
    // If it's moving, do the check immediately since we need to move it into
    // the dynamic tree right away; also make sure it's not in the queue for
    // being moved to the static tree. Otherwise queue it up to be processed
    // after a delay
    if (!is_static) {
        trySwapHandlers(is_local, objid, is_static);
        removeStaticObjectTimeout(objid);
    }
    else {
        // Make sure previous entry is cleared out
        removeStaticObjectTimeout(objid);
        // And insert a new one
        mStaticObjectTimeouts.insert(StaticObjectTimeout(objid, mContext->recentSimTime() + mMoveToStaticDelay, is_local));
    }
}



// MAIN strand
void LibproxProximityBase::updateAggregateLoc(const UUID& objid, const Vector3f& pos, const AggregateBoundingInfo& bnds) {
    if (mLocService->contains(objid) &&
        (mLocService->location(objid).position() != pos || mLocService->bounds(objid) != bnds))
    {
        mLocService->updateLocalAggregateLocation(
            objid,
            TimedMotionVector3f(mContext->simTime(), MotionVector3f(pos, Vector3f(0,0,0)))
        );
        mLocService->updateLocalAggregateBounds(
            objid,
            bnds
        );
    }
}

} // namespace Sirikata
