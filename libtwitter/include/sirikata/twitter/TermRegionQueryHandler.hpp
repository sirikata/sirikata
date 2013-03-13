// Copyright (c) 2013. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_TWITTER_PROX_TERM_REGION_QUERY_HANDLER_HPP_
#define _SIRIKATA_TWITTER_PROX_TERM_REGION_QUERY_HANDLER_HPP_

#include <prox/geom/QueryHandler.hpp>
#include <prox/base/LocationUpdateListener.hpp>
#include <prox/geom/QueryChangeListener.hpp>
#include <prox/base/QueryCache.hpp>

#include <prox/base/DefaultSimulationTraits.hpp>

#include <prox/rtree/RTree.hpp>
#include <prox/rtree/Cut.hpp>

#include <prox/rtree/impl/NodeIterator.hpp>

#include <json_spirit/json_spirit.h>

namespace Sirikata {



template<typename SimulationTraits, typename NodeDataType>
class TermRegionQueryHandler;

namespace TermRegionQueryHandlerImpl {
template<typename SimulationTraits, typename NodeDataType>
class NodeIteratorImpl :
        public Prox::RTreeHandlerImpl::NodeIteratorImpl<SimulationTraits, typename TermRegionQueryHandler<SimulationTraits, NodeDataType>::RTree>
{
public:
    typedef typename Prox::RTreeHandlerImpl::NodeIteratorImpl<SimulationTraits, typename TermRegionQueryHandler<SimulationTraits, NodeDataType>::RTree> Base;

    NodeIteratorImpl(typename Base::RTreeNodeIterator rit)
     : Base(rit)
    {}
    virtual ~NodeIteratorImpl() {}

    virtual NodeIteratorImpl* _clone() {
        return new NodeIteratorImpl(Base::it);
    }
};
} // namespace TermRegionQueryHandlerImpl


template<typename SimulationTraits>
class TermRegionQuery
    : public Prox::Query<SimulationTraits>
{
public:
    typedef Prox::Query<SimulationTraits> BaseQueryType; // Query class
    typedef typename BaseQueryType::QueryBaseType QueryBaseType; // QueryBase class
    typedef Prox::QueryHandler<SimulationTraits> QueryHandlerType;

    typedef typename SimulationTraits::Vector3Type Vector3;
    typedef typename SimulationTraits::MotionVector3Type MotionVector3;
    typedef typename SimulationTraits::BoundingSphereType BoundingSphere;
    typedef typename BaseQueryType::ID ID;

    // The only constructor -- you must specify a query term and region
    TermRegionQuery(QueryHandlerType* parent, ID id, const String& raw_query_, const String& term_, Vector3 rmin, Vector3 rmax, uint32 max_results_ = 100)
     // These are bogus parameters because we're not even going to use the
     // values here, only the data in the query string. This is just an artifact
     // of the hacked-on custom query strings
     : BaseQueryType(parent, id, MotionVector3(), BoundingSphere(), 0.0, SolidAngle::Max),
       raw_query(raw_query_),
       term(term_),
       region_min(rmin),
       region_max(rmax),
       max_results(max_results_)
    {
    }

    virtual ~TermRegionQuery() {}

    virtual void customQuery(const String& new_cqs) {
        assert(QueryBaseType::mValid);
        // We'll just defer to the query handler to do parsing. It should be
        // subscribed for notifications.
        String old_cqs = raw_query;
        raw_query = new_cqs;
        for(typename QueryBaseType::ChangeListenerListIterator it = QueryBaseType::mChangeListeners.begin(); it != QueryBaseType::mChangeListeners.end(); it++)
            (*it)->queryCustomQueryChanged(this, old_cqs, new_cqs);
    }

    // Cached data for tracking changes, not sure this is really necessary but
    // the query change interface wants it...
    String raw_query;
    // The real query data
    String term;
    Vector3 region_min;
    Vector3 region_max;
    uint32 max_results;
};


namespace {
// Return value indicates if we were able to properly parse, i.e. the JSON
// parsed *and* the format of the JSON was sane. To check whether you actually
// have values for all data (e.g. initial setting), check the return value and
// x_updated outputs.
bool parseQueryRequest(
    const String& query,
    bool* term_updated, String* term,
    bool* region_updated, Vector3f* region_min, Vector3f* region_max,
    bool* max_results_updated, uint32* max_results)
{
    if (query.empty())
        return false;

    namespace json = json_spirit;
    json::Value parsed;
    if (!json::read(query, parsed))
        return false;

    *term_updated = false;
    *region_updated = false;
    *max_results_updated = false;

    if (parsed.contains("term") && parsed.get("term").isString()) {
        *term = parsed.getString("term");
        *term_updated = true;
    }

    // We can have region, but not have all the components. In that case, we
    // want to trigger an error, so we want to return false, whereas if it's
    // missing, we just want to indicate it parsed but wasn't updated. So we
    // need to break this part down a bit more.
    if (parsed.contains("region")) {
        if (parsed.contains("region.min") &&
            parsed.get("region.min").isArray() &&
            (parsed.getArray("region.min").size() == 2 || parsed.getArray("region.min").size() == 3) &&
            (parsed.getArray("region.min")[0].isReal() && parsed.getArray("region.min")[1].isReal()) &&
            parsed.contains("region.max") &&
            parsed.get("region.max").isArray() &&
            (parsed.getArray("region.max").size() == 2 || parsed.getArray("region.max").size() == 3) &&
            (parsed.getArray("region.max")[0].isReal() && parsed.getArray("region.max")[1].isReal()) )
        {
            *region_min = Vector3f(parsed.getArray("region.min")[0].getReal(), parsed.getArray("region.min")[1].getReal(), 0);
            *region_max = Vector3f(parsed.getArray("region.max")[0].getReal(), parsed.getArray("region.max")[1].getReal(), 0);
            *region_updated = true;
        }
        else {
            // Invalid format because region existed but didn't have right components
            return false;
        }
    }

    if (parsed.contains("results") && parsed.get("results").isInt()) {
        *max_results = parsed.getInt("results");
        *max_results_updated = true;
    }

    return true;
}

// NOTE: bloom_empty let's us work around some bogus data in bogus nodes,
// i.e. fake top-level pinto trees from local-only implementations
bool checkTermRegion(
    // Query parameters
    const String& term,
    Vector3f region_min, Vector3f region_max,
    float32 cur_min_radius, // current minimum to match target # results
    // Object/aggregate props
    const Twitter::TermBloomFilter& bloom,
    BoundingSphere3f bnds)
{
    // NOTE: This must be <= because the radius is set to the *exact*
    // value to match the budget, but there may be a ton of objects
    // matching this size. To ensure we're under budget, we have to
    // fail anything at the same size as the minimum.
    if (bnds.radius() <= cur_min_radius) return false;

    // Convert to bounding box for easy, though more conservative, overlap check
    Vector3f bnds_min = bnds.center() - Vector3f(bnds.radius(), bnds.radius(), bnds.radius());
    Vector3f bnds_max = bnds.center() + Vector3f(bnds.radius(), bnds.radius(), bnds.radius());
    bool in_region =
        (region_min.x <= bnds_max.x && region_min.y <= bnds_max.y) &&
        (region_max.x >= bnds_min.x && region_max.y >= bnds_min.y);
    if (!in_region) return false;
    // Only evaluate bloom.lookup if in the region since it's much more
    // expensive to do all that hashing. FIXME Ideally we'd pre-hash the search
    // term... The empty term check let's us degrade to region query for
    // convenience, but generally shouldn't be used since it'll generate too
    // many results for large regions.
    return (term.empty() || bloom.lookup(term));
}
}


/** Implementation of QueryHandler that queries data using a textual query term
 * and a region.
 */
template<typename SimulationTraits, typename NodeDataType>
class TermRegionQueryHandler : public Prox::QueryHandler<SimulationTraits> {
public:
    typedef SimulationTraits SimulationTraitsType;

