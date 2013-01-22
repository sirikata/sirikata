// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_COPYABLE_LOC_UPDATE_HPP_
#define _SIRIKATA_COPYABLE_LOC_UPDATE_HPP_

#include <sirikata/pintoloc/Platform.hpp>
#include <sirikata/pintoloc/LocUpdate.hpp>

namespace Sirikata {

/** Implementation of LocUpdate which can be copied and stores all its data
 *  directly (as opposed to, e.g., protocol or PresenceProperties versions which
 *  are backed by other, potentially temporary, values). This is useful if you
 *  need to make a copy of a LocUpdate (any kind), e.g. in order to defer
 *  processing.
 */
class CopyableLocUpdate : public LocUpdate {
public:
    CopyableLocUpdate(const LocUpdate& rhs)
     : mObject(rhs.object()),
       mUpdate(),
       mHasEpoch(rhs.has_epoch()),
       mEpoch(rhs.has_epoch() ? rhs.epoch() : 0)
    {
        copyData(rhs);
    }
    // Need to explicitly override this to get past Noncopyable
    CopyableLocUpdate(const CopyableLocUpdate& rhs)
     : mObject(rhs.object()),
       mUpdate(),
       mHasEpoch(rhs.has_epoch()),
       mEpoch(rhs.has_epoch() ? rhs.epoch() : 0)
    {
        copyData(rhs);
    }
    virtual ~CopyableLocUpdate() {}

    virtual ObjectReference object() const { return mObject; }

    // Request epoch
    virtual bool has_epoch() const { return mHasEpoch; }
    virtual uint64 epoch() const { return mEpoch; }

    // Parent aggregate
    virtual bool has_parent() const { return mHasPart[SequencedPresenceProperties::LOC_PARENT_PART]; }
    virtual ObjectReference parent() const { return ObjectReference(mUpdate.parent()); }
    virtual uint64 parent_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_PARENT_PART); }

    // Location
    virtual bool has_location() const { return mHasPart[SequencedPresenceProperties::LOC_POS_PART]; }
    virtual TimedMotionVector3f location() const { return mUpdate.location(); }
    virtual uint64 location_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_POS_PART); }

    // Orientation
    virtual bool has_orientation() const { return mHasPart[SequencedPresenceProperties::LOC_ORIENT_PART]; }
    virtual TimedMotionQuaternion orientation() const { return mUpdate.orientation(); }
    virtual uint64 orientation_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_ORIENT_PART); }

    // Bounds
    virtual bool has_bounds() const { return mHasPart[SequencedPresenceProperties::LOC_BOUNDS_PART]; }
    virtual AggregateBoundingInfo bounds() const { return mUpdate.bounds(); }
    virtual uint64 bounds_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_BOUNDS_PART); }

    // Mesh
    virtual bool has_mesh() const { return mHasPart[SequencedPresenceProperties::LOC_MESH_PART]; }
    virtual String mesh() const { return mUpdate.mesh().toString(); }
    virtual uint64 mesh_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_MESH_PART); }

    // Physics
    virtual bool has_physics() const { return mHasPart[SequencedPresenceProperties::LOC_PHYSICS_PART]; }
    virtual String physics() const { return mUpdate.physics(); }
    virtual uint64 physics_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_PHYSICS_PART); }

    // Query data
    virtual bool has_query_data() const { return mHasPart[SequencedPresenceProperties::LOC_QUERY_DATA_PART]; }
    virtual String query_data() const { return mUpdate.queryData(); }
    virtual uint64 query_data_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_QUERY_DATA_PART); }

    virtual uint32 index_id_size() const { return mIndexIDs.size(); }
    virtual ProxIndexID index_id(int32 idx) const { return mIndexIDs[idx]; }
    virtual uint64 index_id_seqno() const { return mIndexIDsSeqno; }

private:
    CopyableLocUpdate();

    void copyData(const LocUpdate& rhs) {
        mHasPart[SequencedPresenceProperties::LOC_PARENT_PART] = rhs.has_parent();
        mHasPart[SequencedPresenceProperties::LOC_POS_PART] = rhs.has_location();
        mHasPart[SequencedPresenceProperties::LOC_ORIENT_PART] = rhs.has_orientation();
        mHasPart[SequencedPresenceProperties::LOC_BOUNDS_PART] = rhs.has_bounds();
        mHasPart[SequencedPresenceProperties::LOC_MESH_PART] = rhs.has_mesh();
        mHasPart[SequencedPresenceProperties::LOC_PHYSICS_PART] = rhs.has_physics();

        if (rhs.has_parent()) mUpdate.setParent(rhs.parent(), rhs.parent_seqno());
        if (rhs.has_location()) mUpdate.setLocation(rhs.location(), rhs.location_seqno());
        if (rhs.has_orientation()) mUpdate.setOrientation(rhs.orientation(), rhs.orientation_seqno());
        if (rhs.has_bounds()) mUpdate.setBounds(rhs.bounds(), rhs.bounds_seqno());
        if (rhs.has_mesh()) mUpdate.setMesh(Transfer::URI(rhs.mesh()), rhs.mesh_seqno());
        if (rhs.has_physics()) mUpdate.setPhysics(rhs.physics(), rhs.physics_seqno());

        mIndexIDs.reserve(rhs.index_id_size());
        for(uint32 i = 0; i < rhs.index_id_size(); i++)
            mIndexIDs.push_back(rhs.index_id(i));
        mIndexIDsSeqno = rhs.index_id_seqno();
    }
    // Internally, this looks a lot like PresencePropertiesLocUpdate since we
    // use SequencedPresenceProperties to track the values. The difference is
    // that we use a complete copy rather than a reference (same for the mObject
    // value).
    ObjectReference mObject;
    // We also need to track which parts we actually have
    bool mHasPart[SequencedPresenceProperties::LOC_NUM_PART];
    SequencedPresenceProperties mUpdate;
    // For completeness, also track epoch values
    bool mHasEpoch;
    uint64 mEpoch;
    // And IndexIDs, which aren't tracked by
    // SequencedPresenceProperties
    std::vector<ProxIndexID> mIndexIDs;
    uint64 mIndexIDsSeqno;
};

} // namespace Sirikata

#endif //_SIRIKATA_COPYABLE_LOC_UPDATE_HPP_
