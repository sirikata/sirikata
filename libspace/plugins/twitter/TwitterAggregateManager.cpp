// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/Platform.hpp>
#include "TwitterAggregateManager.hpp"
#include "Options.hpp"

#include <sirikata/core/command/Commander.hpp>

#include <sirikata/core/network/IOWork.hpp>

#include <sirikata/core/transfer/AggregatedTransferPool.hpp>

#include <sirikata/core/util/Sha256.hpp>
#include <sirikata/core/util/Random.hpp>

#define AGG_LOG(lvl, msg) SILOG(aggregate-manager, lvl, msg)

using namespace std::tr1::placeholders;

namespace Sirikata {

namespace json = json_spirit;

TwitterAggregateManager::TwitterAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username)
 :  mLoc(loc),
    mOAuth(oauth),
    mDirtyAggregateCount(0),
    mAggregateDataCache(1000, JSONValuePtr())
{
    mLoc->addListener(this, true);

    String local_path = GetOptionValue<String>(OPT_TWAGGMGR_LOCAL_PATH);
    String local_url_prefix = GetOptionValue<String>(OPT_TWAGGMGR_LOCAL_URL_PREFIX);
    uint16 n_gen_threads = GetOptionValue<uint16>(OPT_TWAGGMGR_GEN_THREADS);
    uint16 n_upload_threads = GetOptionValue<uint16>(OPT_TWAGGMGR_UPLOAD_THREADS);
    mNumAggregationThreads = n_gen_threads;


    if (mLoc->context()->commander()) {
        mLoc->context()->commander()->registerCommand(
            "space.aggregates.stats",
            std::tr1::bind(&TwitterAggregateManager::commandStats, this, _1, _2, _3)
        );
    }

    mTransferPool = Transfer::TransferMediator::getSingleton().registerClient<Transfer::AggregatedTransferPool>(String("TwitterAggregateManager"));

    // Start aggregation threads
    char id = '1';
    memset(mAggregationThreads, 0, sizeof(mAggregationThreads));
    mAggregationService = new Network::IOService(String("TwitterAggregateManager::AggregationService"));
    mAggregationWork = new Network::IOWork(mAggregationService, String("TwitterAggregateManager::AggregationWork"));
    for (uint8 i = 0; i < mNumAggregationThreads; i++) {
        mAggregationThreads[i] = new Thread(String("TwitterAggregateManager Thread ")+id, std::tr1::bind(&TwitterAggregateManager::aggregationThreadMain, this));

        id++;
    }
}

TwitterAggregateManager::~TwitterAggregateManager() {
    delete mAggregationWork;
    for (uint8 i = 0; i < mNumAggregationThreads; i++) {
        mAggregationThreads[i]->join();
        delete mAggregationThreads[i];
    }
    delete mAggregationService;
}

void TwitterAggregateManager::addLeafObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const Transfer::URI& mesh) {
    AGG_LOG(detailed, "addLeafObject called: uuid=" << uuid);
    Lock lck(mAggregateMutex);
    // We may have already had to create this if misordering of
    // localObjectAdded and addChild occurred.
    AggregateObjectPtr new_agg;
    if (mAggregates.find(uuid) == mAggregates.end()) {
        new_agg = AggregateObjectPtr(new AggregateObject(uuid, false));
        mAggregates[uuid] = new_agg;
    }
    else {
        new_agg = mAggregates[uuid];
        assert(new_agg->mesh == "");
    }
    new_agg->loc = loc;
    new_agg->mesh = mesh.toString();

    // If we already have any parents then this was added because
    // of misordered addChild calls. Make sure we mark things
    // dirty for recomputation of aggregates.
    if (!new_agg->parents.empty()) {
        for(AggregateObjectSet::const_iterator it = new_agg->parents.begin(); it != new_agg->parents.end(); it++)
            markDirtyToRoot(*it, /* child isn't dirty, just a new
                                  * leaf*/ AggregateObjectPtr());
    }
}