    typedef Prox::QueryHandler<SimulationTraits> QueryHandlerType;
    typedef Prox::LocationUpdateListener<SimulationTraits> LocationUpdateListenerType;
    typedef Prox::LocationUpdateProvider<SimulationTraits> LocationUpdateProviderType;
    typedef Prox::QueryChangeListener<SimulationTraits> QueryChangeListenerType;

    typedef Prox::AggregateListener<SimulationTraits> AggregateListenerType;

    // QueryType is the base Prox::Query class. We allocate our own so we can
    // store extra parsed info, so we really have TermQueryRegion which we
    // conver to sometimes.
    typedef Prox::Query<SimulationTraits> QueryType;
    typedef TermRegionQuery<SimulationTraits> DetailedQueryType;

    typedef Prox::QueryEvent<SimulationTraits> QueryEventType;
    typedef Prox::LocationServiceCache<SimulationTraits> LocationServiceCacheType;
    typedef typename LocationServiceCacheType::Iterator LocCacheIterator;

    typedef typename SimulationTraits::ObjectIDType ObjectID;
    typedef typename SimulationTraits::ObjectIDHasherType ObjectIDHasher;
    typedef typename SimulationTraits::TimeType Time;
    typedef typename SimulationTraits::realType Real;
    typedef typename SimulationTraits::Vector3Type Vector3;
    typedef typename SimulationTraits::MotionVector3Type MotionVector3;
    typedef typename SimulationTraits::BoundingSphereType BoundingSphere;
    typedef typename SimulationTraits::SolidAngleType SolidAngle;

    typedef typename QueryHandlerType::ShouldTrackCallback ShouldTrackCallback;
    typedef typename QueryHandlerType::ObjectList ObjectList;

    typedef typename std::tr1::function<TermRegionQueryHandler*()> QueryHandlerCreator;

    typedef typename QueryHandlerType::NodeIterator NodeIterator;

    static TermRegionQueryHandler* construct(uint16 elements_per_node) {
        return new TermRegionQueryHandler(elements_per_node);
    }
    static QueryHandlerCreator Constructor(uint16 elements_per_node) {
        return std::tr1::bind(&TermRegionQueryHandler::construct, elements_per_node);
    }

    TermRegionQueryHandler(uint16 elements_per_node)
     : QueryHandlerType(),
       mLocCache(NULL),
       mLocUpdateProvider(NULL),
       mRTree(NULL),
       mLastTime(Time::null()),
       mElementsPerNode(elements_per_node),
       mRebuilding(false)
    {
    }

    virtual ~TermRegionQueryHandler() {
        for(QueryMapIterator it = mQueries.begin(); it != mQueries.end(); it++) {
            QueryState* state = it->second;
            delete state;
        }
        mQueries.clear();

        destroyCurrentTree();

        mLocUpdateProvider->removeUpdateListener(this);
    }

    void initialize(LocationServiceCacheType* loc_cache, LocationUpdateProviderType* loc_up_provider, bool static_objects, bool replicated, ShouldTrackCallback should_track_cb = 0) {
        mLocCache = loc_cache;
        mLocUpdateProvider = loc_up_provider;
        mLocUpdateProvider->addUpdateListener(this);
        mShouldTrackCB = should_track_cb;

        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        using std::tr1::placeholders::_4;

        mRTree = new RTree(
            mElementsPerNode, mLocCache,
            static_objects, replicated,
            std::tr1::bind(&TermRegionQueryHandler::reportRestructures, this),
            std::tr1::bind(&TermRegionQueryHandler::handleRootCreated, this),
            this, aggregateListener(),
            std::tr1::bind(&CutNode<SimulationTraits>::handleRootReplaced, _1, _2, _3),
            0, 0, /* handleRootCreated/DestroyedAboveCut, not relevant for
                   * result-generating query handler */
            std::tr1::bind(&CutNode<SimulationTraits>::handleSplit, _1, _2, _3),
            0, /* handleSplitFinished, only relevant for manual
                * queries */
            0, /* handleSplitAboveCut, not relevant for result-generating query
                * handler */
            std::tr1::bind(&CutNode<SimulationTraits>::handleLiftCut, _1, _2),
            std::tr1::bind(&Cut::handleReorderCut, _1, _2),
            std::tr1::bind(&CutNode<SimulationTraits>::handleObjectInserted, _1, _2, _3),
            std::tr1::bind(&CutNode<SimulationTraits>::handleObjectRemoved, _1, _2, _3, _4),
            std::tr1::bind(&CutNode<SimulationTraits>::handleNodeAddedAboveCut, _1, _2),
            std::tr1::bind(&CutNode<SimulationTraits>::handleNodeRemoved, _1, _2),
            std::tr1::bind(&CutNode<SimulationTraits>::handleNodeReparented, _1, _2, _3, _4),
            0, /* handleNodeReparentedAboveCut, not relevant for
                  result-generating query handler */
            std::tr1::bind(&CutNode<SimulationTraits>::handleObjectReparented, _1, _2, _3, _4)
        );
    }

    virtual bool staticOnly() const {
        return mRTree->staticObjects();
    }

    void validateCuts() const {
#ifdef PROXDEBUG
        for(QueryMapConstIterator it = mQueries.begin(); it != mQueries.end(); it++) {
            QueryState* state = it->second;
            state->cut->validateCut();
        }
#endif
    }

    void tick(const Time& t, bool report) {
        mRTree->update(t);
        if (QueryHandlerType::mShouldRestructure)
            mRTree->restructure(t);

        mRTree->verifyConstraints(t);
        validateCuts();

        if (QueryHandlerType::mReportQueryStats && report)
            printf("tick\n");

        for(QueryMapIterator query_it = mQueries.begin(); query_it != mQueries.end(); query_it++) {
            DetailedQueryType* query = query_it->first;
            QueryState* state = query_it->second;

            int visited = state->cut->update(mLocCache, t);

            if (QueryHandlerType::mReportQueryStats && report) {
                printf("{ \"id\" : %d, \"checks\" : %d, \"cut-length\" : %d, \"results\" : %d }\n", query->id(), visited, state->cut->cutSize(), state->cut->resultsSize());
            }
        }
        mLastTime = t;

        mRTree->verifyConstraints(t);
        validateCuts();

        if (QueryHandlerType::mReportHealth && report) {
            QueryHandlerType::mItsSinceReportedHealth++;
            if (QueryHandlerType::mItsSinceReportedHealth >= QueryHandlerType::mReportHealthFrequency) {
                mRTree->reportBounds(t);
                QueryHandlerType::mItsSinceReportedHealth = 0;
            }
        }
        if (QueryHandlerType::mReportCost && report)
            printf("{ \"cost\" : %f }\n", cost());
    }

