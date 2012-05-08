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
#include "pbj.hpp"
#include <google/protobuf/unknown_field_set.h>

namespace Sirikata {

namespace Protocol {

class LocationExtensions {
public:
    enum {
        mesh = 1000,
        physics = 1001,
        aggregate = 1002,
        parent = 1003,
        zernike = 1004
    };
};

}

/** Stores the basic properties provided for objects, i.e. location,
 *  orientation, mesh, etc. This is intentionally bare-bones: it can be used in
 *  a variety of places to minimally track the properties for an object.
 */
class PresenceProperties {
public:
    typedef google::protobuf::UnknownField UnknownField;
    typedef google::protobuf::UnknownFieldSet UnknownFieldSet;

    PresenceProperties()
     : mLoc(Time::null(), MotionVector3f(Vector3f::zero(), Vector3f::zero())),
       mOrientation(Time::null(), MotionQuaternion(Quaternion::identity(), Quaternion::identity())),
       mBounds(Vector3f::zero(), 0)
    {}
    PresenceProperties(
        const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient,
        const BoundingSphere3f& bnds, const Transfer::URI& msh, const String& phy
    )
     : mLoc(loc),
       mOrientation(orient),
       mBounds(bnds)
    {
        if (!msh.empty()) {
            setMesh(msh);
        }
        if (!phy.empty()) {
            setPhysics(phy);
        }
    }

    PresenceProperties(const PresenceProperties& oth)
        : mLoc(oth.mLoc),
         mOrientation(oth.mOrientation),
         mBounds(oth.mBounds),
         mUnknownFieldIndices(oth.mUnknownFieldIndices)
    {
        mUnknownFieldSet.MergeFrom(oth.mUnknownFieldSet);
    }

    const TimedMotionVector3f& location() const { return mLoc; }
    bool setLocation(const TimedMotionVector3f& l) { mLoc = l; return true; }

    const TimedMotionQuaternion& orientation() const { return mOrientation; }
    bool setOrientation(const TimedMotionQuaternion& o) { mOrientation = o; return true; }

    const BoundingSphere3f& bounds() const { return mBounds; }
    bool setBounds(const BoundingSphere3f& b) { mBounds = b; return true; }

    bool setExtensionVarint(int id, int64 value) {
        UnknownFieldIndexMap::const_iterator iter = mUnknownFieldIndices.upper_bound(id);
        if (iter != mUnknownFieldIndices.begin()) {
            --iter;
            if (iter->first == id) {
                UnknownField* field = mUnknownFieldSet.mutable_field(iter->second);
                field->set_varint(value);
                return true;
            }
        }
        // We get the field_count() before adding to the UnknownFieldSet.
        mUnknownFieldIndices.insert(UnknownFieldIndexMap::value_type
                                    (id, mUnknownFieldSet.field_count()));
        mUnknownFieldSet.AddVarint(id, value);
        return true;
    }

    bool setExtensionLengthDelim(int id, const String& value) {
        UnknownFieldIndexMap::const_iterator iter = mUnknownFieldIndices.upper_bound(id);
        if (iter != mUnknownFieldIndices.begin()) {
            --iter;
            if (iter->first == id) {
                UnknownField* field = mUnknownFieldSet.mutable_field(iter->second);
                field->set_length_delimited(value);
                return true;
            }
        }
        // We get the field_count() before adding to the UnknownFieldSet.
        mUnknownFieldIndices.insert(UnknownFieldIndexMap::value_type
                                    (id, mUnknownFieldSet.field_count()));
        mUnknownFieldSet.AddLengthDelimited(id, value);
        return true;
    }

    const UnknownField * getExtension(int id) const {
        UnknownFieldIndexMap::const_iterator iter = mUnknownFieldIndices.upper_bound(id);
        if (iter != mUnknownFieldIndices.begin()) {
            --iter;
            if (iter->first == id) {
                return &mUnknownFieldSet.field(iter->second);
            }
        }
        return NULL;
    }

    UnknownField * getExtension(int id) {
        UnknownFieldIndexMap::iterator iter = mUnknownFieldIndices.upper_bound(id);
        if (iter != mUnknownFieldIndices.begin()) {
            --iter;
            if (iter->first == id) {
                return mUnknownFieldSet.mutable_field(iter->second);
            }
        }
        return NULL;
    }

    Transfer::URI mesh() const {
        const UnknownField* field = getExtension(Protocol::LocationExtensions::mesh);
        if (field && field->type() == UnknownField::TYPE_LENGTH_DELIMITED) {
            return Transfer::URI(field->length_delimited());
        }
        return Transfer::URI();
    }
    bool setMesh(const Transfer::URI& m) {
        setExtensionLengthDelim(Protocol::LocationExtensions::mesh, m.toString());
        return true;
    }

    const String& physics() const {
        const UnknownField* field = getExtension(Protocol::LocationExtensions::physics);
        if (field && field->type() == UnknownField::TYPE_LENGTH_DELIMITED) {
            return field->length_delimited();
        }
        {
            static String def;
            return def;
        }
    }
    bool setPhysics(const String& phys) {
        setExtensionLengthDelim(Protocol::LocationExtensions::physics, phys);
        return true;
    }

    bool setIsAggregate(bool isAgg) {
        setExtensionVarint(Protocol::LocationExtensions::aggregate, isAgg ? 1 : 0);
        return true;
    }
    bool isAggregate() const {
        const UnknownField* field = getExtension(Protocol::LocationExtensions::aggregate);
        if (field && field->type() == UnknownField::TYPE_VARINT) {
            return !!field->varint();
        }
        return false;
    }

