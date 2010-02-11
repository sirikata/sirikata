/*  Sirikata - Mono Embedding
 *  MonoConvert.cpp
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
#include "oh/Platform.hpp"

#include "util/UUID.hpp"
#include "util/BoundingInfo.hpp"
#include "MonoObject.hpp"
#include "MonoConvert.hpp"
#include "MonoObject.hpp"
#include "util/SpaceID.hpp"
#include "util/ObjectReference.hpp"
#include "util/SpaceObjectReference.hpp"
#include "util/Time.hpp"


namespace Mono {


void ConvertVector3(const Sirikata::Vector3f& in, CSharpVector3* out) {
    out->x = in.x;
    out->y = in.y;
    out->z = in.z;
}
void ConvertVector3(const Sirikata::Vector3d& in, CSharpVector3* out) {
    out->x = in.x;
    out->y = in.y;
    out->z = in.z;
}

SIRIKATA_PLUGIN_EXPORT_C
void ConvertVector3CPPToCSharpAndFree(void* jarg, void* jout) {
    Sirikata::Vector3d* arg = (Sirikata::Vector3d*)jarg;
    CSharpVector3* out = (CSharpVector3*)jout;
    ConvertVector3(*arg, out);
    delete arg;
}

Sirikata::Vector3f Vector3f(CSharpVector3* in) {
    return Sirikata::Vector3f(in->x, in->y, in->z);//warns cus we should have Vector3f and Vector3d in C# land
}

Sirikata::Vector3d Vector3d(CSharpVector3* in) {
    return Sirikata::Vector3d(in->x, in->y, in->z);
}

SIRIKATA_PLUGIN_EXPORT_C
void* ConvertVector3CSharpToCPP(CSharpVector3* csharp_vec3) {
    return (void*) new Sirikata::Vector3d( Vector3d(csharp_vec3) );
}





void ConvertQuaternion(const Sirikata::Quaternion& in, CSharpQuaternion* out) {
    out->w = in.w;
    out->x = in.x;
    out->y = in.y;
    out->z = in.z;
}

SIRIKATA_PLUGIN_EXPORT_C
void ConvertQuaternionCPPToCSharpAndFree(void* jarg, void* jout) {
    Sirikata::Quaternion* arg = (Sirikata::Quaternion*)jarg;
    CSharpQuaternion* out = (CSharpQuaternion*)jout;
    ConvertQuaternion(*arg, out);
    delete arg;
}

::Sirikata::Quaternion Quaternion(CSharpQuaternion* in) {

    return ::Sirikata::Quaternion(in->w, in->x, in->y, in->z, ::Sirikata::Quaternion::WXYZ());
}

SIRIKATA_PLUGIN_EXPORT_C
void* ConvertQuaternionCSharpToCPP(CSharpQuaternion* csharp_quat) {
    return (void*) new Sirikata::Quaternion( Quaternion(csharp_quat) );
}




void ConvertLocation(const Sirikata::Location& in, CSharpLocation* out) {
    ConvertVector3(in.getPosition(), &out->pos);
    ConvertQuaternion(in.getOrientation(), &out->orient);
    ConvertVector3(in.getVelocity(), &out->vel);
    ConvertVector3(in.getAxisOfRotation(), &out->angVelocityAxis);
    out->angVelocityRadians=in.getAngularSpeed();
}

SIRIKATA_PLUGIN_EXPORT_C
void ConvertLocationCPPToCSharpAndFree(void* jarg, void* jout) {
    Sirikata::Location* arg = (Sirikata::Location*)jarg;
    CSharpLocation* out = (CSharpLocation*)jout;
    ConvertLocation(*arg, out);
    delete arg;
}

Sirikata::Location Location(CSharpLocation* in) {
    float temp=in->angVelocityRadians;
    return Sirikata::Location(
        Vector3d(&in->pos),
        Quaternion(&in->orient),
        Vector3f(&in->vel),
        Vector3f(&in->angVelocityAxis),
        temp
    );
}

SIRIKATA_PLUGIN_EXPORT_C
void* ConvertLocationCSharpToCPP(CSharpLocation* csharp_loc) {
    return (void*) new Sirikata::Location( Location(csharp_loc) );
}




Sirikata::Vector4f Color(CSharpColor* in) {
    return Sirikata::Vector4f(in->r, in->g, in->b, in->a);
}

void ConvertColor(const Sirikata::Vector4f& in, CSharpColor* out) {
    out->r = in.x;
    out->g = in.y;
    out->b = in.z;
    out->a = in.w;
}





Sirikata::BoundingInfo BoundingInfo(const Object& in) {
  float minx=in.getField("mMinx").unboxSingle();
  float miny=in.getField("mMiny").unboxSingle();
  float minz=in.getField("mMinz").unboxSingle();

  float radius=in.getField("mRadius").unboxSingle();

  float maxx=in.getField("mMaxx").unboxSingle();
  float maxy=in.getField("mMaxy").unboxSingle();
  float maxz=in.getField("mMaxz").unboxSingle();
  return Sirikata::BoundingInfo(Sirikata::Vector3f(minx,miny,minz),
                            Sirikata::Vector3f(maxx,maxy,maxz),
                            radius);
}


void ConvertUUID(const Sirikata::UUID& in, CSharpUUID* out) {
    // Internal C# representation is little endian, UUID format is big endian
    out->data[0] = in.getArray()[3];
    out->data[1] = in.getArray()[2];
    out->data[2] = in.getArray()[1];
    out->data[3] = in.getArray()[0];
    out->data[4] = in.getArray()[5];
    out->data[5] = in.getArray()[4];
    out->data[6] = in.getArray()[7];
    out->data[7] = in.getArray()[6];
    out->data[8] = in.getArray()[8];
    out->data[9] = in.getArray()[9];
    out->data[10] = in.getArray()[10];
    out->data[11] = in.getArray()[11];
    out->data[12] = in.getArray()[12];
    out->data[13] = in.getArray()[13];
    out->data[14] = in.getArray()[14];
    out->data[15] = in.getArray()[15];
}

Sirikata::UUID UUID(CSharpUUID* in) {
    // Internal C# representation is little endian, UUID format is big endian
    unsigned char tmp[16];
    tmp[0] = in->data[3];
    tmp[1] = in->data[2];
    tmp[2] = in->data[1];
    tmp[3] = in->data[0];
    tmp[4] = in->data[5];
    tmp[5] = in->data[4];
    tmp[6] = in->data[7];
    tmp[7] = in->data[6];
    tmp[8] = in->data[8];
    tmp[9] = in->data[9];
    tmp[10] = in->data[10];
    tmp[11] = in->data[11];
    tmp[12] = in->data[12];
    tmp[13] = in->data[13];
    tmp[14] = in->data[14];
    tmp[15] = in->data[15];
    return Sirikata::UUID(tmp, 16);
}



void ConvertSpaceID(const Sirikata::SpaceID& in, CSharpSpaceID* out) {
    ConvertUUID(in.getAsUUID(), &out->id);
}

Sirikata::SpaceID SpaceID(CSharpSpaceID* in) {
    return Sirikata::SpaceID( UUID(&in->id) );
}



void ConvertObjectReference(const Sirikata::ObjectReference& in, CSharpObjectReference* out) {
    ConvertUUID(in.getAsUUID(), &out->id);
}

Sirikata::ObjectReference ObjectReference(CSharpObjectReference* in) {
    return Sirikata::ObjectReference( UUID(&in->id) );
}




void ConvertSpaceObjectReference(const Sirikata::SpaceObjectReference& in, CSharpSpaceObjectReference* out) {
    ConvertSpaceID(in.space(), &out->space);
    ConvertObjectReference(in.object(), &out->reference);
}

SIRIKATA_PLUGIN_EXPORT_C
void ConvertSpaceObjectReferenceCPPToCSharpAndFree(void* jarg, void* jout) {
    Sirikata::SpaceObjectReference* arg = (Sirikata::SpaceObjectReference*)jarg;
    CSharpSpaceObjectReference* out = (CSharpSpaceObjectReference*)jout;
    ConvertSpaceObjectReference(*arg, out);
    delete arg;
}

Sirikata::SpaceObjectReference SpaceObjectReference(CSharpSpaceObjectReference* in) {
    return Sirikata::SpaceObjectReference(
        SpaceID(&in->space),
        ObjectReference(&in->reference)
    );
}

SIRIKATA_PLUGIN_EXPORT_C
void* ConvertSpaceObjectReferenceCSharpToCPP(CSharpSpaceObjectReference* csharp_sor) {
    return (void*) new Sirikata::SpaceObjectReference( SpaceObjectReference(csharp_sor) );
}



/*
SIRIKATA_PLUGIN_EXPORT_C
Sirikata::int64 ConvertTimeCPPToCSharpAndFree(void* jarg) {
    Sirikata::Time* arg = (Sirikata::Time*)jarg;
    Sirikata::int64 ticks = ConvertTime(*arg);
    delete arg;
    return ticks;
    }*/