void TwitterAggregateManager::removeLeafObject(const UUID& uuid) {
    AGG_LOG(detailed, "removeLeafObject: " << uuid);

    Lock lck(mAggregateMutex);

    assert(aggregateExists(uuid));
    AggregateObjectPtr agg = getAggregate(uuid);
    assert(!agg->aggregate);

    // This can happen before being removed as an aggregate because of a
    // disconnection from the space. In that case, run removeChild for them
    // even though another signal will come again.
    if (agg->parents.empty()) {
        while(!agg->parents.empty())
            removeChild((*agg->parents.begin())->uuid, uuid);
    }
    mAggregates.erase(uuid);
}

void TwitterAggregateManager::addAggregate(const UUID& uuid) {
    AggregateObjectPtr agg(new AggregateObject(uuid, true));

    Lock lck(mAggregateMutex);
    AGG_LOG(detailed, "addAggregate called: uuid=" << uuid);
    mAggregates[uuid] = agg;
}

void TwitterAggregateManager::removeAggregate(const UUID& uuid) {
    Lock lck(mAggregateMutex);
    AGG_LOG(detailed, "removeAggregate: " << uuid);

    assert(aggregateExists(uuid));
    AggregateObjectPtr agg = getAggregate(uuid);
    assert(agg->aggregate);
    assert(agg->children.empty());
    assert(agg->dirty_children.empty()); // shouldn't have any children -> no dirty children
    ensureNotInReadyList(agg);
    assert(!agg->parent);
    mAggregates.erase(uuid);
}

void TwitterAggregateManager::addChild(const UUID& uuid, const UUID& child_uuid) {
    Lock lck(mAggregateMutex);
    AGG_LOG(detailed, "addChild:  "  << uuid << " CHILD " << child_uuid);

    // FIXME These actually aren't safe because the tree processing and location
    // updates occur asynchronously (we can connect an object, add it to the
    // tree, post the update for addChild, then disconnect it and process the
    // disconnection before getting the addChild call). But this should only
    // happen with fast connect/disconnect so we're ignoring it for now.
    assert(aggregateExists(uuid));
    // If we got the addChild before the localObjectAdded call, then
    // we need to fill in a bogus temporary for now so that we can at
    // least setup the tree structure. However, this should only occur
    // for leaf objects.
    if (!aggregateExists(child_uuid)) {
        mAggregates[child_uuid] = AggregateObjectPtr(new AggregateObject(child_uuid, false));
        // We would need to fill in bogus data here except that we're
        // setup ok with empty default values. We really only care
        // about the "mesh" and an empty value is ok there.
    }

    AggregateObjectPtr parent = getAggregate(uuid);
    AggregateObjectPtr agg = getAggregate(child_uuid);
    // Must be an aggregate with no parent or a leaf object
    assert( (agg->aggregate && !agg->parent) ||
        (!agg->aggregate));

    // Also marks dirty
    setParent(agg, parent);
}

void TwitterAggregateManager::removeChild(const UUID& uuid, const UUID& child_uuid) {
    Lock lck(mAggregateMutex);
    AGG_LOG(detailed, "removeChild:  "  << uuid << " CHILD " << child_uuid);

    assert(aggregateExists(uuid));
    // In the case of leaf objects, we can get a forced localObjectRemoved
    // before getting this signal, so it's not safe to assert that the child
    // still exists. In that case, we'll have already cleaned up and there's
    // nothing to do here.
    if (!aggregateExists(child_uuid))
        return;

    AggregateObjectPtr agg = getAggregate(child_uuid);
    AggregateObjectPtr parent = getAggregate(uuid);
    assert( (agg->aggregate && agg->parent && agg->parent->uuid == uuid && agg->parent == parent) ||
        (!agg->aggregate && (agg->parents.find(parent) != agg->parents.end())) );

    if (agg->dirty || agg->processing) {
        // We may have queued it up as dirty multiple times. Make sure
        // we clear them all out. We check processing as well because we could
        // be in the process of generating the child aggregate, having marked it
        // no longer dirty but not yet removing it from the parents
        // dirty_children list
        std::pair<AggregateObjectMultiSetIterator, AggregateObjectMultiSetIterator> range = agg->parent->dirty_children.equal_range(agg);
        agg->parent->dirty_children.erase(range.first, range.second);
    }
    // Parent is dirty no matter what due to removal.
    markDirtyToRoot(agg->parent, AggregateObjectPtr());

    // Remove parent/children pointers
    if (agg->aggregate) {
        agg->parent = AggregateObjectPtr();
        assert(parent->children.find(agg) != parent->children.end());
        parent->children.erase(agg);
    }
    else {
        agg->parents.erase(parent);
        assert(parent->children.find(agg) != parent->children.end());
        parent->children.erase(agg);
    }
}

