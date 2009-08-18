/*  Sirikata - Mono Embedding
 *  MonoConvert.hpp
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
#ifndef _MONO_CONVERT_HPP_
#define _MONO_CONVERT_HPP_


#include <mono/metadata/object.h>

// Conversions for Mono objects to and from C++ versions

namespace Mono {

struct CSharpVector3 {
    double x;
    double y;
    double z;
};

struct CSharpQuaternion {
    double w;
    double x;
    double y;
    double z;
};

struct CSharpLocation {
    CSharpVector3 pos;
    CSharpQuaternion orient;
    CSharpVector3 vel;
    CSharpVector3 angVelocityAxis;
    float angVelocityRadians;
};

struct CSharpColor {
    float r;
    float g;
    float b;
    float a;
};

struct CSharpUUID {
    unsigned char data[16];
};

struct CSharpSpaceID {
    CSharpUUID id;
};

struct CSharpObjectReference {
    CSharpUUID id;
};

struct CSharpSpaceObjectReference {
    CSharpSpaceID space;
    CSharpObjectReference reference;
};

struct CSharpDuration {
    Sirikata::int32 lowerTicks;
    Sirikata::int32 upperTicks;
};


Sirikata::Vector3f Vector3f(CSharpVector3* in);
Sirikata::Vector3d Vector3d(CSharpVector3* in);
Sirikata::Quaternion Quaternion(CSharpQuaternion* in);
Sirikata::Location Location(CSharpLocation* in);
Sirikata::Vector4f Color(CSharpColor* in);
Sirikata::BoundingInfo BoundingInfo(const Object& in);
Sirikata::UUID UUID(CSharpUUID* in);
Sirikata::SpaceID SpaceID(CSharpSpaceID* in);
Sirikata::ObjectReference ObjectReference(CSharpObjectReference* in);
Sirikata::SpaceObjectReference SpaceObjectReference(CSharpSpaceObjectReference* in);
Sirikata::Time RawTime(Sirikata::int64 ticks);
Sirikata::Duration Duration(CSharpDuration* in);
Sirikata::Time Time(CSharpDuration* in);

void ConvertVector3(const Sirikata::Vector3f& in, CSharpVector3* out);
void ConvertVector3(const Sirikata::Vector3d& in, CSharpVector3* out);
void ConvertQuaternion(const Sirikata::Quaternion& in, CSharpQuaternion* out);
void ConvertLocation(const Sirikata::Location& in, CSharpLocation* out);
void ConvertColor(const Sirikata::Vector4f& in, CSharpColor* out);
void ConvertUUID(const Sirikata::UUID& in, CSharpUUID* out);
void ConvertSpaceObjectReference(const Sirikata::SpaceObjectReference& in, CSharpSpaceObjectReference* out);
CSharpDuration ConvertTime(const Sirikata::Time& in);
void ConvertDuration(const Sirikata::Duration& in, CSharpDuration* out);
void ConvertTime(const Sirikata::Duration& in, CSharpDuration* out);

} // namespace Mono

#endif //_MONO_CONVERT_HPP_
