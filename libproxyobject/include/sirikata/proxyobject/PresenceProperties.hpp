// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPROXYOBJECT_PRESENCE_PROPERTIES_HPP_
#define _SIRIKATA_LIBPROXYOBJECT_PRESENCE_PROPERTIES_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/transfer/URI.hpp>

namespace Sirikata {

/** Stores the basic properties provided for objects, i.e. location,
 *  orientation, mesh, etc. This is intentionally bare-bones: it can be used in
 *  a variety of places to minimally track the properties for an object.
 */
class PresenceProperties {
public:

    PresenceProperties()
     : mLoc(Time::null(), MotionVector3f(Vector3f::zero(), Vector3f::zero())),
       mOrientation(Time::null(), MotionQuaternion(Quaternion::identity(), Quaternion::identity())),
       mBounds(Vector3f::zero(), 0),
       mMesh(),
       mPhysics()
    {}
    const TimedMotionVector3f& location() const { return mLoc;}
    bool setLocation(const TimedMotionVector3f& l) { mLoc = l; return true; }

    const TimedMotionQuaternion& orientation() const { return mOrientation;}
    bool setOrientation(const TimedMotionQuaternion& o) { mOrientation = o; return true; }

    const BoundingSphere3f& bounds() const { return mBounds;}
    bool setBounds(const BoundingSphere3f& b) { mBounds = b; return true; }

    const Transfer::URI& mesh() const { return mMesh;}
    bool setMesh(const Transfer::URI& m) { mMesh = m; return true; }

    const String& physics() const { return mPhysics;}
    bool setPhysics(const String& p) { mPhysics = p; return true; }

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
        if (predictive || reqloc.updateTime() >= mLoc.updateTime()) {
            mLoc = reqloc;
            return true;
        }

        return false;
    }

    bool setOrientation(const TimedMotionQuaternion& reqorient, uint64 seqno, bool predictive=false) {
        if (seqno < mUpdateSeqno[LOC_ORIENT_PART] && !predictive) return false;

        if (!predictive) mUpdateSeqno[LOC_ORIENT_PART] = seqno;

        // FIXME see relevant comment in setLocation
        if (predictive || reqorient.updateTime() >= mOrientation.updateTime()) {
            mOrientation = reqorient;
            return true;
        }
        return false;
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