void TwitterAggregateManager::aggregateObserved(const UUID& objid, uint32 nobservers, uint32 nchildren) {
    // We don't care about these. Maybe we could prioritize using them.
}

void TwitterAggregateManager::generateAggregateMesh(const UUID& uuid, const Duration& delayFor) {
    // We ignore these requests. It's only called by the proximity
    // implementation when bounds change, but we find out about that from loc
    // service updates. I'm not even sure why this exists in the first place...
}



void TwitterAggregateManager::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc,
                                                const TimedMotionQuaternion& orient,
                                                const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                                const String& query_data)
{
}

void TwitterAggregateManager::localObjectRemoved(const UUID& uuid, bool agg) {
}

void TwitterAggregateManager::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
}

void TwitterAggregateManager::localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
}

void TwitterAggregateManager::localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
}

void TwitterAggregateManager::localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {
    // FIXME we should get these updates via AggregateManager
    // (AggregateListener in libprox) interface...

    // We'll get updates about all objects, even if we triggered the update. We
    // can ignore updates of aggregates since we'll have already updated the
    // mesh url.
    if (agg) return;

    AGG_LOG(detailed, "localMeshUpdated: " << uuid);
    Lock lck(mAggregateMutex);

    AggregateObjectPtr aggobj = getAggregate(uuid);
    // Sometimes these can come in after a removal due to posting between threads
    if (!aggobj) return;
    // Don't bother triggering anything unless we see a real change
    if (aggobj->mesh == newval) return;

    aggobj->mesh = newval;
    // Mark dirty nodes. This is a leaf object which can have multiple parents.
    for(AggregateObjectSet::const_iterator it = aggobj->parents.begin(); it != aggobj->parents.end(); it++)
        markDirtyToRoot(*it, /* child isn't dirty, it just changed */ AggregateObjectPtr());
}

void TwitterAggregateManager::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc,
                                                const TimedMotionQuaternion& orient,
                                                const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                                const String& query_data)
{
}

void TwitterAggregateManager::replicaObjectRemoved(const UUID& uuid) {
}

void TwitterAggregateManager::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
}

void TwitterAggregateManager::replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
}

void TwitterAggregateManager::replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval) {
}

void TwitterAggregateManager::replicaMeshUpdated(const UUID& uuid, const String& newval) {
}



void TwitterAggregateManager::commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    //result.put("stats.raw_updates", mRawAggregateUpdates.read());

    cmdr->result(cmdid, result);
}




bool TwitterAggregateManager::aggregateExists(const UUID& uuid) const {
    return (mAggregates.find(uuid) != mAggregates.end());
}

TwitterAggregateManager::AggregateObjectPtr TwitterAggregateManager::getAggregate(const UUID& uuid) const {
    AggregateObjectMap::const_iterator it = mAggregates.find(uuid);
    if (it == mAggregates.end()) return AggregateObjectPtr();
    return it->second;
}

void TwitterAggregateManager::setParent(AggregateObjectPtr child, AggregateObjectPtr parent) {
    if (child->aggregate) {
        child->parent = parent;
        parent->children.insert(child);
    }
    else {
        child->parents.insert(parent);
        parent->children.insert(child);
    }
    if (child->mesh != "" || child->dirty)
        markDirtyToRoot(parent, (child->dirty ? child : AggregateObjectPtr()));
}

