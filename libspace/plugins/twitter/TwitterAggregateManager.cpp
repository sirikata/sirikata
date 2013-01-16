// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/Platform.hpp>
#include "TwitterAggregateManager.hpp"
#include "Options.hpp"

#include <sirikata/core/command/Commander.hpp>

#include <sirikata/core/network/IOWork.hpp>

#define AGG_LOG(lvl, msg) SILOG(aggregate-manager, lvl, msg)

using namespace std::tr1::placeholders;

namespace Sirikata {

TwitterAggregateManager::TwitterAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username)
 :  mLoc(loc)
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
    assert(aggregateExists(child_uuid));

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

    if (agg->dirty)
        agg->parent->dirty_children.erase( agg->parent->dirty_children.find(agg) );
    // Parent is dirty no matter what due to removal.
    markDirtyToRoot(agg->parent, AggregateObjectPtr());

    // Remove parent/children pointers
    if (agg->aggregate) {
        agg->parent = AggregateObjectPtr();
        parent->children.erase(agg);
    }
    else {
        agg->parents.erase(parent);
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
                                                const String& zernike)
{
    if (!agg) {
        AGG_LOG(detailed, "localObjectAdded called: uuid=" << uuid);
        AggregateObjectPtr new_agg(new AggregateObject(uuid, agg));
        new_agg->loc = loc;
        new_agg->mesh = mesh;
        Lock lck(mAggregateMutex);
        mAggregates[uuid] = new_agg;
    }
}

void TwitterAggregateManager::localObjectRemoved(const UUID& uuid, bool agg) {
    if (!agg) {
        AGG_LOG(detailed, "localObjectRemoved: " << uuid);

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
}

void TwitterAggregateManager::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
}

void TwitterAggregateManager::localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
}

void TwitterAggregateManager::localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
}

void TwitterAggregateManager::localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {
}

void TwitterAggregateManager::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc,
                                                const TimedMotionQuaternion& orient,
                                                const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                                const String& zernike)
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
        child = agg;
        agg = agg->parent;

        first = false;
    }
}




// Aggregation threads

void TwitterAggregateManager::aggregationThreadMain() {
    mAggregationService->run();
}

void TwitterAggregateManager::processAggregate() {
    AggregateObjectPtr agg;
    AggregateObjectSet parents;
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

            AGG_LOG(detailed, "Processing aggregate " << agg->uuid);
            break;
        }
    }
    if (!agg) return;


    {
        // Mark as finished processing, clean, and add any parent(s) that now no
        // longer have dirty children
        Lock lck(mAggregateMutex);
        AGG_LOG(detailed, "Finished processing aggregate " << agg->uuid);

        agg->processing = false;

        // Whether we've been marked dirty again while generating the aggregate,
        // we need to reduce dirty children counts: when not dirtied, the logic
        // is simple, when re-dirtied we will have double-incremented to account
        // for both times marked dirty.
        for(AggregateObjectSet::const_iterator it = parents.begin(); it != parents.end(); it++) {
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

}
