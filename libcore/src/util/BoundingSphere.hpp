/*  Sirikata Utilities -- Math Library
 *  BoundingSphere.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _SIRIKATA_BOUNDING_SPHERE_HPP
#define _SIRIKATA_BOUNDING_SPHERE_HPP
namespace Sirikata {
template <typename real> class BoundingSphere {
    Vector3<real> mCenter;
    float32 mRadius;
public:
    static const real Pi = 3.1415926536;

    static BoundingSphere<real> null() {
        return BoundingSphere<real>(Vector3<real>(0,0,0),0);
    }
    BoundingSphere() {}
    BoundingSphere(const Vector3<real>&center, float radius){
        mCenter=center;
        mRadius=radius;
    }
    float32 radius()const{
        return mRadius;
    }
    Vector3<real> center() const {
        return mCenter;
    }
    BoundingSphere<real> recenter(const Vector3<real>&newCenter) {
        mRadius+=(newCenter-mCenter).length();
        mCenter=newCenter;
    }
    BoundingSphere& mergeIn(const BoundingSphere& rhs) {
        *this = merge(rhs);
        return *this;
    }

    BoundingSphere merge(const BoundingSphere& rhs) const {
        if (rhs.degenerate())
            return *this;

        if (this->degenerate())
            return rhs;

        real center_dist = (rhs.mCenter - mCenter).length();
        if (center_dist + mRadius <= rhs.mRadius)
            return rhs;
        if (center_dist + rhs.mRadius <= mRadius)
            return *this;

        real new_radius = (mRadius + center_dist + rhs.mRadius) * 0.5;
        real ratio = (new_radius - mRadius) / center_dist;
        Vector3<real> new_center = mCenter + (rhs.mCenter - mCenter) * ratio;
        return BoundingSphere(new_center, new_radius);
    }

    bool contains(const BoundingSphere& other) const {
        real centers_len = (mCenter - other.mCenter).length();
        return (mRadius >= centers_len + other.mRadius);
    }

    bool contains(const BoundingSphere& other, real epsilon) const {
        real centers_len = (mCenter - other.mCenter).length();
        return (mRadius + epsilon >= centers_len + other.mRadius);
    }

    bool contains(const Vector3<real>& pt) const {
        return ( (mCenter-pt).lengthSquared() <= mRadius*mRadius );
    }

    bool degenerate() const {
        return ( mRadius <= 0 );
    }

    real volume() const {
        if (degenerate()) return 0.0;
        return 4.0 / 3.0 * Pi * mRadius * mRadius * mRadius;
    }

    bool operator==(const BoundingSphere& rhs) {
        return (mCenter == rhs.mCenter && mRadius == rhs.mRadius);
    }
    bool operator!=(const BoundingSphere& rhs) {
        return (mCenter != rhs.mCenter || mRadius != rhs.mRadius);
    }
};
}
#endif