void TwitterAggregateManager::ensureNotInReadyList(AggregateObjectPtr agg) {
    AggregateQueue::iterator it = std::find(mReadyAggregates.begin(), mReadyAggregates.end(), agg);
    if (it != mReadyAggregates.end())
        mReadyAggregates.erase(it);
}

void TwitterAggregateManager::ensureInReadyList(AggregateObjectPtr agg) {
    AggregateQueue::iterator it = std::find(mReadyAggregates.begin(), mReadyAggregates.end(), agg);
    if (it == mReadyAggregates.end()) {
        mReadyAggregates.push_back(agg);
        mAggregationService->post(std::tr1::bind(&TwitterAggregateManager::processAggregate, this));
    }
}

void TwitterAggregateManager::markDirtyToRoot(AggregateObjectPtr agg, AggregateObjectPtr new_dirty_child) {
    // Track whether each child was just marked as dirty or was already dirty
    bool just_marked = false;
    bool first = true;
    AggregateObjectPtr child = new_dirty_child;
    while(agg) {
        // If already dirty, could be in ready list. But should only be if it
        // had no dirty children left.
        if (agg->dirty && agg->dirty_children.empty())
            ensureNotInReadyList(agg);
        // If previous aggregate (our child) was just marked, then we now have
        // one more dirty child
        if (just_marked || (first && new_dirty_child))
            agg->dirty_children.insert(child);
        // Once we've made sure we've taken care of counting dirty children,
        // check if we can be placed on the ready list (should only matter for
        // the first aggregate being marked).
        if (agg->dirty_children.empty())
            ensureInReadyList(agg);
        // And make sure just_marked is ready for the next parent, ensuring we
        // set it up before modifying state that would give us the wrong info
        just_marked = !(agg->dirty);

        agg->dirty = true;
        if (just_marked) mDirtyAggregateCount++;
        child = agg;
        agg = agg->parent;

        first = false;
    }
}




// Aggregation threads

void TwitterAggregateManager::aggregationThreadMain() {
    mAggregationService->run();
}

namespace {
// Return true if the service is a default value
bool ServiceIsDefault(const String& s) {
    if (!s.empty() && s != "http" && s != "80")
        return false;
    return true;
}
// Return empty if this is already empty or the default for this
// service. Otherwise, return the old value
String ServiceIfNotDefault(const String& s) {
    if (!ServiceIsDefault(s))
        return s;
    return "80";
}

typedef std::vector<String> StringList;
typedef std::tr1::unordered_map<String, float32> TermFrequencyMap;
struct TweetInfo;
struct ClusterInfo;
struct TweetInfo {
    TweetInfo()
     :cluster(NULL)
    {}

    StringList terms;