/*

SIRIKATA_PLUGIN_EXPORT_C
void* ConvertTimeCSharpToCPP(Sirikata::int64 csharp_time) {
    return (void*) new Sirikata::Time( Time(csharp_time) );
}
*/

Sirikata::Time RawTime(Sirikata::int64 ticks) {
    return Sirikata::Time::microseconds(ticks);
}



void ConvertDuration(const Sirikata::Duration& in, CSharpDuration* out) {
    Sirikata::uint64 micro=in.toMicroseconds();
    Sirikata::uint32 lowerlower=micro%65536;
    Sirikata::uint32 upperlower=micro/65536%65536;
    micro/=65536;
    micro/=65536;
    out->lowerTicks = lowerlower+65536*upperlower;
    out->upperTicks = (Sirikata::uint32)micro;
}
void ConvertTime(const Sirikata::Time& in, CSharpDuration* out) {
    Sirikata::uint64 micro=in.raw();
    Sirikata::uint32 lowerlower=micro%65536;
    Sirikata::uint32 upperlower=micro/65536%65536;
    micro/=65536;
    micro/=65536;
    out->lowerTicks = lowerlower+65536*upperlower;
    out->upperTicks = (Sirikata::uint32)micro;
}
CSharpDuration ConvertTime(const Sirikata::Time& in) {
    CSharpDuration retval;
    ConvertTime(in,&retval);
    return retval;
}

SIRIKATA_PLUGIN_EXPORT_C
void ConvertDurationCPPToCSharpAndFree(void* jarg, void* jout) {
    Sirikata::Duration* arg = (Sirikata::Duration*)jarg;
    CSharpDuration* out = (CSharpDuration*)jout;
    ConvertDuration(*arg, out);
    delete arg;
}
SIRIKATA_PLUGIN_EXPORT_C
void ConvertTimeCPPToCSharpAndFree(void* jarg, void* jout) {
    Sirikata::Time* arg = (Sirikata::Time*)jarg;
    CSharpDuration* out = (CSharpDuration*)jout;
    ConvertTime(*arg, out);
    delete arg;
}

Sirikata::Duration Duration(CSharpDuration* in) {
    Sirikata::uint64 micro;
    micro=in->upperTicks;
    micro*=65536;
    micro*=65536;
    micro+=in->lowerTicks;
    return Sirikata::Duration::microseconds(micro);
}

SIRIKATA_PLUGIN_EXPORT_C
void* ConvertDurationCSharpToCPP(CSharpDuration* csharp_duration) {
    return (void*) new Sirikata::Duration( Duration(csharp_duration) );
}


} // namespace Mono