    virtual void rebuild() {
        validateCuts();

        // First, get all cuts out of the original tree
        for(typename QueryMap::iterator it = mQueries.begin(); it != mQueries.end(); it++)
            it->second->cut->startSwapTrees();

        // Then rebuild
        mRebuilding = true;

        ObjectList objects = allObjects();
        bool static_objects = mRTree->staticObjects();
        bool replicated = mRTree->replicated();

        // Destroy current tree
        destroyCurrentTree();

        // Build new tree
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        using std::tr1::placeholders::_4;
        mRTree = new RTree(
            mElementsPerNode, mLocCache,
            static_objects, replicated,
            std::tr1::bind(&TermRegionQueryHandler::reportRestructures, this),
            std::tr1::bind(&TermRegionQueryHandler::handleRootCreated, this),
            this, aggregateListener(),
            std::tr1::bind(&CutNode<SimulationTraits>::handleRootReplaced, _1, _2, _3),
            0, 0, /* handleRootReplacedAboveCut, not relevant for
                   * result-generating query handler */
            std::tr1::bind(&CutNode<SimulationTraits>::handleSplit, _1, _2, _3),
            0, /* handleSplitFinished, only relevant for manual
                * queries */
            0, /* handleSplitAboveCut, not relevant for result-generating query
                * handler */
            std::tr1::bind(&CutNode<SimulationTraits>::handleLiftCut, _1, _2),
            std::tr1::bind(&Cut::handleReorderCut, _1, _2),
            std::tr1::bind(&CutNode<SimulationTraits>::handleObjectInserted, _1, _2, _3),
            std::tr1::bind(&CutNode<SimulationTraits>::handleObjectRemoved, _1, _2, _3, _4),
            std::tr1::bind(&CutNode<SimulationTraits>::handleNodeAddedAboveCut, _1, _2),
            std::tr1::bind(&CutNode<SimulationTraits>::handleNodeRemoved, _1, _2),
            std::tr1::bind(&CutNode<SimulationTraits>::handleNodeReparented, _1, _2, _3, _4),
            0, /* handleNodeReparentedAboveCut, not relevant for
                  result-generating query handler */
            std::tr1::bind(&CutNode<SimulationTraits>::handleObjectReparented, _1, _2, _3, _4)
        );
        mRTree->bulkLoad(objects, mLastTime);

        mRebuilding = false;

        // Then reinsert into the new tree
        for(typename QueryMap::iterator it = mQueries.begin(); it != mQueries.end(); it++)
            it->second->cut->finishSwapTrees(mRTree->root());

        validateCuts();
    }

    virtual float cost() {
        return mRTree->cost(mLastTime);
    }

    virtual uint32 numObjects() const {
        return (uint32)mObjects.size();
    }
    virtual uint32 numQueries() const {
        return (uint32)mQueries.size();
    }
    virtual uint32 numNodes() const {
        // This is inefficient, but the RTree doesn't track number of nodes
        // internally.
        uint32 count = 0;
        NodeIterator it = QueryHandlerType::nodesBegin();
        NodeIterator end_it = QueryHandlerType::nodesEnd();
        while(it != end_it) {
            count++; it++;
        }
        return count;
    }

    virtual ObjectID rootAggregateID()  {
        return (mRTree->root() ? mRTree->root()->aggregateID() : typename SimulationTraits::ObjectIDNullType()());
    }

    virtual uint32 numResultsForQuery(const QueryType* q) const {
        QueryMapConstIterator it = mQueries.find(static_cast<DetailedQueryType*>(const_cast<QueryType*>(q)));
        if (it == mQueries.end()) return 0; // For rebuilding query handler
        QueryState* state = it->second;
        return state->cut->resultsSize();
    }
    virtual uint32 sizeForQuery(const QueryType* q) const {
        QueryMapConstIterator it = mQueries.find(static_cast<DetailedQueryType*>(const_cast<QueryType*>(q)));
        if (it == mQueries.end()) return 0; // For rebuilding query handler
        QueryState* state = it->second;
        return state->cut->cutSize();
    }

    virtual LocationServiceCacheType* locationCache() const {
        return mLocCache;
    }

    virtual void draw() { mRTree->draw(); }

    void addObject(const ObjectID& obj_id) {
        addObject(mLocCache->startTracking(obj_id));
    }
    void addObject(const LocCacheIterator& obj_loc_it) {
        ObjectID obj_id = mLocCache->iteratorID(obj_loc_it);
        assert(mObjects.find(obj_id) == mObjects.end());

        mObjects[obj_id] = obj_loc_it;
        mRTree->insert(mObjects[obj_id], mLastTime);

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }
    void addObject(const ObjectID& obj_id, const ObjectID& parent) {
        addObject(mLocCache->startTracking(obj_id), parent);
    }
    void addObject(const LocCacheIterator& obj_loc_it, const ObjectID& parent) {
        ObjectID obj_id = mLocCache->iteratorID(obj_loc_it);
        assert(mObjects.find(obj_id) == mObjects.end());

        mObjects[obj_id] = obj_loc_it;
        mRTree->insert(mObjects[obj_id], parent, mLastTime);

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }

    void removeObject(const ObjectID& obj_id, bool temporary = false) {
        typename ObjectSet::iterator it = mObjects.find(obj_id);
        if (it == mObjects.end()) return;

        mRTree->verifyConstraints(mLastTime);
        validateCuts();

        LocCacheIterator obj_loc_it = it->second;
        deleteObj(obj_id, mLastTime, temporary);
        mLocCache->stopTracking(obj_loc_it);
        mObjects.erase(it);

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }

    bool containsObject(const ObjectID& obj_id) {
        return (mObjects.find(obj_id) != mObjects.end());
    }

    ObjectList allObjects() {
        ObjectList retval;
        retval.reserve(mObjects.size());
        for(typename ObjectSet::iterator it = mObjects.begin(); it != mObjects.end(); it++)
            retval.push_back(mLocCache->startTracking(it->first));
        return retval;
    }


