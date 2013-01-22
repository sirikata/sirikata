// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_TWITTER_TERM_BLOOM_FILTER_NODE_DATA_HPP_
#define _SIRIKATA_TWITTER_TERM_BLOOM_FILTER_NODE_DATA_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <prox/rtree/RTreeCore.hpp>
#include <sirikata/twitter/TermBloomFilter.hpp>

namespace Sirikata {

/** Maintains bounding sphere, maximum sized object, and bloom filter for search
 *  terms.
 */
template<typename SimulationTraits, typename NodeData>
class TermBloomFilterNodeDataBase : public Prox::BoundingSphereDataBase<SimulationTraits, NodeData> {
public:
    typedef Prox::BoundingSphereDataBase<SimulationTraits, NodeData> ThisBase;

    typedef typename SimulationTraits::ObjectIDType ObjectID;
    typedef typename SimulationTraits::realType Real;
    typedef typename SimulationTraits::Vector3Type Vector3;
    typedef typename SimulationTraits::BoundingSphereType BoundingSphere;
    typedef typename SimulationTraits::SolidAngleType SolidAngle;
    typedef typename SimulationTraits::TimeType Time;
    typedef Prox::LocationServiceCache<SimulationTraits> LocationServiceCacheType;
    typedef typename LocationServiceCacheType::Iterator LocCacheIterator;

private:
    TermBloomFilterNodeDataBase();
public:
    TermBloomFilterNodeDataBase(uint32 buckets, uint16 hashes)
     : Prox::BoundingSphereDataBase<SimulationTraits, NodeData>(),
       mMaxRadius(0.f),
       mFilter(buckets, hashes)
    {
    }

    TermBloomFilterNodeDataBase(uint32 buckets, uint16 hashes, LocationServiceCacheType* loc, const LocCacheIterator& obj_id, const Time& t)
     : Prox::BoundingSphereDataBase<SimulationTraits, NodeData>(loc, obj_id, t),
       mMaxRadius( loc->maxSize(obj_id) ),
       mFilter(buckets, hashes)
    {
        // Note: we override this here because we need worldCompleteBounds for
        // just the bounds data, but with the max size values, we can use the
        // smaller worldRegion along with the maximum size object.  Note
        // difference in satisfiesConstraints
        ThisBase::bounding_sphere = loc->worldRegion(obj_id, t);

        if (!loc->queryData(obj_id).empty()) {
            mFilter.deserialize(loc->queryData(obj_id));
        }
    }

    NodeData merge(const NodeData& other) const {
        NodeData result = Prox::BoundingSphereDataBase<SimulationTraits, NodeData>::merge(other);
        result.mMaxRadius = std::max( mMaxRadius, other.mMaxRadius );
        result.mFilter.mergeIn(other.mFilter);
        return result;
    }

    // Merge the given info into this info
    void mergeIn(const NodeData& other, uint32 currentChildrenCount) {
        Prox::BoundingSphereDataBase<SimulationTraits, NodeData>::mergeIn(other, currentChildrenCount);
        mMaxRadius = std::max( mMaxRadius, other.mMaxRadius );

        mFilter.mergeIn(other.mFilter);
    }

    // Check if this data satisfies the query constraints given
    bool satisfiesConstraints(const Vector3& qpos, const BoundingSphere& qregion, const float qmaxsize, const SolidAngle& qangle, const float qradius) const {
        // We create a virtual stand in object which is the worst case object that could be in this subtree.
        // It's centered at the closest point on the hierarchical bounding sphere to the query, and has the
        // largest radius of any objects in the subtree.

        Vector3 obj_pos = ThisBase::bounding_sphere.center();
        float obj_radius = ThisBase::bounding_sphere.radius();

        return (Prox::satisfiesConstraintsBoundsAndMaxSize<SimulationTraits>(obj_pos, obj_radius, mMaxRadius, qpos, qregion, qmaxsize, qangle, qradius) != -1);
    }
    // Get the score (or -1) for this data, given the query constraints
    float32 score(const Vector3& qpos, const BoundingSphere& qregion, const float qmaxsize, const SolidAngle& qangle, const float qradius) const {
        Vector3 obj_pos = ThisBase::bounding_sphere.center();
        float obj_radius = ThisBase::bounding_sphere.radius();

        return Prox::satisfiesConstraintsBoundsAndMaxSize<SimulationTraits>(obj_pos, obj_radius, mMaxRadius, qpos, qregion, qmaxsize, qangle, qradius);
    }

