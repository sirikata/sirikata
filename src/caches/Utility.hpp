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
