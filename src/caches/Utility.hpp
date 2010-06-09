/*  Sirikata
 *  Utility.hpp
 *
 *  Copyright (c) 2010, Behram Mistree
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

#include <iostream>
#include <iomanip>
#include <string>
#include <stdint.h>

#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#define PI 3.14159

#define DEGREES_TO_RADIANS  PI/180.0

namespace Sirikata
{


  typedef int64_t ObjID;
  typedef int BlockID;
  typedef int64_t CacheTimeMS;


  const static int     NullVecIndex   = -1;
  const static BlockID NullBlockID    = -1;
  const static ObjID   NullObjID      = -1;

  const static CacheTimeMS TIME_MS_INFINITY_TIME = 10000000; //not really infinity, but will do for our purposes for now.
  const int TIMEVAL_SECOND_TO_US = 1000000;
  CacheTimeMS convertTimeToAge(const struct timeval& tv);

  double getUniform(double min, double max);
  double getUniform();
  double b_abval(double a);
  double mdifftime(const struct timeval &newest, const struct timeval& oldest);
  double normalGaussianPDF(double val);

  double mavg(double a, double b);
  float mavg(float a, float b);

  double findScalingAutoRange(double target, double meterRange, double (*fallOffFunction)(double radius, double scaling));
  float findScalingAutoRange(float target, float meterRange, float (*fallOffFunction)(float radius, float scaling));
  double findScaling(double target,double low, double high, double meterRange, double (*fallOffFunction)(double radius, double scaling));
  float findScaling(float target,float low, float high, float meterRange, float (*fallOffFunction)(float radius, float scaling));

  enum ObjType {NullObjType, OBJ_TYPE_STATIC, OBJ_TYPE_MOBILE_RANDOM, OBJ_TYPE_QUAKE_TRACE, OBJ_TYPE_SL_TRACE};

}


#define PTR_AS_INT(X) ((uintptr_t)X)

#endif
