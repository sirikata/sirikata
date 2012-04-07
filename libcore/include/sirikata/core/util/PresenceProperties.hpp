// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_PRESENCE_PROPERTIES_HPP_
#define _SIRIKATA_LIBCORE_PRESENCE_PROPERTIES_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/ObjectReference.hpp>

namespace Sirikata {

/** Read-only interface for PresenceProperties.  Copies internal data on calls
 * instead of passing references to allow multiple threads to use without huge
 * amounts of coordination.
 */
class IPresencePropertiesRead {
public:
    virtual ~IPresencePropertiesRead() {}

    virtual TimedMotionVector3f location() const = 0;
    virtual TimedMotionQuaternion orientation() const = 0;
    virtual BoundingSphere3f bounds() const = 0;
    virtual Transfer::URI mesh() const = 0;
    virtual String physics() const = 0;
    virtual bool isAggregate() const = 0;
    virtual ObjectReference parent() const = 0;
};
typedef std::tr1::shared_ptr<IPresencePropertiesRead> IPresencePropertiesReadPtr;

/** Stores the basic properties provided for objects, i.e. location,
 *  orientation, mesh, etc. This is intentionally bare-bones: it can be used in
 *  a variety of places to minimally track the properties for an object.
 */
class PresenceProperties : public virtual IPresencePropertiesRead {
public:
    PresenceProperties()
     : mLoc(Time::null(), MotionVector3f(Vector3f::zero(), Vector3f::zero())),
       mOrientation(Time::null(), MotionQuaternion(Quaternion::identity(), Quaternion::identity())),
       mBounds(Vector3f::zero(), 0),
       mMesh(),
       mPhysics(),
       mIsAggregate(false),
       mParent(ObjectReference::null())
    {}
    PresenceProperties(
        const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient,
        const BoundingSphere3f& bnds, const Transfer::URI& msh, const String& phy
    )
     : mLoc(loc),
       mOrientation(orient),
       mBounds(bnds),
       mMesh(msh),
       mPhysics(phy)
    {}
    virtual ~PresenceProperties() {}

    virtual TimedMotionVector3f location() const { return mLoc; }
    virtual bool setLocation(const TimedMotionVector3f& l) { mLoc = l; return true; }

    virtual TimedMotionQuaternion orientation() const { return mOrientation; }
    virtual bool setOrientation(const TimedMotionQuaternion& o) { mOrientation = o; return true; }

    virtual BoundingSphere3f bounds() const { return mBounds; }
    virtual bool setBounds(const BoundingSphere3f& b) { mBounds = b; return true; }

    virtual Transfer::URI mesh() const { return mMesh; }
    virtual bool setMesh(const Transfer::URI& m) { mMesh = m; return true; }

    virtual String physics() const { return mPhysics; }
    virtual bool setPhysics(const String& p) { mPhysics = p; return true; }

    virtual bool setIsAggregate(bool isAgg) { mIsAggregate = isAgg; return true; }
    virtual bool isAggregate() const { return mIsAggregate; }

    virtual bool setParent(const ObjectReference& par) { mParent = par; return true; }
    virtual ObjectReference parent() const { return mParent; }
protected:
    TimedMotionVector3f mLoc;
    TimedMotionQuaternion mOrientation;
    BoundingSphere3f mBounds;
    Transfer::URI mMesh;
    String mPhysics;
    bool mIsAggregate;
    ObjectReference mParent;
};
typedef std::tr1::shared_ptr<PresenceProperties> PresencePropertiesPtr;

/** Stores the basic properties for objects, i.e. location, orientation, mesh,
 *  etc., as well sequence numbers for each of those properties. Useful in
 *  determining the most up-to-date information for an object.
 */
class SequencedPresenceProperties : public PresenceProperties {
public:
    enum LOC_PARTS {
        LOC_POS_PART = 0,
        LOC_ORIENT_PART = 1,
        LOC_BOUNDS_PART = 2,
        LOC_MESH_PART = 3,
        LOC_PHYSICS_PART = 4,
        LOC_IS_AGG_PART = 5,
        LOC_PARENT_PART = 6,
        LOC_NUM_PART = 7
    };

    SequencedPresenceProperties()
    {
        reset();
    }
    virtual ~SequencedPresenceProperties() {}

    uint64 getUpdateSeqNo(LOC_PARTS whichPart) const {
        if (whichPart >= LOC_NUM_PART)
        {
            SILOG(proxyobject, error, "Error in getUpdateSeqNo of proxy.  Requesting an update sequence number for a field that does not exist.  Returning 0");
            return 0;
        }
        return mUpdateSeqno[whichPart];
    }

    bool setLocation(const TimedMotionVector3f& reqloc, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_POS_PART])
            return false;

        mUpdateSeqno[LOC_POS_PART] = seqno;
        mLoc = reqloc;
        return true;
    }
    bool setLocation(const TimedMotionVector3f& l) { return PresenceProperties::setLocation(l); }

    bool setOrientation(const TimedMotionQuaternion& reqorient, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_ORIENT_PART]) return false;

        mUpdateSeqno[LOC_ORIENT_PART] = seqno;
        mOrientation = reqorient;
        return true;
    }
    bool setOrientation(const TimedMotionQuaternion& o) { return PresenceProperties::setOrientation(o); }

    bool setBounds(const BoundingSphere3f& b, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_BOUNDS_PART])
            return false;

        mUpdateSeqno[LOC_BOUNDS_PART] = seqno;
        mBounds = b;
        return true;
    }
    bool setBounds(const BoundingSphere3f& b) { return PresenceProperties::setBounds(b); }

    bool setMesh(const Transfer::URI& m, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_MESH_PART])
            return false;

        mUpdateSeqno[LOC_MESH_PART] = seqno;
        mMesh = m;
        return true;
    }
    bool setMesh(const Transfer::URI& m) { return PresenceProperties::setMesh(m); }

    bool setPhysics(const String& p, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_PHYSICS_PART])
            return false;

        mUpdateSeqno[LOC_PHYSICS_PART] = seqno;
        mPhysics = p;
        return true;
    }
    bool setPhysics(const String& p) { return PresenceProperties::setPhysics(p); }

    bool setIsAggregate(bool isAgg, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_IS_AGG_PART])
            return false;

        mUpdateSeqno[LOC_IS_AGG_PART] = seqno;
        mIsAggregate = isAgg;
        return true;
    }
    bool setIsAggregate(bool isAgg) { return PresenceProperties::setIsAggregate(isAgg); }


    bool setParent(const ObjectReference& parent, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_PARENT_PART])
            return false;

        mUpdateSeqno[LOC_PARENT_PART] = seqno;
        mParent = parent;
        return true;
    }
    bool setParent(const ObjectReference& parent) { return PresenceProperties::setParent(parent); }


    void reset() {
        memset(mUpdateSeqno, 0, LOC_NUM_PART * sizeof(uint64));
    }

    uint64 maxSeqNo() const {
        uint64 maxseqno = mUpdateSeqno[0];
        for(uint32 i = 1; i < LOC_NUM_PART; i++)
            maxseqno = std::max(maxseqno, mUpdateSeqno[i]);
        return maxseqno;
    }
private:
    uint64 mUpdateSeqno[LOC_NUM_PART];
};
typedef std::tr1::shared_ptr<SequencedPresenceProperties> SequencedPresencePropertiesPtr;

} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_PRESENCE_PROPERTIES_HPP_
