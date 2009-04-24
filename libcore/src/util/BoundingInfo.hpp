/*  Meru
 *  BoundingInfo.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _BOUNDING_INFO_HPP_
#define _BOUNDING_INFO_HPP_

#include "Vector3.hpp"
#include "BoundingBox.hpp"
#include "BoundingSphere.hpp"

namespace Sirikata {

/** BoundingInfo represents a 3D axis aligned bounding box and a radius around it.  Used for bounding a centered mesh. Use BoundingBox for spacial information */
class BoundingInfo {
public:
    /** Construct a degenerate bounding box. */
    BoundingInfo();
    /** Construct a bounding box with the given minimum and maximum
     *  points.
     *  \param bbmin minimum point of bounding box
     *  \param bbmax maximum point of bounding box
     *  \param radius is the radial size of the boudning box (not necessarily diagonal size
     */
    BoundingInfo(const Vector3f& bbmin, const Vector3f& bbmax, float32 radius);

    /** Construct a bounding box with the given minimum and maximum
     *  points.
     *  \param bbmin minimum point of bounding box
     *  \param bbmax maximum point of bounding box
     * note the radius is calculated from the impoverished information
     */
    BoundingInfo(const Vector3f& bbmin, const Vector3f& bbmax);
    /** Construct a bounding box centered at the origin which encompasses
     *  a sphere with the given radius.
     */
    explicit BoundingInfo(float32 radius);
    /** Construct a bounding box which encompasses the given
     *  bounding sphere.
     */
    explicit BoundingInfo(const BoundingSphere<float32>& bs);
    BoundingInfo scale(const Vector3f &scale)const;
    /** Returns the minimum point of the bounding box. */
    const Vector3f& min() const{return mMin;}
    /** Returns the maximum point of the bounding box. */
    const Vector3f& max() const{return mMax;}
    /** Returns the diagonal vector of the bounding box, i.e. max - min. */
    Vector3f diag() const;
    /** Returns the center of the bounding box. */
    Vector3f center() const;
    BoundingBox<float32> boundingBox()const;
    /** Returns the union of this bounding box with the given bounding box. */
    BoundingInfo merge(const BoundingBox<float32>& bbox) const;
    /** Returns the union of this bounding box with the given bounding box. */
    BoundingInfo merge(const BoundingInfo& bbox) const;
    /** Returns the union of this bounding box with the given point. */
    BoundingInfo merge(const Vector3f& point) const;
    float32 radius()const { return mRadius; }
private:
    Vector3f mMin;
    float32 mRadius;
    Vector3f mMax;
}; // class Bounding Box

} // namespace Meru

#endif //_BOUNDING_BOX_HPP_