    void addNode(const ObjectID& nodeid) {
        // Nodes (aggregates) that don't have parents specified are just treated
        // like objects. This should only work if we are getting aggregate loc
        // info and we're not trying to reconstruct the tree exactly
        // (i.e. aggregates and objects are treated as a soup of normal objects
        // with which we can do whatever we want).
        addObject(nodeid);
    }
    void addNode(const ObjectID& nodeid, const ObjectID& parent) {
        addNode(mLocCache->startTracking(nodeid), parent);
    }
    void addNode(const LocCacheIterator& node_loc_it, const ObjectID& parent) {
        ObjectID node_id = mLocCache->iteratorID(node_loc_it);
        assert(mNodes.find(node_id) == mNodes.end());

        mNodes[node_id] = node_loc_it;
        mRTree->insertNode(node_loc_it, parent, mLastTime);
    }
    void removeNode(const ObjectID& nodeid, bool temporary = false) {
        typename ObjectSet::iterator it = mNodes.find(nodeid);
        if (it == mNodes.end()) return;

        LocCacheIterator node_loc_it = it->second;
        mRTree->eraseNode(node_loc_it, mLastTime, temporary);
        mLocCache->stopTracking(node_loc_it);
        mNodes.erase(it);
    }

    void reparent(const ObjectID& nodeobjid, const ObjectID& parentid) {
        typename ObjectSet::iterator obj_it = mObjects.find(nodeobjid);
        if (obj_it != mObjects.end()) {
            mRTree->reparentObject(obj_it->second, parentid, mLastTime);
            return;
        }

        typename ObjectSet::iterator node_it = mNodes.find(nodeobjid);
        if (node_it != mNodes.end()) {
            mRTree->reparentNode(node_it->second, parentid, mLastTime);
            return;
        }
    }


    void locationConnected(const ObjectID& obj_id, bool aggregate, bool local, const MotionVector3& pos, const BoundingSphere& region, Real ms) {
        assert(mObjects.find(obj_id) == mObjects.end());

        bool do_track = true;
        if (mShouldTrackCB) do_track = mShouldTrackCB(obj_id, local, aggregate, pos, region, ms);

        if (do_track) {
            if (!aggregate)
                addObject(obj_id);
            else
                addNode(obj_id);
        }
    }

    void locationConnectedWithParent(const ObjectID& obj_id, const ObjectID& parent, bool aggregate, bool local, const MotionVector3& pos, const BoundingSphere& region, Real ms) {
        assert(mObjects.find(obj_id) == mObjects.end());

        bool do_track = true;
        if (mShouldTrackCB) do_track = mShouldTrackCB(obj_id, local, aggregate, pos, region, ms);

        if (do_track) {
            if (!aggregate)
                addObject(obj_id, parent);
            else
                addNode(obj_id, parent);
        }
    }

    // LocationUpdateListener Implementation
    void locationParentUpdated(const ObjectID& obj_id, const ObjectID& old_par, const ObjectID& new_par) {
        reparent(obj_id, new_par);
    }

    void locationPositionUpdated(const ObjectID& obj_id, const MotionVector3& old_pos, const MotionVector3& new_pos) {
        updateObj(obj_id, mLastTime); // FIXME new time?

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }

    void locationRegionUpdated(const ObjectID& obj_id, const BoundingSphere& old_region, const BoundingSphere& new_region) {
        updateObj(obj_id, mLastTime); // FIXME new time?

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }

    void locationMaxSizeUpdated(const ObjectID& obj_id, Real old_maxSize, Real new_maxSize) {
        updateObj(obj_id, mLastTime); // FIXME new time?

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }

    void locationQueryDataUpdated(const ObjectID& obj_id, const String& old_query_data, const String& new_query_data) {
        updateObj(obj_id, mLastTime); // FIXME new time?

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }

    void locationDisconnected(const ObjectID& obj_id, bool temporary = false) {
        removeObject(obj_id, temporary);
        removeNode(obj_id, temporary);
    }

    // QueryChangeListener Implementation
    void queryPositionChanged(QueryType* query, const MotionVector3& old_pos, const MotionVector3& new_pos) {
        // Nothing to be done, we use values directly from the query
    }

    void queryRegionChanged(QueryType* query, const BoundingSphere& old_region, const BoundingSphere& new_region) {
        // XXX FIXME
    }

    void queryMaxSizeChanged(QueryType* query, Real old_ms, Real new_ms) {
        // XXX FIXME
    }

    void queryAngleChanged(QueryType* query, const SolidAngle& old_val, const SolidAngle& new_val) {
        // XXX FIXME
    }

    void queryMaxResultsChanged(QueryType* query, const uint32 old_val, const uint32 new_val) {
        // XXX FIXME
    }

    void queryCustomQueryChanged(QueryType* query, const String& old_val, const String& new_val) {
        DetailedQueryType* detailed_query = static_cast<DetailedQueryType*>(query);
        // Try parsing the updated
        String term;
        Vector3f region_min, region_max;
        uint32 max_results;
        bool term_updated, region_updated, max_results_updated;
        bool parsed_success = parseQueryRequest(new_val, &term_updated, &term, &region_updated, &region_min, &region_max, &max_results_updated, &max_results);
        if (!parsed_success) {
            SILOG(term-region-query, error, "Got invalid query update.");
            return;
        }

        if (term_updated)
            detailed_query->term = term;
        if (region_updated) {
            detailed_query->region_min = region_min;
            detailed_query->region_max = region_max;
        }
        if (max_results_updated)
            detailed_query->max_results = max_results;

        // Allow relaxation to start happening immediately in case
        // this needs refinement
        QueryMapIterator query_it = mQueries.find(detailed_query);
        assert(query_it != mQueries.end());
        QueryState* state = query_it->second;
        state->cut->mMinResultRadiusRelaxDelayIts = 0;
    }


    void queryDestroyed(QueryType* query, bool implicit) {
        QueryMapIterator it = mQueries.find(static_cast<DetailedQueryType*>(const_cast<QueryType*>(query)));
        assert( it != mQueries.end() );
        QueryState* state = it->second;

        // Fill in removal events if they aren't implicit
        if (!implicit) {
            QueryEventType rem_evt(mLocCache, QueryHandlerType::handlerID());
            state->cut->destroyCut(rem_evt);
            query->pushEvent(rem_evt);
        }

        delete state;
        mQueries.erase(it);

        mRTree->verifyConstraints(mLastTime);
        validateCuts();
    }

    void queryDeleted(const QueryType* query) {
    }

protected:

    using QueryHandlerType::mQueryIDSource;

