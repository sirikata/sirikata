/*  Sirikata Utilities -- Math Library
 *  SolidAngle.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/SolidAngle.hpp>

namespace Sirikata {

const float SolidAngle::Pi = 3.1415926536f;
const float SolidAngle::MinVal = 0.0f;
const float SolidAngle::MaxVal = 4.0f*SolidAngle::Pi;
const SolidAngle SolidAngle::Min = SolidAngle(SolidAngle::MinVal);
const SolidAngle SolidAngle::Max = SolidAngle(SolidAngle::MaxVal);

SolidAngle::SolidAngle()
 : mSolidAngle(MinVal)
{
}

SolidAngle::SolidAngle(float sa)
 : mSolidAngle(sa)
{
    clamp();
}

SolidAngle::SolidAngle(const SolidAngle& rhs)
 : mSolidAngle(rhs.mSolidAngle)
{
}

SolidAngle::~SolidAngle() {
}

float SolidAngle::asFloat() const {
    return mSolidAngle;
}

SolidAngle SolidAngle::fromCenterRadius(const Vector3<float>& to_center, float radius) {
    float to_center_len = to_center.length();
    if (to_center_len <= radius)
        return SolidAngle(MaxVal);

    float sin_alpha = radius / to_center_len;
    float cos_alpha = sqrtf(1 - sin_alpha*sin_alpha);
    return SolidAngle( 2.0f * Pi * (1.0f - cos_alpha) );
}

float SolidAngle::maxDistance(float obj_radius) const {
    float C = 1.f - mSolidAngle / (2.0f * SolidAngle::Pi);
    C = 1.f - C*C;
    return obj_radius / C;
}

SolidAngle SolidAngle::operator+(const SolidAngle& rhs) const {
    return SolidAngle( mSolidAngle + rhs.mSolidAngle );
}

SolidAngle& SolidAngle::operator+=(const SolidAngle& rhs) {
    mSolidAngle += rhs.mSolidAngle;
    clamp();
    return *this;
}

SolidAngle SolidAngle::operator-(const SolidAngle& rhs) const {
    return SolidAngle( mSolidAngle - rhs.mSolidAngle );
}

SolidAngle& SolidAngle::operator-=(const SolidAngle& rhs) {
    mSolidAngle -= rhs.mSolidAngle;
    clamp();
    return *this;
}

SolidAngle SolidAngle::operator*(float rhs) const {
    assert(rhs >= 0.f);
    return SolidAngle( mSolidAngle * rhs );
}

SolidAngle& SolidAngle::operator*=(float rhs) {
    assert(rhs >= 0.f);
    mSolidAngle *= rhs;
    clamp();
    return *this;
}

SolidAngle SolidAngle::operator/(float rhs) const {
    assert(rhs > 0.f);
    return SolidAngle( mSolidAngle / rhs );
}

SolidAngle& SolidAngle::operator/=(float rhs) {
    assert(rhs > 0.f);
    mSolidAngle /= rhs;
    clamp();
    return *this;
}

bool SolidAngle::operator<(const SolidAngle& rhs) const {
    return (mSolidAngle < rhs.mSolidAngle);
}

bool SolidAngle::operator==(const SolidAngle& rhs) const {
    return (mSolidAngle == rhs.mSolidAngle);
}

bool SolidAngle::operator<=(const SolidAngle& rhs) const {
    return (mSolidAngle <= rhs.mSolidAngle);
}

bool SolidAngle::operator>(const SolidAngle& rhs) const {
    return (mSolidAngle > rhs.mSolidAngle);
}

bool SolidAngle::operator>=(const SolidAngle& rhs) const {
    return (mSolidAngle >= rhs.mSolidAngle);
}

bool SolidAngle::operator!=(const SolidAngle& rhs) const {
    return (mSolidAngle != rhs.mSolidAngle);
}

void SolidAngle::clamp() {
    if (mSolidAngle < MinVal) mSolidAngle = MinVal;
    if (mSolidAngle > MaxVal) mSolidAngle = MaxVal;
}

std::ostream& operator<< (std::ostream &os, const Sirikata::SolidAngle& output) {
    os << output.asFloat() << "sr";
    return os;
}

} // namespace Sirikata
