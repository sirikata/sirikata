/*  Sirikata
 *  ExpIntegral.cpp
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
#include "ExpIntegral.hpp"
#include <boost/math/special_functions/erf.hpp>
#include <iostream>
#include <string.h>

static double square(double x) {
    return x*x;
}

static double expFunction(double k,double x, double y, double u, double v) {
    return exp(-k*(square(x-u)+square(y-v)));
}
static double myerf(double value) {
    return boost::math::erf(value);
}
static double integralExpFunction(double k, double xmin, double xmax, double ymin, double ymax, double umin, double umax, double vmin, double vmax) {
    double e=2.7182818284590451;
    double j=xmin;
    double l=xmax;
    double m=ymin;
    double n=ymax;
    double p=umin;
    double q=umax;
    double r=vmin;
    double s=vmax;
    double pi=3.1415926535897931;
    double sqrtloge=1;
    double loge=1;
    return (1./(4*k*log(e)))
        *pi*(  ( j - p)*myerf(sqrt(k)*(j - p)*sqrtloge)
             + (-l + p)*myerf(sqrt(k)*(l - p)*sqrtloge)
             + (-j + q)*myerf(sqrt(k)*(j - q)*sqrtloge)
             + ( l - q)*myerf(sqrt(k)*(l - q)*sqrtloge)
               + (exp(-k*square(j - p))
                  - exp(-k*square(j - q)))/(sqrt(k)*sqrt(pi)*sqrtloge) +
               (-exp(-k*square(l - p)) + exp(-k*square(l - q)))/(sqrt(k)*sqrt(pi)*sqrtloge))
        *(  ( m - r)*myerf(sqrt(k)*(m - r)*sqrtloge)
            + (-n + r)*myerf(sqrt(k)*(n - r)*sqrtloge)
            + (-m + s)*myerf(sqrt(k)*(m - s)*sqrtloge)
            + ( n - s)*myerf(sqrt(k)*(n - s)*sqrtloge)
            + (exp(-k*square(m - r)) - exp(-k*square(m - s)))/(sqrt(k)*sqrt(pi)*sqrtloge)
            + (-exp(-k*square(n - r)) + exp(-k*square(n - s)))/(sqrt(k)*sqrt(pi)*sqrtloge));
}
namespace Sirikata {
double integralExpFunction(double k, const Vector3d& xymin, const Vector3d& xymax, const Vector3d& uvmin, const Vector3d& uvmax){
    return ::integralExpFunction(k,xymin.x,xymax.x,xymin.y,xymax.y,uvmin.x,uvmax.x,uvmin.y,uvmax.y);
}
}
int maino (int argc, char**argv) {
    float k=atof(argv[1]);

    float xmin=-1;
    float xmax= 1;
    float ymin=-1;
    float ymax= 1;
    float arg=atof(argv[2]);
    float umin=-arg;
    float umax= arg;
    float vmin=-arg;
    float vmax= arg;

    std::cout << " Function evaluated at hundred: "<<
        expFunction(1,100,100,100,100)<<
        " Integration of function over a lot "<<integralExpFunction(k,
                                xmin,
                                xmax,
                                ymin,
                                ymax,
                                umin,
                                umax,
                                vmin,
                                vmax);
    return 0;
}