    virtual bool customQueryType() const { return true;}
    virtual QueryType* registerCustomQuery(const String& query_arg) {
        String term;
        Vector3f region_min, region_max;
        uint32 max_results;
        bool term_updated, region_updated, max_results_updated;
        bool parsed_success = parseQueryRequest(query_arg, &term_updated, &term, &region_updated, &region_min, &region_max, &max_results_updated, &max_results);
        // Check all but max_results for validity. Max results has a
        // default value
        if (!parsed_success || !term_updated || !region_updated) {
            SILOG(term-region-query, error, "Got invalid initial query.");
            return NULL;
        }

        // FIXME test data - full US
        //DetailedQueryType* q = new DetailedQueryType(this, mQueryIDSource++, "lol", Vector3(-129.02, 24.9, 0), Vector3(-58.71, 50.6, 0));
        // FIXME test data - quarter of US
        //DetailedQueryType* query = new DetailedQueryType(this, mQueryIDSource++, "lol", Vector3(-129.02, 24.9, 0), Vector3(-93.5, 37.5, 0));
        DetailedQueryType* query = new DetailedQueryType(this, mQueryIDSource++, query_arg, term, region_min, region_max, max_results_updated ? max_results : 100);
        QueryState* state = new QueryState(this, query, mRTree->root());
        mQueries[query] = state;
        query->addChangeListener(this);

        mRTree->verifyConstraints(mLastTime);
        validateCuts();

        return query;
    }
    void registerQuery(QueryType* query) {
        // Should not hit this version -- it's only used for registering basic
        // geometric queries and we need a custom query.
        assert(false);
    }

private:
    void destroyCurrentTree() {
        delete mRTree;

        for(ObjectSetIterator it = mObjects.begin(); it != mObjects.end(); it++) {
            mLocCache->stopTracking(it->second);
        }
        mObjects.clear();
    }

    void updateObj(const ObjectID& obj_id, const Time& t) {
        typename ObjectSet::iterator it = mObjects.find(obj_id);
        if (it != mObjects.end()) {
            mRTree->update(mObjects[obj_id], t);
        }

        typename ObjectSet::iterator nit = mNodes.find(obj_id);
        if (nit != mNodes.end()) {
            mRTree->updateNode(mNodes[obj_id], t);
        }
    }

    void deleteObj(const ObjectID& obj_id, const Time& t, bool temporary) {
        assert(mObjects.find(obj_id) != mObjects.end());
        mRTree->erase(mObjects[obj_id], t, temporary);
    }



    void handleRootCreated() {
        assert(mRTree->root() != NULL);
        // Initialize any cuts that haven't been initialized yet (never got
        // past, or dropped to, cut length 0)
        for(QueryMapIterator it = mQueries.begin(); it != mQueries.end(); it++)
            if (it->second->cut->cutSize() == 0) it->second->cut->init(mRTree->root());
    }


    ///this needs to be a template class for no good reason: Microsoft visual studio bugs demand it.
    template <class XSimulationTraits> struct CutNode;
    struct Cut;

public: // Public for the sake of implementation -- node iterators are separate classes
    typedef NodeDataType NodeData;
    typedef Prox::RTree<SimulationTraits, NodeData, CutNode<SimulationTraits> > RTree;
private:
    typedef typename RTree::RTreeNodeType RTreeNodeType;

    ///this needs to be a template class for no good reason: Microsoft visual studio bugs demand it.
    template <class XSimulationTraits> struct CutNode :
        public Prox::CutNodeBase<SimulationTraits, QueryHandlerType, NodeData, Cut, CutNode<SimulationTraits> >
    {
        typedef Prox::CutNodeBase<SimulationTraits, QueryHandlerType, NodeData, Cut, CutNode<SimulationTraits> > CutNodeBaseType;
        typedef typename CutNodeBaseType::CutType CutType;
        typedef typename CutNodeBaseType::RangeType RangeType;

        CutNode(QueryHandlerType* handler, Cut* _parent, RTreeNodeType* _rt, AggregateListenerType* listener)
         : CutNodeBaseType(handler, _parent, _rt, listener)
        {
        }

    private:
        friend class Prox::CutNodeBase<SimulationTraits, QueryHandlerType, NodeData, Cut, CutNode<SimulationTraits> >;
        ~CutNode() {
        }

    public:
        bool updateSatisfies(const String& term, Vector3 region_min, Vector3 region_max) {
            satisfies = checkTermRegion(
                term, region_min, region_max, getParent()->mMinResultRadius,
                rtnode->data().getBloomFilter(), rtnode->data().getBounds()
            );
            return satisfies;
        }

        using CutNodeBaseType::rtnode;
        using CutNodeBaseType::satisfies;
        using CutNodeBaseType::getParent;
    };

