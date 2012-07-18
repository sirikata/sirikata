/*  Sirikata
 *  LocalPintoServerQuerier.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIRIKATA_LIBSPACE_LOCAL_PINTO_SERVER_QUERIER_HPP_
#define _SIRIKATA_LIBSPACE_LOCAL_PINTO_SERVER_QUERIER_HPP_

#include <sirikata/space/PintoServerQuerier.hpp>

#include "Protocol_Prox.pbj.hpp"

namespace Sirikata {

/** LocalPintoServerQuerier is a dummy implementation of PintoServerQuerier for
 *  single server setups.  Since there is only one server, only a minimal set of
 *  results are returned.
 *
 *  Previous versions of this class did nothing, but for some Proximity
 *  implementations this isn't sufficient since they expect results from the
 *  Pinto server before they'll progress to evaluating any objects on any
 *  server. For these implementations, this class does implement what amounts to
 *  a single-node Pinto tree where the only node is for the only space server in
 *  the system and the result is always return immediately (though
 *  asynchronously), purely for bootstrapping.
 */
class LocalPintoServerQuerier : public PintoServerQuerier {
public:
    LocalPintoServerQuerier(SpaceContext* ctx)
     : mContext(ctx),
       mNotified(false),
       mRegion(),
       mLargest(),
       mRootNodeID(UUID::random()),
       mSeqnoSource(1)
    {}
    virtual ~LocalPintoServerQuerier() {}

    // Service Interface
    virtual void start() {
        // This is a bit odd, but we actually just send results as soon as
        // things are ready to start.

        // To be compatible with all types of query processors, we have to at
        // least pretend there's a top-level pinto tree here. To make
        // compatibility simple, we'll just assume it's a single node tree and
        // that when this space server (the only space server) updates it's
        // query, we only need to notify it once of the update -- essentially we
        // set the cut at the root (and leaf) and we're done forever. Do it
        // async so it's like it's coming from the network and so we don't have
        // any issues with it running within the updateQuery request.

        // But because of the way other implementations behave, we just want to
        // send out the result (i.e. to this server) immediately.  Other
        // implementations connect to a server and have an implicit query
        // initialization. We don't have any connection to make, but we can just
        // pretend we've connected and gotten the info back, but post it async
        // so it's like it came from the network.

        mContext->mainStrand->post(
            std::tr1::bind(&LocalPintoServerQuerier::notifyResult, this)
        );
    }
    virtual void stop() {}

    // PintoServerQuerier Interface
    virtual void updateRegion(const BoundingBox3f& region) {
        mRegion = region;
    }
    virtual void updateLargestObject(float max_radius) {
        mLargest = max_radius;
    }

    virtual void updateQuery(const String& update) {
    }

private:
    void notifyResult() {
        // Construct dummy update consisting of single node addition of leaf
        // node with our own ID so recursion to our own data will proceed where
        // an update like this is necessary for that to happen. It's actually a
        // bit more complicated because we don't actually support single node
        // trees -- we need at least one internal node and then the leaf node.
        Sirikata::Protocol::Prox::ProximityUpdate prox_update;

        Sirikata::Protocol::Prox::IIndexProperties index_props = prox_update.mutable_index_properties();
        index_props.set_id(1);
        index_props.set_index_id( boost::lexical_cast<String>(NullServerID) );
        index_props.set_dynamic_classification(Sirikata::Protocol::Prox::IndexProperties::Static);


        // Single addition of fake root
        {
            Sirikata::Protocol::Prox::IObjectAddition addition = prox_update.add_addition();
            // Shoe-horn server ID into UUID
            addition.set_object(mRootNodeID);

            addition.set_seqno (mSeqnoSource++);

            Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
            motion.set_t(mContext->simTime());
            motion.set_position(mRegion.center());
            motion.set_velocity(Vector3f(0,0,0));

            Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
            msg_orient.set_t(Time::null());
            msg_orient.set_position(Quaternion::identity());
            msg_orient.set_velocity(Quaternion::identity());

            Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
            msg_bounds.set_center_offset( Vector3f(0,0,0) );
            msg_bounds.set_center_bounds_radius( mRegion.toBoundingSphere().radius() );
            msg_bounds.set_max_object_size(mLargest);

            addition.set_type(Sirikata::Protocol::Prox::ObjectAddition::Aggregate);
        }


        // Single addition of our own node
        {
            Sirikata::Protocol::Prox::IObjectAddition addition = prox_update.add_addition();
            // Shoe-horn server ID into UUID
            addition.set_object(UUID((uint32)mContext->id()));

            addition.set_seqno (mSeqnoSource++);

            Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
            motion.set_t(mContext->simTime());
            motion.set_position(mRegion.center());
            motion.set_velocity(Vector3f(0,0,0));

            Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
            msg_orient.set_t(Time::null());
            msg_orient.set_position(Quaternion::identity());
            msg_orient.set_velocity(Quaternion::identity());

            Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
            msg_bounds.set_center_offset( Vector3f(0,0,0) );
            msg_bounds.set_center_bounds_radius( mRegion.toBoundingSphere().radius() );
            msg_bounds.set_max_object_size(mLargest);

            addition.set_type(Sirikata::Protocol::Prox::ObjectAddition::Object);
            addition.set_parent(mRootNodeID);
        }

        notify(&PintoServerQuerierListener::onPintoServerResult, prox_update);
    }
    SpaceContext* mContext;
    bool mNotified;
    BoundingBox3f3f mRegion;
    float32 mLargest;
    UUID mRootNodeID;
    uint64 mSeqnoSource;
}; // PintoServerQuerier

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_LOCAL_PINTO_SERVER_QUERIER_HPP_
