// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPROXYOBJECT_PRESENCE_PROPERTIES_HPP_
#define _SIRIKATA_LIBPROXYOBJECT_PRESENCE_PROPERTIES_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/transfer/URI.hpp>

namespace Sirikata {

/** Read-only interface for PresenceProperties. Note that you need to be
 *  careful using this interface in threaded code since you can't implement a
 *  thread-safe version of this interface just by locking each accessor -- you
 *  need to lock around the calls since they return const-references. This
 *  approach currently makes sense because it allows us to avoid making copies
 *  where they aren't necessary and frequently we need locks for longer anyway,
 *  e.g. to ensure that operations on a ProxyObject are performed atomically.
 */
class IPresencePropertiesRead {
public:
    virtual ~IPresencePropertiesRead() {}

    virtual const TimedMotionVector3f& location() const = 0;
    virtual const TimedMotionQuaternion& orientation() const = 0;
    virtual const BoundingSphere3f& bounds() const = 0;
    virtual const Transfer::URI& mesh() const = 0;
    virtual const String& physics() const = 0;
};

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
       mPhysics()
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

    virtual const TimedMotionVector3f& location() const { return mLoc; }
    virtual bool setLocation(const TimedMotionVector3f& l) { mLoc = l; return true; }

    virtual const TimedMotionQuaternion& orientation() const { return mOrientation; }
    virtual bool setOrientation(const TimedMotionQuaternion& o) { mOrientation = o; return true; }

    virtual const BoundingSphere3f& bounds() const { return mBounds; }
    virtual bool setBounds(const BoundingSphere3f& b) { mBounds = b; return true; }

    virtual const Transfer::URI& mesh() const { return mMesh; }
    virtual bool setMesh(const Transfer::URI& m) { mMesh = m; return true; }

    virtual const String& physics() const { return mPhysics; }
    virtual bool setPhysics(const String& p) { mPhysics = p; return true; }

protected:
    TimedMotionVector3f mLoc;
    TimedMotionQuaternion mOrientation;
    BoundingSphere3f mBounds;
    Transfer::URI mMesh;
    String mPhysics;
};

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
        LOC_NUM_PART = 5
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

    bool setLocation(const TimedMotionVector3f& reqloc, uint64 seqno, bool predictive=false) {
        if (seqno < mUpdateSeqno[LOC_POS_PART] && !predictive)
            return false;

        if (!predictive) mUpdateSeqno[LOC_POS_PART] = seqno;

        // FIXME at some point we need to resolve these, but this might
        // require additional information from the space server, e.g. if
        // requests were actually accepted. This all gets very tricky
        // unless we track multiple outstanding update requests and can
        // figure out which one failed, even if the space server generates
        // other update while handling eht requests...
        // We used to check the update times here but that really doesn't work
        // given synchronization. Instead we just accept the values now, but we
        // really should figure out whether the update is up to date with our
        // prediction, i.e. the last request...
        mLoc = reqloc;
        return true;
    }

    bool setOrientation(const TimedMotionQuaternion& reqorient, uint64 seqno, bool predictive=false) {
        if (seqno < mUpdateSeqno[LOC_ORIENT_PART] && !predictive) return false;

        if (!predictive) mUpdateSeqno[LOC_ORIENT_PART] = seqno;

        // FIXME see relevant comment in setLocation
        mOrientation = reqorient;
        return true;
    }

    bool setBounds(const BoundingSphere3f& b, uint64 seqno, bool predictive=false) {
        if (seqno < mUpdateSeqno[LOC_BOUNDS_PART] && !predictive)
            return false;

        if (!predictive)
            mUpdateSeqno[LOC_BOUNDS_PART] = seqno;
        mBounds = b;
        return true;
    }

    bool setMesh(const Transfer::URI& m, uint64 seqno, bool predictive=false) {
        if (seqno < mUpdateSeqno[LOC_MESH_PART] && !predictive)
            return false;

        if (!predictive)
            mUpdateSeqno[LOC_MESH_PART] = seqno;
        mMesh = m;
        return true;
    }

    bool setPhysics(const String& p, uint64 seqno, bool predictive=false) {
        if (seqno < mUpdateSeqno[LOC_PHYSICS_PART] && !predictive)
            return false;

        if (!predictive)
            mUpdateSeqno[LOC_PHYSICS_PART] = seqno;
        mPhysics = p;
        return true;
    }

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

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROXYOBJECT_PRESENCE_PROPERTIES_HPP_