    // Clustering info
    ClusterInfo* cluster;
};
typedef std::vector<TweetInfo> TweetInfoList;
typedef std::tr1::unordered_set<TweetInfo*> TweetInfoPtrSet;
struct ClusterInfo {
    TermFrequencyMap frequencies;
    StringList top_terms;
    TweetInfoPtrSet members;
};
typedef std::vector<ClusterInfo> ClusterInfoList;

#define MAX_CLUSTER_ITS 1000
#define NUM_TOP_TERMS   3
#define NUM_CLUSTERS    3

void init_clusters(ClusterInfoList& cluster_list, TweetInfoList& tweet_list) {
    // Create clusters and just split the data between them arbitrarily
    for(uint32 i = 0; i < NUM_CLUSTERS; i++)
        cluster_list.push_back(ClusterInfo());
    for(uint32 i = 0; i < tweet_list.size(); i++) {
        cluster_list[i % NUM_CLUSTERS].members.insert(&tweet_list[i]);
        tweet_list[i].cluster = &(cluster_list[i % NUM_CLUSTERS]);
    }
}

void extract_popular_terms(ClusterInfo& cluster) {
    // Can end up with empty clusters -- too few tweets or just because
    // clustering naturally groups tweet into fewer than the max # of clusters
    if (cluster.members.empty())
        return;

    // Raw counts
    cluster.frequencies.clear();
    for(TweetInfoPtrSet::iterator member_it = cluster.members.begin(); member_it != cluster.members.end(); member_it++) {
        for(uint32 ti = 0; ti < (*member_it)->terms.size(); ti++) {
            String& term = (*member_it)->terms[ti];
            if (cluster.frequencies.find(term) == cluster.frequencies.end())
                cluster.frequencies[term] = 1.f;
            else
                cluster.frequencies[term] += 1.f;
        }
    }
    // Normalize by the number of tweets, giving a value something like the
    // fraction of tweets the term appeared in. This is important for computing
    // similarities later.
    for(TermFrequencyMap::iterator it = cluster.frequencies.begin(); it != cluster.frequencies.end(); it++)
        it->second = it->second / float32(cluster.members.size());

    // Get sorted by inserting into a multimap by frequency
    std::multimap<float32, String> inverted_term_counts;
    for(TermFrequencyMap::iterator it = cluster.frequencies.begin(); it != cluster.frequencies.end(); it++)
        inverted_term_counts.insert(std::make_pair(it->second, it->first));
    // And grab the last N terms
    while(inverted_term_counts.size() > NUM_TOP_TERMS)
        inverted_term_counts.erase(inverted_term_counts.begin());
    cluster.top_terms.clear();
    for(std::multimap<float32, String>::iterator it = inverted_term_counts.begin(); it != inverted_term_counts.end(); it++)
        cluster.top_terms.push_back(it->second);
}

void update_popular_terms(ClusterInfoList& cluster_list) {
    for(uint32 i = 0; i < cluster_list.size(); i++)
        extract_popular_terms(cluster_list[i]);
}

// Returns true if any membership changes occurred
bool update_membership(ClusterInfoList& cluster_list, TweetInfoList& tweet_list) {
    bool any_changed = false;
    for(TweetInfoList::iterator tw_it = tweet_list.begin(); tw_it != tweet_list.end(); tw_it++) {
        TweetInfo& tweet = *tw_it;

        float32 best_score = 0.f;
        ClusterInfo* best_cluster = NULL;
        for(ClusterInfoList::iterator cl_it = cluster_list.begin(); cl_it != cluster_list.end(); cl_it++) {
            ClusterInfo& cluster = *cl_it;
            // Compute a value for this tweet in this cluster. Just scan through
            // terms and look for any matches, adding the normalized frequency
            // from the cluster.
            float32 score = 0.f;
            for(StringList::iterator term_it = tweet.terms.begin(); term_it != tweet.terms.end(); term_it++) {
                if (cluster.frequencies.find(*term_it) == cluster.frequencies.end()) continue;
                score += cluster.frequencies[*term_it];
            }
            if (best_cluster == NULL || score > best_score) {
                best_score = score;
                best_cluster = &cluster;
            }
        }

        // Update pointers: tweet <--> cluster
        assert(best_cluster != NULL);
        any_changed = any_changed || (best_cluster != tweet.cluster);
        if (tweet.cluster != NULL)
            tweet.cluster->members.erase(&tweet);
        tweet.cluster = best_cluster;
        tweet.cluster->members.insert(&tweet);
    }
    return any_changed;
}

}


