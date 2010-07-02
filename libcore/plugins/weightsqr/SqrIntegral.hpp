/*  Sirikata
 *  SqrIntegral.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {
class SqrIntegral {
    enum {MAX_CACHE=2};
    double cachedCutoff[MAX_CACHE];
    double cachedDimensions[MAX_CACHE][2];
    double cachedFlatness[MAX_CACHE];
    double infiniteIntegral[MAX_CACHE];
    bool normalize;
    void *gsl_rng_r;
    void *gsl_monte_plane_state_s;
    static bool approxEqual(double a, double b) {
        double eps=1.0e-6;
        if (a>b) {
            return a-b<eps;
        }
        return b-a<eps;
    }
public:
    SqrIntegral(bool normalize);
    typedef double result_type;
    double integrate(double cutoff, double flatness,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax, double*error);
    double computeWithGivenCutoff(int whichCutoff,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax);
    double computeInfiniteIntegral(int whichCutoff);
    double operator()(float cutoff, float flatness, const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax){
        double deltaLocation=(((xymin+xymax)-(uvmin+uvmax))*.5).length();
        deltaLocation/=flatness;
        deltaLocation+=cutoff;
        if (deltaLocation<2.7) deltaLocation=2.7;
        double logLocDelta=deltaLocation*log(deltaLocation);
        double r1=(xymax-xymin).length();
        double r2=(uvmax-uvmin).length();
        return 2.*r1*r2/(logLocDelta*logLocDelta);
    }

};
}