    // Given an object and a time, select the best child node to put the object in
    template<typename CutNode>
    static Prox::RTreeNode<SimulationTraits, NodeData, CutNode>* selectBestChildNode(
        const Prox::RTreeNode<SimulationTraits, NodeData, CutNode>* node,
        LocationServiceCacheType* loc, const LocCacheIterator& obj_id, const Time& t)
    {
        typedef Prox::RTreeNode<SimulationTraits, NodeData, CutNode> RTreeNodeType;

        float min_increase = 0.f;
        RTreeNodeType* min_increase_node = NULL;

        BoundingSphere obj_bounds = loc->worldRegion(obj_id, t);
        float obj_max_size = loc->maxSize(obj_id);

        for(int i = 0; i < node->size(); i++) {
            RTreeNodeType* child_node = node->node(i);
            BoundingSphere merged = child_node->data().bounding_sphere.merge(obj_bounds);
            float new_max_size = std::max(child_node->data().mMaxRadius, obj_max_size);
            BoundingSphere old_total( child_node->data().bounding_sphere.center(), child_node->data().bounding_sphere.radius() + child_node->data().mMaxRadius );
            BoundingSphere total(merged.center(), merged.radius() + new_max_size);
            float increase = total.volume() - old_total.volume();
            if (min_increase_node == NULL || increase < min_increase) {
                min_increase = increase;
                min_increase_node = child_node;
            }
        }

        return min_increase_node;
    }
    template<typename CutNode>
    static Prox::RTreeNode<SimulationTraits, NodeData, CutNode>* selectBestChildNodeFromPair(
        Prox::RTreeNode<SimulationTraits, NodeData, CutNode>* n1,
        Prox::RTreeNode<SimulationTraits, NodeData, CutNode>* n2,
        LocationServiceCacheType* loc, const LocCacheIterator& obj_id, const Time& t)
    {
        typedef Prox::RTreeNode<SimulationTraits, NodeData, CutNode> RTreeNodeType;

        float min_increase = 0.f;
        RTreeNodeType* min_increase_node = NULL;

        BoundingSphere obj_bounds = loc->worldRegion(obj_id, t);
        float obj_max_size = loc->maxSize(obj_id);

        RTreeNodeType* nodes[2] = { n1, n2 };
        for(int i = 0; i < 2; i++) {
            RTreeNodeType* child_node = nodes[i];
            BoundingSphere merged = child_node->data().bounding_sphere.merge(obj_bounds);
            float new_max_size = std::max(child_node->data().mMaxRadius, obj_max_size);
            BoundingSphere old_total( child_node->data().bounding_sphere.center(), child_node->data().bounding_sphere.radius() + child_node->data().mMaxRadius );
            BoundingSphere total(merged.center(), merged.radius() + new_max_size);
            float increase = total.volume() - old_total.volume();
            if (min_increase_node == NULL || increase < min_increase) {
                min_increase = increase;
                min_increase_node = child_node;
            }
        }

        return min_increase_node;
    }

    void verifyChild(const NodeData& child) const {
        Prox::BoundingSphereDataBase<SimulationTraits, NodeData>::verifyChild(child);

        if ( child.mMaxRadius > mMaxRadius) {
            printf(
                "Child radius greater than recorded maximum child radius: %f > %f\n",
                child.mMaxRadius, mMaxRadius
            );
        }
    }

    /** Gets the current bounds of the node.  This should be the true,
     *  static bounds of the objects, not just the region they cover.
     *  Use of this method (to generate aggregate object Loc
     *  information) currently assumes we're not making aggregates be
     *  moving objects.
     */
    BoundingSphere getBounds() const {
        return BoundingSphere( ThisBase::bounding_sphere.center(), ThisBase::bounding_sphere.radius() + mMaxRadius );
    }

    Vector3 getBoundsCenter() const {
        return ThisBase::bounding_sphere.center();
    }
    Real getBoundsCenterBoundsRadius() const {
        return ThisBase::bounding_sphere.radius();
    }
    Real getBoundsMaxObjectSize() const {
        return mMaxRadius;
    }

    /** Gets the volume of this bounds of this region. */
    float volume() const {
        return getBounds().volume();
    }

    /** Get the radius within which a querier asking for the given minimum solid
     *  angle will get this data as a result, i.e. the radius within which this
     *  node will satisfy the given query.
     */
    float getValidRadius(const SolidAngle& min_sa) const {
        // There's a minimum value based on when we end up *inside* the volume
        float bounds_max = getBounds().radius() + mMaxRadius;
        // Otherwise, we just invert the solid angle formula
        float sa_max = min_sa.maxDistance(mMaxRadius) + getBounds().radius();
        return std::max( bounds_max, sa_max );
    }

    static float hitProbability(const NodeData& parent, const NodeData& child) {
        static SolidAngle rep_sa(.01); // FIXME
        float parent_max_rad = parent.getValidRadius(rep_sa);
        float child_max_rad = child.getValidRadius(rep_sa);
        float ratio = child_max_rad / parent_max_rad;
        return ratio*ratio;
    }

private:
    float mMaxRadius;
    Twitter::TermBloomFilter mFilter;
};


} // namespace Sirikata

#endif //_SIRIKATA_TWITTER_TERM_BLOOM_FILTER_NODE_DATA_HPP_