void TwitterAggregateManager::processAggregate() {

    AggregateObjectPtr agg;
    // Because other threads may be updating while we're processing an
    // aggregate, we make copies of the data we need to process them.
    AggregateObjectSet parents;
    typedef std::vector<Transfer::URL> DataURLList;
    DataURLList child_data_urls;

    Time started = Timer::now();
    {
        Lock lck(mAggregateMutex);
        AGG_LOG(insane, "Checking for ready aggregates: " << mReadyAggregates.size());
        for(AggregateQueue::const_iterator it = mReadyAggregates.begin(); it != mReadyAggregates.end(); it++) {
            // If it's already processing from being added before, ignore it for
            // now. We'll return to it later to re-process to handle the updated
            // state.
            if ((*it)->processing) continue;

            agg = *it;
            assert(agg->dirty);
            assert(agg->dirty_children.empty());
            agg->processing = true;
            agg->dirty = false;
            mDirtyAggregateCount--;
            ensureNotInReadyList(agg);

            // We need to record parents now while we have the lock. Because we
            // marked this aggregate as no longer dirty, the other code managing
            // objects while we're processing won't update dirty counts
            // properly. We record the info here and make sure we clean up
            // properly after we're done processing, when it'll be safe to
            // remove the objects from the list, allowing parents to move into
            // the ready list.
            if (agg->aggregate) {
                if (agg->parent)
                    parents.insert(agg->parent);
            }
            else {
                assert(!agg->parents.empty());
                parents = agg->parents;
            }

            AGG_LOG(detailed, "Processing aggregate " << agg->uuid << " with " << agg->children.size() << " children");

            for(AggregateObjectSet::const_iterator child_it = agg->children.begin(); child_it != agg->children.end(); child_it++) {
                if (!(*child_it)->mesh.empty())
                    child_data_urls.push_back(Transfer::URL((*child_it)->mesh));
            }
            break;
        }
    }
    if (!agg) return;

    // Grab and parse all child data. These happen together in one step so we
    // can cache parsed data.
    typedef std::vector<JSONValuePtr> JSONList;
    JSONList children_json;
    {
        std::vector<Transfer::ResourceDownloadTaskPtr> downloads;
        boost::mutex mutex;
        DataMap children_data;
        uint32 children_left = 0;
        boost::condition_variable all_children_ready;

        {
            boost::unique_lock<boost::mutex> lock(mutex);
            for(DataURLList::const_iterator child_url_it = child_data_urls.begin(); child_url_it != child_data_urls.end(); child_url_it++) {
                JSONValuePtr child_data;
                {
                    Lock lock(mAggregateDataCacheMutex);
                    child_data = mAggregateDataCache.get(*child_url_it);
                }
                if (child_data) {
                    children_json.push_back(child_data);
                }
                else {
                    Transfer::ResourceDownloadTaskPtr req(
                        Transfer::ResourceDownloadTask::construct(
                            Transfer::URI(child_url_it->toString()), mTransferPool, 1.0,
                            std::tr1::bind(&TwitterAggregateManager::finishedDownload, this, *child_url_it, _1, _2, _3, &mutex, &children_left, &children_data, &all_children_ready)
                        )
                    );
                    downloads.push_back(req);
                    req->start();
                    children_left++;
                }
            }
            while(children_left > 0)
                all_children_ready.wait(lock);
        }

        // Parse data
        for(DataMap::const_iterator data_it = children_data.begin(); data_it != children_data.end(); data_it++) {
            std::string child_json((char*)data_it->second->begin(), data_it->second->size());
            JSONValuePtr child_data(new json::Value());
            if (!json::read(child_json, *child_data)) {
                AGG_LOG(error, "Couldn't parse new environment: " << child_json);
            }
            else {
                children_json.push_back(child_data);
                Lock lock(mAggregateDataCacheMutex);
                mAggregateDataCache.insert(data_it->first, child_data);
            }
        }
    }

    // Create the actual aggregate data
    String aggregate_json_serialized;
    {

        json::Value agg_json = json::Object();

        // Clustering to determine
        {
            // Extract terms in more convenient form
            TweetInfoList tweet_infos;
            for(JSONList::const_iterator data_it = children_json.begin(); data_it != children_json.end(); data_it++) {
                const json::Array& child_tweets = (*data_it)->getArray("tweets");
                for(json::Array::const_iterator tw_it = child_tweets.begin(); tw_it != child_tweets.end(); tw_it++) {
                    TweetInfo tweet;
                    const json::Array& terms_list = tw_it->getArray("terms");
                    for(json::Array::const_iterator term_it = terms_list.begin(); term_it != terms_list.end(); term_it++) {
                        if (!term_it->isString()) continue;
                        tweet.terms.push_back(term_it->getString());
                    }
                    tweet_infos.push_back(tweet);
                }
            }

            ClusterInfoList clusters;
            init_clusters(clusters, tweet_infos);
            bool some_changed = true;
            int it;
            for(it = 0; some_changed && it < MAX_CLUSTER_ITS; it++) {
                update_popular_terms(clusters);
                some_changed = update_membership(clusters, tweet_infos);
            }
            // One last update of the terms
            update_popular_terms(clusters);

            AGG_LOG(fatal, "Finished clustering " << tweet_infos.size() << " tweets after " << it << " iterations");

            agg_json.put("clusters", json::Array());
            json::Array& agg_clusters = agg_json.getArray("clusters");
            for(uint32 i = 0; i < clusters.size(); i++) {
                if (clusters[i].members.empty()) continue;
                json::Object obj;
                json::Value cluster_json(obj);
                json::Array terms_json;
                for(StringList::iterator ti = clusters[i].top_terms.begin(); ti != clusters[i].top_terms.end(); ti++) {
                    json::Object term_details_obj;
                    json::Value term_details(term_details_obj);
                    term_details.put("term", *ti);
                    term_details.put("frequency", clusters[i].frequencies[*ti]);
                    terms_json.push_back(term_details);
                }
                cluster_json.put("terms", terms_json);
                agg_clusters.push_back(cluster_json);
            }
        }

        // Select a random (representative) subset of the data
        {
            agg_json.put("tweets", json::Array());
            json::Array& agg_tweets = agg_json.getArray("tweets");

            uint32 total_tweet_count = 0;
            for(JSONList::const_iterator data_it = children_json.begin(); data_it != children_json.end(); data_it++) {
                const json::Array& child_tweets = (*data_it)->getArray("tweets");
                total_tweet_count += child_tweets.size();
            }
            // Select random subset to cap the maximum number of tweets in an
            // aggregate.
#define MAX_AGGREGATE_TWEETS 500
            uint32 subset_tweet_count = std::min(total_tweet_count, (uint32)MAX_AGGREGATE_TWEETS);
            float64 subset_tweet_frac = subset_tweet_count / (float64)total_tweet_count;

            for(JSONList::const_iterator data_it = children_json.begin(); data_it != children_json.end(); data_it++) {
                const json::Array& child_tweets = (*data_it)->getArray("tweets");
                for(json::Array::const_iterator tw_it = child_tweets.begin(); tw_it != child_tweets.end(); tw_it++)
                    if (agg_tweets.size() < subset_tweet_count &&
                        (subset_tweet_frac == 1.0 || randFloat() <= subset_tweet_frac))
                        agg_tweets.push_back(*tw_it);
            }
        }

        aggregate_json_serialized = json::write(agg_json);
    }

    // URL to post to is based on the hash of the data.
    String aggregate_hash = SHA256::computeDigest(aggregate_json_serialized).toString();

    // Upload the new aggregate "mesh". FIXME this should really be using OAuth
    // or something to secure it. We just use the oauth host/service settings
    // currently
    {
        assert(!mOAuth->hostname.empty() && !mOAuth->service.empty());

        boost::mutex mutex;
        boost::condition_variable upload_finished;
        boost::unique_lock<boost::mutex> lock(mutex);
        // FIXME this sort of matches name + chunk approach, but there's not upload
        // URL for a resource, so there's no way to extract all the info we would
        // need. We need some other way of specifying that through the request,
        // i.e. a more generic version of OAuthParams
        Transfer::HttpManager::getSingleton().post(
            Network::Address(Network::Address(mOAuth->hostname, mOAuth->service)),
            String("/") + aggregate_hash, "text/json", aggregate_json_serialized,
            std::tr1::bind(&TwitterAggregateManager::uploadFinished, this, _1, _2, _3, &mutex, &upload_finished)
        );
        // And wait for it to finish. No check of condition because this is one
        // producer, one consumer.
        upload_finished.wait(lock);
    }

    // New URL is just based on the the hash of the data
    String new_url = "http://" + mOAuth->hostname + ":" + mOAuth->service + "/" + aggregate_hash;

    // Update loc with the new mesh data
    mLoc->context()->mainStrand->post(
        std::tr1::bind(
            &TwitterAggregateManager::updateAggregateLocMesh, this,
            agg->uuid, new_url
        ),
        "TwitterAggregateManager::updateAggregateLocMesh"
    );

    {
        // Mark as finished processing, clean, and add any parent(s) that now no
        // longer have dirty children
        Lock lck(mAggregateMutex);
        Time finished = Timer::now();
        AGG_LOG(detailed, "Finished processing aggregate " << agg->uuid << ", " << child_data_urls.size() << " children and " << (finished-started) << ". " << mDirtyAggregateCount << " more dirty aggregates remain.");

        // We're going to get an update from loc eventually too, but we update
        // this immediately because we could start processing the parent and we
        // want this data up to date right away.
        agg->mesh = new_url;

        agg->processing = false;

        // Whether we've been marked dirty again while generating the aggregate,
        // we need to reduce dirty children counts: when not dirtied, the logic
        // is simple, when re-dirtied we will have double-incremented to account
        // for both times marked dirty.
        for(AggregateObjectSet::const_iterator it = parents.begin(); it != parents.end(); it++) {
            // Because we can have multiple entries in dirty_children
            // but can also have the tree moving around while we're
            // processing, we can't assume parents still
            // match. If this happens, a parent may have already been
            // put in the queue and processed since they no longer see
            // us as a dirty child.
            if ((*it)->children.find(agg) == (*it)->children.end()) {
                // We're no longer even a child. Our state should have
                // been cleaned out of the parent and we no longer
                // have an effect on it
                continue;
            }
            // Otherwise, we should still be a dirty child
            assert((*it)->dirty);
            assert(!(*it)->dirty_children.empty());
            assert((*it)->dirty_children.find(agg) != (*it)->dirty_children.end());
            (*it)->dirty_children.erase( (*it)->dirty_children.find(agg) );
        }

        // If we got re-dirtied and added back to the ready list while we were
        // processing, just set ourselves up for more processing.
        if (agg->dirty) {
            // Only need to set ourselves up no children have been dirtied.
            if (agg->dirty_children.empty()) {
                assert(std::find(mReadyAggregates.begin(), mReadyAggregates.end(), agg) != mReadyAggregates.end());
                mAggregationService->post(std::tr1::bind(&TwitterAggregateManager::processAggregate, this));
            }
        }
        else {
            // Otherwise, we're good for now and can allow parent(s) to proceed.
            for(AggregateObjectSet::const_iterator it = parents.begin(); it != parents.end(); it++) {
                if ((*it)->dirty_children.empty())
                    ensureInReadyList(*it);
            }
        }
    }
}

void TwitterAggregateManager::finishedDownload(Transfer::URL data_url, Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request, Transfer::DenseDataPtr response, boost::mutex* mutex, uint32* children_left, DataMap* children_data, boost::condition_variable* all_children_ready) {
    boost::unique_lock<boost::mutex> lock(*mutex);
    // Currently we'll just ignore failures to get data. Not ideal, but should
    // only hurt results slightly since it should be infrequent
    if (response)
        (*children_data)[data_url] = response;
    (*children_left)--;
    if (*children_left == 0)
        all_children_ready->notify_one();
}

void TwitterAggregateManager::uploadFinished(
        Transfer::HttpManager::HttpResponsePtr response,
        Transfer::HttpManager::ERR_TYPE error,
        const boost::system::error_code& boost_error,
        boost::mutex* mutex, boost::condition_variable* upload_finished)
{
    if (!upload_finished) {
        AGG_LOG(error, "Aggregate upload failed");
    }

    boost::unique_lock<boost::mutex> lock(*mutex);
    upload_finished->notify_one();
}


void TwitterAggregateManager::updateAggregateLocMesh(UUID uuid, String mesh) {
    if (mLoc->contains(uuid)) {
        mLoc->updateLocalAggregateMesh(uuid, mesh);
    }
}

}
