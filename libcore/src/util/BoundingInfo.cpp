/*  Meru
 *  BoundingInfo.cpp
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
#include "util/Standard.hh"
#include "BoundingInfo.hpp"
#include "BoundingSphere.hpp"
#include "BoundingBox.hpp"
namespace Sirikata {
BoundingInfo::BoundingInfo():mMin(0,0,0),mRadius(0),mMax(0,0,0) {

}


BoundingInfo::BoundingInfo(const Vector3f& bbmin, const Vector3f& bbmax, float32 radius):mMin(bbmin),mRadius(radius),mMax(bbmax) {}
static float32 boundingBoxRadius(const Vector3f &bbmin, const Vector3f &bbmax) {
    Vector3f tmin(fabs(bbmin.x),
                  fabs(bbmin.y),
                  fabs(bbmin.z));
    Vector3f tmax(fabs(bbmax.x),
                  fabs(bbmax.y),
                  fabs(bbmax.z));
    Vector3d tedge(tmin.x<tmax.x?tmax.x:tmin.x,
                   tmin.y<tmax.y?tmax.y:tmin.y,
                   tmin.z<tmax.z?tmax.z:tmin.z);
    return (float32)tedge.length();

}
BoundingInfo::BoundingInfo(const Vector3f& bbmin, const Vector3f& bbmax):mMin(bbmin),mMax(bbmax) {
    mRadius=boundingBoxRadius(bbmin,bbmax);
}
BoundingInfo::BoundingInfo(float radius):mMin(-radius,-radius,-radius),mRadius(radius),mMax(radius,radius,radius) {
}
BoundingInfo BoundingInfo::scale(const Vector3f &scale)const {
    float maxscale=scale.x>scale.y?scale.x:scale.y;
    if (maxscale<scale.z) maxscale=scale.z;
    return BoundingInfo(Vector3f(mMin.x*scale.x,mMin.y*scale.y,mMin.z*scale.z),
                        Vector3f(mMax.x*scale.x,mMax.y*scale.y,mMax.z*scale.z),
                        maxscale*mRadius);
}
BoundingInfo::BoundingInfo(const BoundingSphere<float32>& bs):mMin(bs.center().x-bs.radius(),bs.center().y-bs.radius(),bs.center().z-bs.radius()),mRadius(bs.center().length()+bs.radius()),mMax(bs.center().x+bs.radius(),bs.center().y+bs.radius(),bs.center().z+bs.radius()) {}

Vector3f BoundingInfo::center() const{
    return (mMin+mMax)*.5f;
}
Vector3f BoundingInfo::diag() const{
    return mMax-mMin;
}
BoundingBox<float32> BoundingInfo::boundingBox()const {
    return BoundingBox<float32>(Vector3f(mMin.x,mMin.y,mMin.z),
                       Vector3f(mMax.x,mMax.y,mMax.z));
}
BoundingInfo BoundingInfo::merge(const BoundingBox<float32>& bbox) const{
    Vector3f mn(bbox.min().x,bbox.min().y,bbox.min().z);
    Vector3f mx(bbox.max().x,bbox.max().y,bbox.max().z);
    return merge(Vector3f(mn.x,mn.y,mn.z)).
        merge(Vector3f(mn.x,mn.y,mx.z)).
        merge(Vector3f(mn.x,mx.y,mn.z)).
        merge(Vector3f(mn.x,mx.y,mx.z)).
        merge(Vector3f(mx.x,mn.y,mn.z)).
        merge(Vector3f(mx.x,mn.y,mx.z)).
        merge(Vector3f(mx.x,mx.y,mn.z)).
        merge(Vector3f(mx.x,mx.y,mx.z));
}
BoundingInfo BoundingInfo::merge(const Vector3f& point) const{
    float len=point.length();
    return BoundingInfo(Vector3f(point.x<mMin.x?point.x:mMin.x,
                                 point.y<mMin.y?point.y:mMin.y,
                                 point.z<mMin.z?point.z:mMin.z),
                        Vector3f(point.x>mMax.x?point.x:mMax.x,
                                 point.y>mMax.y?point.y:mMax.y,
                                 point.z>mMax.z?point.z:mMax.z),
                        len>mRadius?len:mRadius);

}
BoundingInfo BoundingInfo::merge(const BoundingInfo &bbox)const {
    return BoundingInfo(Vector3f(bbox.mMin.x<mMin.x?bbox.mMin.x:mMin.x,
                                 bbox.mMin.y<mMin.y?bbox.mMin.y:mMin.y,
                                 bbox.mMin.z<mMin.z?bbox.mMin.z:mMin.z),
                        Vector3f(bbox.mMax.x>mMax.x?bbox.mMax.x:mMax.x,
                                 bbox.mMax.y>mMax.y?bbox.mMax.y:mMax.y,
                                 bbox.mMax.z>mMax.z?bbox.mMax.z:mMax.z),
                        bbox.mRadius>mRadius?bbox.mRadius:mRadius);
}

}
