/*  cbr
 *  BoundingBox.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_BOUNDING_BOX_HPP_
#define _CBR_BOUNDING_BOX_HPP_

#include "Utility.hpp"
#include <cmath>

// FIXME these should really come from float.h or the like
#define BBOX_MIN -1e30f
#define BBOX_MAX 1e30f

namespace CBR {

template<typename CoordType>
class BoundingBox {
public:
    typedef typename CoordType::real real;
    static const uint8 size = CoordType::size;

    BoundingBox()
     : mMin(BBOX_MAX),
       mMax(BBOX_MIN)
    {
    }

    BoundingBox(const CoordType& min, const CoordType& max)
     : mMin(min),
       mMax(max)
    {
    }

    const CoordType& min() const {
        return mMin;
    }

    const CoordType& max() const {
        return mMax;
    }

    CoordType center() const {
        return mMin + (mMax - mMin) * .5;
    }

    CoordType extents() const {
        return (mMax - mMin);
    }

    BoundingBox& mergeIn(const BoundingBox& rhs) {
        mMin = mMin.min(rhs.mMin);
        mMax = mMax.max(rhs.mMax);
        return *this;
    }

    BoundingBox merge(const BoundingBox& rhs) const {
        return BoundingBox(mMin.min(rhs.mMin), mMax.max(rhs.mMax));
    }

    BoundingBox intersection(const BoundingBox& rhs) const {
        return BoundingBox(mMin.max(rhs.mMin), mMax.min(rhs.mMin));
    }

    bool degenerate() const {
        for(int i = 0; i < size; i++)
            if (mMin[i] > mMax[i]) return true;
        return false;
    }

    bool contains(const CoordType& point) const {
        for(int i = 0; i < size; i++)
            if (mMin[i] > point[i] || mMax[i] < point[i]) return false;
        return true;
    }

    real volume() const {
        if (degenerate()) return 0.0;
        CoordType diag = mMax - mMin;
        real vol = 1;
        for(int i = 0; i < size; i++)
            vol *= diag[i];
        return vol;
    }

    CoordType clamp(const CoordType& v) const {
        return v.max(min()).min(max());
    }

    bool operator==(const BoundingBox& rhs) {
        return (mMin == rhs.mMin && mMax == rhs.mMax);
    }
    bool operator!=(const BoundingBox& rhs) {
        return (mMin != rhs.mMin || mMax != rhs.mMax);
    }

private:
    CoordType mMin;
    CoordType mMax;
}; // class BoundingBox

template<typename CoordType> inline std::ostream& operator <<(std::ostream& os, const BoundingBox<CoordType> &rhs) {
    os << '<' << rhs.min() << ',' << rhs.max() << '>';
    return os;
}
template<typename CoordType> inline std::istream& operator >>(std::istream& is, BoundingBox<CoordType> &rhs) {
    // FIXME this should be more robust.  It currently relies on the exact format provided by operator <<
    char dummy;
    CoordType minval, maxval;
    is >> dummy >> minval >> dummy >> maxval >> dummy;
    rhs = BoundingBox<CoordType>(minval, maxval);
    return is;
}

typedef BoundingBox<Vector3f> BoundingBox3f;
typedef BoundingBox<Vector3d> BoundingBox3d;

} // namespace CBR

#endif //_CBR_BOUNDING_BOX_HPP_
