/*  Sirikata Utilities -- Math Library
 *  BoundingBox.hpp
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
#ifndef _SIRIKATA_BOUNDING_BOX_HPP
#define _SIRIKATA_BOUNDING_BOX_HPP
namespace Sirikata {
template <typename real> class BoundingBox {
    Vector3<real> mMin;
    Vector3f mAcross;
public:
    BoundingBox() {}
    static BoundingBox<real> null() {
        return BoundingBox<real>(Vector3<real>(0,0,0),0);
    }

    BoundingBox(const Vector3<real>&center, float radius){
        mMin=center-Vector3<real>(radius,radius,radius);
        mAcross=Vector3f(2.0*radius,2.0*radius,2.0*radius);
    }
    template <typename flt> BoundingBox(const BoundingBox<flt>&input) {
        mMin=Vector3<real>(input.mMin);
        mAcross=input.mAcross;
    }
    BoundingBox(const Vector3<real>&imin,const Vector3<real>&imax){
        mMin=imin;
        mAcross=Vector3f(imax-imin);
    }
    
    const Vector3<real> &min()const{
        return mMin;
    }
    const Vector3f& across() const {
        return mAcross;
    }
    Vector3<real> max() const {
        return mMin+Vector3<real>(mAcross);
    }
    Vector3<real> center() const {
        return mMin+Vector3<real>(mAcross * 0.5f);
    }
    BoundingSphere<real> toBoundingSphere() {
        Vector3<real> center=this->center();
        float maxlen=(this->max()-this->center()).lengthSquared();
        float minlen=(this->min()-this->center()).lengthSquared();
        float radius=std::sqrt(minlen<maxlen?maxlen:minlen);
        return BoundingSphere<real>(center,radius);
    }

    BoundingBox<real> merge(const BoundingBox<real>&other) {
        return BoundingBox<real>(min().min(other.min()),
                           max().max(other.max()));
    }
    BoundingBox merge(const Vector3<real>&other) {
        return BoundingBox(min().min(other),
                           max().max(other));
    }
};
}
#endif
