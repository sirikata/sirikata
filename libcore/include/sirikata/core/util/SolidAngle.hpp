/*  Sirikata Utilities -- Math Library
 *  SolidAngle.hpp
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
 *  * Neither the name of libprox nor the names of its contributors may
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

#ifndef _SIRIKATA_SOLID_ANGLE_HPP_
#define _SIRIKATA_SOLID_ANGLE_HPP_

namespace Sirikata {

class SIRIKATA_EXPORT SolidAngle {
public:
    static const float Pi;
    static const SolidAngle Min;
    static const SolidAngle Max;

    SolidAngle();
    explicit SolidAngle(float sa);
    SolidAngle(const SolidAngle& cpy);
    ~SolidAngle();

    /// Get the solid angle represented by the circular area with the given vector to its center and radius
    static SolidAngle fromCenterRadius(const Vector3<float>& to_center, float radius);

    SolidAngle operator+(const SolidAngle& rhs) const;
    SolidAngle& operator+=(const SolidAngle& rhs);
    SolidAngle operator-(const SolidAngle& rhs) const;
    SolidAngle& operator-=(const SolidAngle& rhs);

    SolidAngle operator*(float rhs) const;
    SolidAngle& operator*=(float rhs);

    SolidAngle operator/(float rhs) const;
    SolidAngle& operator/=(float rhs);

    bool operator<(const SolidAngle& rhs) const;
    bool operator==(const SolidAngle& rhs) const;
    bool operator<=(const SolidAngle& rhs) const;
    bool operator>(const SolidAngle& rhs) const;
    bool operator>=(const SolidAngle& rhs) const;
    bool operator!=(const SolidAngle& rhs) const;

    float asFloat() const;

    /// Get the maximum distance from an object of the given radius that could
    /// result in this solid angle.  Effectively the inverse of fromCenterRadius.
    float maxDistance(float obj_radius) const;
protected:
    static const float MinVal;
    static const float MaxVal;

    void clamp();

    float mSolidAngle;
}; // class SolidAngle

SIRIKATA_FUNCTION_EXPORT std::ostream & operator<<(std::ostream & os, const Sirikata::SolidAngle& sa);

} // namespace Sirikata

#endif //_SIRIKATA_SOLID_ANGLE_HPP_