    friend class Prox::CutBase<SimulationTraits, TermRegionQueryHandler, NodeData, Cut, CutNode<SimulationTraits> >;
    class Cut
        : public Prox::CutBase<SimulationTraits, TermRegionQueryHandler, NodeData, Cut, CutNode<SimulationTraits> >
    {
    private:
        Cut();

        typedef Prox::CutBase<SimulationTraits, TermRegionQueryHandler, NodeData, Cut, CutNode<SimulationTraits> > CutBaseType;
        typedef typename CutBaseType::CutNodeType CutNodeType;
        typedef typename CutBaseType::CutNodeList CutNodeList;
        typedef typename CutBaseType::CutNodeListIterator CutNodeListIterator;
        typedef typename CutBaseType::CutNodeListConstIterator CutNodeListConstIterator;
        typedef typename CutBaseType::ChangeReason ChangeReason;

        using CutBaseType::parent;
        using CutBaseType::query;
        DetailedQueryType* detailedQuery() const {
            return static_cast<DetailedQueryType*>(query);
        }
        using CutBaseType::nodes;
        using CutBaseType::length;
        using CutBaseType::events;

        using CutBaseType::getLocCache;
    public:
        using CutBaseType::validateCut;
    private:
        typedef typename CutBaseType::ResultSet ResultSet;
        ResultSet results;


    public:
        // Keep one extra parameter used by queries which is
        // cut-specific and not a real part of the query: a minimum
        // size of result which we can use to force the cut up or down
        // the tree to try to match a specific number of results.
        float32 mMinResultRadius;
        int32 mMinResultRadiusRelaxDelayIts;
#define MIN_RADIUS_RELAX_FACTOR 0.999
#define MIN_RADIUS_RELAX_DELAY 250

        /** Regular constructor.  A new cut simply starts with the root node and
         *  immediately refines.
         */
        Cut(TermRegionQueryHandler* _parent, QueryType* _query, RTreeNodeType* root)
         : CutBaseType(_parent, _query),
           mMinResultRadius(0.f),
           mMinResultRadiusRelaxDelayIts(0)
        {
            init(root);
            // Make sure we get events from initialization to the client
            if (!events.empty()) query->pushEvents(events);
        }

        ~Cut() {
        }

        // Methods required by CutBase
        bool withAggregates() const {
            return true;
        }
        AggregateListenerType* aggregateListener() {
            return parent->aggregateListener();
        }
        LocationServiceCacheType* locCache() const {
            return parent->mLocCache;
        }
        const Time& curTime() const {
            return parent->mLastTime;
        }
        RTreeNodeType* rootRTreeNode() {
            return parent->mRTree->root();
        }
        bool rebuilding() const {
            return parent->mRebuilding;
        }
        void addResult(CutNodeType* cnode) {
            assert(!inResultsSlow(cnode->rtnode->aggregateID()));
            assert(!cnode->active_result);

            cnode->active_result = true;
            results.insert(cnode->rtnode->aggregateID());
        }
        void addResult(CutNodeType* cnode, const ObjectID& objid) {
            // Verify with assert, but just trust that they gave us a
            // real object child
            assert((cnode->rtnode->aggregateID() != objid));
            assert(cnode->rtnode->objectChildren());
            assert(!inResultsSlow(objid));
            // Would be nice but we need to guarantee some ordering of
            //remove/add, and depends on aggs vs. no aggs
            //assert(!cnode->active_result);
            results.insert(objid);
        }
        size_t tryRemoveResult(CutNodeType* cnode) {
            cnode->active_result = false;
            return results.erase(cnode->rtnode->aggregateID());
        }
        size_t tryRemoveResult(CutNodeType* cnode, const ObjectID& objid) {
            // Verify with assert, but just trust that they gave us a
            // real object child
            assert((cnode->rtnode->aggregateID() != objid));
            assert(cnode->rtnode->objectChildren());
            // Would be nice but we need to guarantee some ordering of
            //remove/add, and depends on aggs vs. no aggs
            //assert(cnode->active_result);
            return results.erase(objid);
        }
        void removeResult(CutNodeType* cnode) {
            assert(inResultsSlow(cnode->rtnode->aggregateID()));
            assert(cnode->active_result);
            size_t removed = tryRemoveResult(cnode);
            assert(removed);
        }
        void removeResult(CutNodeType* cnode, const ObjectID& objid) {
            assert(inResultsSlow(objid));
            size_t removed = tryRemoveResult(cnode, objid);
            assert(removed);
        }
        bool inResults(CutNodeType* cnode) const {
            assert( (results.find(cnode->rtnode->aggregateID()) != results.end()) == cnode->active_result);
            return cnode->active_result;
        }
        bool inResults(CutNodeType* cnode, const ObjectID& objid) const {
            // Verify with assert, but just trust that they gave us a
            // real object child
            assert((cnode->rtnode->aggregateID() != objid));
            assert(cnode->rtnode->objectChildren());
            // Can't necessarily make this fast since we have nowhere
            // to store the bit, but we can shortcut if we know the
            // parent is a result
            if (cnode->active_result) {
                assert((results.find(objid) == results.end()));
                return false;
            }
            else if (withAggregates()) {
                // However, with aggregates this can be fast again
                // because we should have the child in the results
                assert((results.find(objid) != results.end()));
                return true;
            }
            return inResultsSlow(objid);
        }
        bool inResultsSlow(const ObjectID& objid) const {
            return results.find(objid) != results.end();
        }
        int resultsSize() const {
            return results.size();
        }
        bool satisfiesQuery(RTreeNodeType* node, LocCacheIterator objit, int objidx) const {
            Time t = curTime();
            DetailedQueryType* dquery = detailedQuery();
            NodeData nd = node->childData(objidx, t);
            return checkTermRegion(
                dquery->term, dquery->region_min, dquery->region_max, mMinResultRadius,
                nd.getBloomFilter(), nd.getBounds()
            );
        }
        bool includeAddition(ChangeReason act) const {
            // In this query handler we want regular cut maintainence. All
            // removal events should be forwarded.
            return true;
        }
        bool includeRemoval(ChangeReason act) const {
            // In this query handler we want regular cut maintainence. All
            // removal events should be forwarded.
            return true;
        }
        bool includeReparent() const {
            // Just need cut, adjustments to parents would just be ignored.
            return false;
        }
        bool includeIntermediateEvents() const {
            // As long as we maintain a cut, the client doesn't care -- it just
            // needs the result set.
            return false;
        }
    private:
        // Struct which keeps track of how many children do not satisfy the
        // constraint so they can be collapsed.
        struct ParentCollapseInfo {
            RTreeNodeType* parent;
            int count;
        };

    public:
        // Checks for child_id's membership in the result set.  This version
        // should be used for non-aggregate queries.
        void checkMembership(const ObjectID& child_id, bool child_satisfies) {
            updateMembership(child_id, child_satisfies);
        }

        // Returns the number of "nodes" visited, including objects.
        // In other words, gives the number of solid angle tests performed.
        int update(LocationServiceCacheType* loc, const Time& t) {
            // Update assumes that the cut is already valid, i.e. that any
            // adjustments to the tree have already caused fixes in the cut
            // itself. The only thing that should have changed at this point is
            // the query itself. Therefore, we can simply expand the cut down
            // and pull it up as appropriate.
            //
            // We traverse the cut linearly and handle a few cases based on the
            // rule that we store cut nodes at RTree nodes if they are leaf
            // nodes (that satisfy or do not satisfy the solid angle constraint)
            // or at internal nodes which do not satisfy the solid angle
            // constraint.
            //
            // 1. If we encounter an internal node that satisfies the
            // constraint, we expand that node to contain all child nodes.  We
            // then continue iteration at the first of those new nodes.
            //
            // 2. If we encounter a leaf node, we test the true leaf children
            // iff the leaf node satisfies the constraints.  When testing true
            // leaves we just ensure they are appropriately in or out of the
            // result set.
            //
            // 3. (Continued from updateDown) If all the children in a parent
            // node are not satisfying the constraint, check if the parent
            // does. If the parent also does not, push the cut up to the parent
            // node.  Repeat until no more "merging" or pushing the cut up can
            // occur.
            // NOTE: 3 is not implemented. Doing this would make everything more
            // efficient but shouldn't affect correctness.

            // FIXME we should handle a few different cases differently here.
            // If the query is new, has been updated, or is moving, we should
            // evaluate across the entire tree.  If none of these things have
            // happened, previous updating should have taken care of everything
            // except pushing the cut back up the tree for now-unsatisfied
            // nodes.

            int visited = 0;

            // Keeps track of candidates for collapse and the number of children
            // they have that don't satisfy the constraint. Form a stack since
            // they may get deeper in the tree.
            std::stack<ParentCollapseInfo> collapseStack;

            DetailedQueryType* dquery = detailedQuery();
            const String& query_term = dquery->term;
            Vector3 query_region_min = dquery->region_min;
            Vector3 query_region_max = dquery->region_max;
            int32 query_max_results = dquery->max_results;

            bool updated_min_size = false;
            bool updated_min_size_in_loop = true;

            uint32 initial_results_size = results.size();
            for(CutNodeListIterator it = nodes.begin(); it != nodes.end(); ) {
                CutNode<SimulationTraits>* node = *it;
                bool last_satisfies = node->satisfies;
                bool satisfies = node->updateSatisfies(query_term, query_region_min, query_region_max);
                visited++;

                // If we went from satisfies -> not satisfies, we may need to
                // clean up some results
                if (last_satisfies && !satisfies) {
                    // If we're tracking aggregates and we went from satisfies ->
                    // not satisfies, then we need to clear out any children
                    typename ResultSet::iterator this_result_it = results.find(node->rtnode->aggregateID());
                    if (this_result_it != results.end()) {
                        // Either this node was in the result set last time...
                        // And there's nothing to do -- it will be removed when
                        // the cut is pushed up the tree
                    }
                    else {
                        // Or its children were, in which case we remove them and
                        // add this node (even though its not a real result).
                        // If this change allows merging to the parent node of
                        // this node, that'll happen upon push-up
                        QueryEventType evt(getLocCache(), parent->handlerID());
                        // No need to check like Cut code does for whether
                        // to includeAddition() because we know it always
                        // returns true
                        evt.addAddition( typename QueryEventType::Addition(node->rtnode, QueryEventType::Imposter) );
                        results.insert( node->rtnode->aggregateID() );
                        for(int i = 0; i < node->rtnode->size(); i++) {
                            ObjectID child_id = loc->iteratorID(node->rtnode->object(i).object);
                            typename ResultSet::iterator result_it = results.find(child_id);
                            assert(result_it != results.end());
                            results.erase(result_it);
                            // No need to check like Cut code does for
                            // whether to includeRemoval() because we know
                            // it always returns true
                            evt.addRemoval( typename QueryEventType::Removal(child_id, QueryEventType::Transient) );
                        }
                        events.push_back(evt);
                    }
                }

                // Aside from clean up above, if we don't satisfy the
                // constraints there's no reason to continue with this node.
                if (!satisfies) {
                    // This section deals with tracking sequences of nodes that
                    // do not satisfy the constraint.  The basic approach is to
                    // maintain a stack of valid ancestors that have been
                    // encountered on the cut, keep a count of children that do
                    // not satisfy them, and collapse as appropriate.  The stack
                    // is necessary because a cut might start high in the tree
                    // and then get deeper; the deeper part could collapse,
                    // leaving the earlier, higher part also collapsable. We
                    // need the stack to keep track of these other candidates.
                    // The stack gets cleaned out as we discover that the parent
                    // node currently being considered is not an ancestor of the
                    // current cut node anymore.

                    RTreeNodeType* cur_parent = (collapseStack.empty() ? NULL : collapseStack.top().parent);

                    // First make sure we have the right parent.
                    RTreeNodeType* this_node = node->rtnode;
                    RTreeNodeType* this_parent = node->rtnode->parent();
                    // If we have a mismatch:
                    if (this_parent != cur_parent) {
                        // First we check if we need to pop nodes off since we
                        // may have left a subtree.
                        while(!collapseStack.empty()) {
                            // FIXME this could be more efficient by tracing up
                            // the parent tree and the stack at the same time.
                            if (!_is_ancestor(this_node, collapseStack.top().parent))
                                collapseStack.pop();
                            else
                                break;
                        }
                        // Update cur_parent to reflect new state
                        cur_parent = (collapseStack.empty() ? NULL : collapseStack.top().parent);

                        // Then we recheck if we still have a mismatch --
                        // popping nodes may have found us the parent we were
                        // looking for.  If we do have a mismatch, we need to
                        // pop a new node on, but only if it is worth it (this
                        // node is the first child node of the parent, otherwise
                        // we should have found the parent or the first node
                        // satisfied the constraint and this parent isn't really
                        // an option).
                        if (this_parent != cur_parent) {
                            if (this_parent != NULL &&
                                this_parent->node(0) == node->rtnode)
                            {
                                ParentCollapseInfo new_collapse_info;
                                new_collapse_info.parent = this_parent;
                                new_collapse_info.count = 0;
                                collapseStack.push(new_collapse_info);
                                // Update cur_parent
                                cur_parent = this_parent;
                            }
                        }
                    }

                    // Now, if there's a match, increment. Finally, check if the
                    // top node can be cleaned out.
                    if (this_parent == cur_parent &&
                        this_parent != NULL)
                    {
                        collapseStack.top().count++;
                        if (collapseStack.top().count == collapseStack.top().parent->size()) {
                            // And this collapse is only useful if the parent is
                            // now *also* not satisfying the constraint.  This
                            // must also be true, otherwise we will constantly
                            // ping-pong back and forth between collapsing the
                            // node and expanding it back again.
                            bool parent_satisfies =
                                checkTermRegion(
                                    query_term, query_region_min, query_region_max, mMinResultRadius,
                                    this_parent->data().getBloomFilter(), this_parent->data().getBounds()
                                );
                            visited++;
                            if (!parent_satisfies) {
                                // Note that the iterator returned is the parent
                                // cut node, so advancing the iterator at the
                                // end of this block is safe.
                                // We need an event for both with and without
                                // aggregates because the removal of a leaf node
                                // could affect the result set in either case.
                                QueryEventType evt(getLocCache(), parent->handlerID());
                                it = replaceChildrenWithParent(it, &evt);
                                if (evt.size() > 0)
                                    events.push_back(evt);
                                // Since we've replaced the parent, we need to
                                // pop it off the candidate stack
                                // FIXME should check if it needs to push *it's*
                                // parent on the stack.
                                collapseStack.pop();
                            }
                        }
                    }

                    // And of course, we need to advance the cut node iterator
                    // and continue.
                    it++;
                    continue;
                }

                // What we do with satisfying nodes depends on whether they are
                // a leaf or not
                if (!node->objectChildren()) {
                    // For internal nodes that satisfy, we replace this node
                    // with the children. Note: don't increment since we need to
                    // start with the first child, which will now be
                    // it
                    // However, we only do this if there *are*
                    // children to replace it with
                    if (!node->rtnode->empty()) {
                        QueryEventType evt(getLocCache(), parent->handlerID());
                        it = replaceParentWithChildren(it, &evt);
                        events.push_back(evt);
                    }
                    else {
                        // If we couldn't refine, we just have to keep
                        // moving forward
                        it++;
                    }
                }
                else {
                    // For leaf nodes, there are two possibilities depending on
                    // whether we're tracking aggregates in the result set or
                    // not.
                    // If we are tracking aggregates, we need to manage
                    // membership of the parent node.  last_satisfies and
                    // satisfies indicate whether a change occurred.

                    typename ResultSet::iterator result_it = results.find(node->rtnode->aggregateID());
                    bool in_results = (result_it != results.end());

                    // In either case, we're going to need to
                    // check if any of the children satisfy the
                    // constraint.
                    bool any_child_satisfied = false;
                    for(int i = 0; i < node->rtnode->size(); i++) {
                        NodeData nd = node->rtnode->childData(i, t);
                        bool child_satisfies =
                            checkTermRegion(
                                query_term, query_region_min, query_region_max, mMinResultRadius,
                                nd.getBloomFilter(), nd.getBounds()
                            );
                        visited++;
                        if (child_satisfies) {
                            any_child_satisfied = true;
                            break;
                        }
                    }

                    if (!in_results && !any_child_satisfied) {
                        // If this node wasn't already in the
                        // results, then all the children should
                        // be. If no children satisfy anymore,
                        // lift the cut back up.
                        QueryEventType evt(getLocCache(), parent->handlerID());
                        replaceLeafChildrenWithParent(node, &evt);
                        if (evt.size() > 0)
                            events.push_back(evt);

                    }
                    else if (in_results && any_child_satisfied) {
                        // If it is in the result set and some
                        // child did satisfy the constraint, we
                        // need to displace it with its children.
                        // With aggregates there's no need to
                        // check the objects for satisfying the
                        // query.
                        replaceParentWithChildrenResults(node);
                    }

                    it++;
                }

                // Stop gap to limit how big the cut gets since we
                // need to finish processing the cut without changing
                // the query. In this case, we change the query, but
                // make sure it's consistent with all the results
                // we've computed so far -- by picking the minimum
                // object in the results (less a little to make sure
                // it continues to satisfy the query), we ensure
                // everything higher in the tree (larger) continues to
                // satisfy the query. We check a couple of conditions
                // because this approach isn't perfect and can result
                // in counterproductive updates if we always update
                // when we see too many results
                if ((int)results.size() >= query_max_results && // too big
                    (results.size() > initial_results_size) &&  // and growing
                    !updated_min_size_in_loop) // and we haven't set the min yet (if we
                             // redo, can only get worse)
                {
                    float32 min_rad = FLT_MAX;
                    for(CutNodeListIterator cit = nodes.begin(); cit != nodes.end(); cit++) {
                        CutNode<SimulationTraits>* node = *cit;
                        // We need to get results, not just elements
                        // on the cut.
                        if (inResults(node, node->rtnode->aggregateID())) {
                            min_rad = std::min(min_rad, node->rtnode->data().getBounds().radius());
                        }
                        else {
                            // Not in results, so children satisfy
                            for(int i = 0; i < node->rtnode->size(); i++)
                                min_rad = std::min(min_rad, node->rtnode->childData(i, t).getBounds().radius());
                        }
                        // Slightly smaller so smallest still
                        // satisfies query
                        mMinResultRadius = min_rad * MIN_RADIUS_RELAX_FACTOR;
                        updated_min_size_in_loop = true;
                    }
                }
            }

            // If we reached our budget, immediately set a maximum
            // size based on what we see in the cut so that we
            // don't end up refining too much stuff. This will
            // immediately affect whether nodes are marked as
            // satisfying. We only do this if a) we haven't done
            // it yet this round or b) we've increased in size yet
            // still which means we're in danger of continuing to
            // increase in size so updating again and setting a
            // stricter budget is worth it.
            //
            // This is only done outside the loop currently because
            // updating it effectively changes the query and that can
            // cause the traversal to do bad things because whether a
            // node satisfies the query will change while processing
            // the cut.
            //
            // FIXME obviously this isn't really the best way to
            // accomplish this -- it doesn't let us trade worse
            // objects for better ones as the tree changes (we
            // don't perform matching refine/coarsen if we're
            // already at the budget), but this is much easier to
            // fit into the current cut update scheme. It also
            // responsive (acting upon too many results) rather
            // than proactive (not letting too many results in the
            // first place)....
            if ((int)results.size() >= query_max_results && // too big
                (results.size() > initial_results_size)) // and growing
            {
                std::vector<float32> radii;
                for(CutNodeListIterator cit = nodes.begin(); cit != nodes.end(); cit++) {
                    CutNode<SimulationTraits>* node = *cit;
                    // We need to get results, not just elements
                    // on the cut. Also, we don't want to get the
                    // radii of the objects on the cut, we want to
                    // get the radii of their parent since the
                    // parents control the actual length ofthe
                    // cut/results since there won't be any
                    // coarsening until the parent doesn't satisfy
                    // either.
                    radii.push_back(
                        node->rtnode->parent() != NULL ?
                        node->rtnode->parent()->data().getBounds().radius() :
                        node->rtnode->data().getBounds().radius());
                }
                // FIXME: this fails...
                //assert(radii.size() == results.size());
                std::sort(radii.begin(), radii.end());
                // Want to keep the largest, which are at the end
                // of the sorted list, so count backwards.
                // FIXME something weird is going on where the #
                // of radii doesn't match # of results, so we need
                // some bounds checking
                int offset = std::min(std::max((size_t)0, radii.size()-query_max_results+1), radii.size()-1);
                mMinResultRadius = radii[offset];
                //SILOG(term-region-query, fatal, "^ Updated min radius to " << mMinResultRadius << " " << results.size() << "  min: " << radii[0] << "  max: " << radii[radii.size()-1]);
                mMinResultRadiusRelaxDelayIts = MIN_RADIUS_RELAX_DELAY;
            }
            else {
                // If we're coming in under budget for the length of the
                // cut, slowly let the minimum size slowly get smaller so
                // that.
                if (mMinResultRadiusRelaxDelayIts > 0) {
                    mMinResultRadiusRelaxDelayIts--;
                }
                else {
                    mMinResultRadius = mMinResultRadius * MIN_RADIUS_RELAX_FACTOR;
                    //if (mMinResultRadius != 0) SILOG(term-region-query, fatal, "v Updated min radius to " << mMinResultRadius << " " << results.size());
                }
            }

            validateCut();
            // It would be nice to assert that we're always under budget:
            // assert(results.size() <= query_max_results);
            // but until we clean out entries when we have too many
            // nodes, we can't do this because inserted objects can
            // force themselves into the result set.

            query->pushEvents(events);

            return visited;
        }

    }; // class Cut