    bool setParent(const ObjectReference& par) {
        UUID::Data bytes = par.toRawBytes();
        setExtensionLengthDelim(Protocol::LocationExtensions::parent,
                String(bytes.begin(), bytes.end()));
        return true;
    }
    ObjectReference parent() const {
        const UnknownField* field = getExtension(Protocol::LocationExtensions::parent);
        if (field && field->type() == UnknownField::TYPE_LENGTH_DELIMITED) {
            const String& s = field->length_delimited();
            UUID::Data data;
            size_t i;
            for (i = 0; i < s.length() && i < UUID::static_size; ++i) {
                data[i] = s[i];
            }
            for (; i < UUID::static_size; ++i) {
                data[i] = 0;
            }
            return ObjectReference(UUID(data));
        }
        return ObjectReference::null();
    }

protected:
    TimedMotionVector3f mLoc;
    TimedMotionQuaternion mOrientation;
    BoundingSphere3f mBounds;

    typedef std::multimap<int, int> UnknownFieldIndexMap;
    UnknownFieldIndexMap mUnknownFieldIndices;

    UnknownFieldSet mUnknownFieldSet;
};
typedef std::tr1::shared_ptr<PresenceProperties> PresencePropertiesPtr;

/** Stores the basic properties for objects, i.e. location, orientation, mesh,
 *  etc., as well sequence numbers for each of those properties. Useful in
 *  determining the most up-to-date information for an object.
 */
class SequencedPresenceProperties : public PresenceProperties {
public:
    enum LOC_PARTS {
        LOC_POS_PART = 2, // DANGER: Must be same as protocol
        LOC_ORIENT_PART = 3, // DANGER: Must be same as protocol
        LOC_BOUNDS_PART = 4, // DANGER: Must be same as protocol
        LOC_MESH_PART = Protocol::LocationExtensions::mesh,
        LOC_PHYSICS_PART = Protocol::LocationExtensions::physics,
        LOC_IS_AGG_PART = Protocol::LocationExtensions::aggregate,
        LOC_PARENT_PART = Protocol::LocationExtensions::parent
    };

    SequencedPresenceProperties()
    {
        reset();
    }

    uint64 getUpdateSeqNo(LOC_PARTS whichPart) const {
        SeqnoMap::const_iterator iter = mUpdateSeqno.find(whichPart);
        if (iter == mUpdateSeqno.end())
        {
            SILOG(proxyobject, error, "Error in getUpdateSeqNo of proxy.  Requesting an update sequence number for a field that does not exist.  Returning 0");
            return 0;
        }
        return iter->second;
    }

    bool setLocation(const TimedMotionVector3f& reqloc, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_POS_PART])
            return false;

        mUpdateSeqno[LOC_POS_PART] = seqno;
        return PresenceProperties::setLocation(reqloc);
    }
    bool setLocation(const TimedMotionVector3f& l) { return PresenceProperties::setLocation(l); }

    bool setOrientation(const TimedMotionQuaternion& reqorient, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_ORIENT_PART]) return false;

        mUpdateSeqno[LOC_ORIENT_PART] = seqno;
        return PresenceProperties::setOrientation(reqorient);
    }
    bool setOrientation(const TimedMotionQuaternion& o) { return PresenceProperties::setOrientation(o); }

    bool setBounds(const BoundingSphere3f& b, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_BOUNDS_PART])
            return false;

        mUpdateSeqno[LOC_BOUNDS_PART] = seqno;
        return PresenceProperties::setBounds(b);
    }
    bool setBounds(const BoundingSphere3f& b) { return PresenceProperties::setBounds(b); }

    bool setMesh(const Transfer::URI& m, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_MESH_PART])
            return false;

        mUpdateSeqno[LOC_MESH_PART] = seqno;
        return PresenceProperties::setMesh(m);
    }
    bool setMesh(const Transfer::URI& m) { return PresenceProperties::setMesh(m); }

    bool setPhysics(const String& p, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_PHYSICS_PART])
            return false;

        mUpdateSeqno[LOC_PHYSICS_PART] = seqno;
        return PresenceProperties::setPhysics(p);
    }
    bool setPhysics(const String& p) { return PresenceProperties::setPhysics(p); }

    bool setIsAggregate(bool isAgg, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_IS_AGG_PART])
            return false;

        mUpdateSeqno[LOC_IS_AGG_PART] = seqno;
        return PresenceProperties::setIsAggregate(isAgg);
    }
    bool setIsAggregate(bool isAgg) { return PresenceProperties::setIsAggregate(isAgg); }


    bool setParent(const ObjectReference& parent, uint64 seqno) {
        if (seqno < mUpdateSeqno[LOC_PARENT_PART])
            return false;

        mUpdateSeqno[LOC_PARENT_PART] = seqno;
        return PresenceProperties::setParent(parent);
    }
    bool setParent(const ObjectReference& parent) { return PresenceProperties::setParent(parent); }


    void reset() {
        mUpdateSeqno.clear();
    }

    uint64 maxSeqNo() const {
        uint64 maxseqno = 0;
        for(SeqnoMap::const_iterator iter = mUpdateSeqno.begin(); iter != mUpdateSeqno.end(); ++iter)
        {
            maxseqno = std::max(maxseqno, iter->second);
        }
        return maxseqno;
    }
private:
    typedef std::map<int, uint64> SeqnoMap;
    SeqnoMap mUpdateSeqno;
};
typedef std::tr1::shared_ptr<SequencedPresenceProperties> SequencedPresencePropertiesPtr;

} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_PRESENCE_PROPERTIES_HPP_