    AggregateListenerType* aggregateListener() {
        return QueryHandlerType::mAggregateListener;
    }


    typedef typename TermRegionQueryHandlerImpl::NodeIteratorImpl<SimulationTraits, NodeData> NodeIteratorImpl;
    friend class TermRegionQueryHandlerImpl::NodeIteratorImpl<SimulationTraits, NodeData>;

    virtual NodeIteratorImpl* nodesBeginImpl() const {
        return new NodeIteratorImpl(mRTree->nodesBegin());
    }
    virtual NodeIteratorImpl* nodesEndImpl() const {
        return new NodeIteratorImpl(mRTree->nodesEnd());
    }



    struct QueryState {
        QueryState(TermRegionQueryHandler* _parent, QueryType* _query, RTreeNodeType* root)
        {
            cut = new Cut(_parent, _query, root);
        }

        ~QueryState() {
            delete cut;
        }

        Cut* cut;
    };

private:
    typedef std::tr1::unordered_map<ObjectID, LocCacheIterator, ObjectIDHasher> ObjectSet;
    typedef typename ObjectSet::iterator ObjectSetIterator;
    typedef std::tr1::unordered_map<DetailedQueryType*, QueryState*> QueryMap;
    typedef typename QueryMap::iterator QueryMapIterator;
    typedef typename QueryMap::const_iterator QueryMapConstIterator;

    LocationServiceCacheType* mLocCache;
    LocationUpdateProviderType* mLocUpdateProvider;
    ShouldTrackCallback mShouldTrackCB;

    RTree* mRTree;
    ObjectSet mObjects;
    ObjectSet mNodes;
    QueryMap mQueries;
    Time mLastTime;
    uint16 mElementsPerNode;
    bool mRebuilding;
}; // class TermRegionQueryHandler

} // namespace Sirikata

#endif //_SIRIKATA_TWITTER_PROX_TERM_REGION_QUERY_HANDLER_HPP_
