/*  Sirikata
 *  Utility.cpp
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

#include "Utility.hpp"
#include <stdlib.h>
#include <string>
#include <cassert>
#include <unistd.h>

#include <cmath>

#define TOLERANCE_FIND_SCALING                    .000000001
#define MAX_NUM_FIND_SCALING_TOTAL_ITERATIONS          10000

#define DEFAULT_FIND_SCALING_LOW                           0
#define DEFAULT_FIND_SCALING_HIGH                        100


namespace Sirikata
{

  //doing my own absolute value function.
  double b_abval(double a)
  {
    if (a < 0)
      return -a;

    return a;
  }

  double getUniform(double min, double max)
  {
    return getUniform()*(max-min) + min;
  }

  double getUniform()
  {
    return ((double)rand() / ((double)(RAND_MAX) + (double) (1)));
  }


  TimeMS convertTimeToAge(const struct timeval& tv)
  {
    TimeMS returner;
    returner = ((TimeMS)tv.tv_sec) * TIMEVAL_SECOND_TO_US + ((TimeMS)tv.tv_usec);
    return returner;
  }

  double mdifftime(const struct timeval& newest, const struct timeval& oldest)
  {
    double seconds = ((double)newest.tv_sec) - ((double)oldest.tv_sec);
    double usec    = ((double)newest.tv_usec) - ((double)oldest.tv_usec);

    return (seconds + (usec/1000000.0));
  }

  double normalGaussianPDF(double val)
  {
    return (1/sqrt(2*PI))*exp(.5*val*val);
  }


  float findScalingAutoRange(float target, float meterRange, float (*fallOffFunction)(float radius, float scaling))
  {
    return findScaling(target,DEFAULT_FIND_SCALING_LOW, DEFAULT_FIND_SCALING_HIGH,meterRange,fallOffFunction);
  }


  double findScalingAutoRange(double target, double meterRange, double (*fallOffFunction)(double radius, double scaling))
  {
    return findScaling(target,DEFAULT_FIND_SCALING_LOW, DEFAULT_FIND_SCALING_HIGH,meterRange,fallOffFunction);
  }


  double mavg(double a, double b)
  {
    return (a+b)/2.0;
  }

  float mavg(float a, float b)
  {
    return (a+b)/2.0;
  }


  float findScaling(float target,float low, float high, float meterRange, float (*fallOffFunction)(float radius, float scaling))
  {
    float checking     = mavg(high,low);
    int numIterations   =             0;

    while (fabs(high-low) > TOLERANCE_FIND_SCALING)
    {
      ++numIterations;
      if (numIterations > MAX_NUM_FIND_SCALING_TOTAL_ITERATIONS)
      {
        std::cout<<"\n\n COULD NOT FIND THE SCALING FACTOR IN ENOUGH ITERATIONS \n\n";
        assert(false);
      }

      float result = (*fallOffFunction)(meterRange,checking);

      if (result > target)
      {
        //increase the checking;
        low      =         checking;
        checking =   mavg(high,low);
      }
      else
      {
        //reduce the checking
        high     =         checking;
        checking =   mavg(high,low);
      }
    }
    return checking;
  }

  //meterRange is probably 1000....1000 meters
  //target is .9 (target% of the pdf need to be within meterRange).
  //low is starting high guess
  //high is starting low guess
  double findScaling(double target,double low, double high, double meterRange,double (*fallOffFunction)(double radius, double scaling))
  {
    double checking     = mavg(high,low);
    int numIterations   =             0;

    while (fabs(high-low) > TOLERANCE_FIND_SCALING)
    {
      ++numIterations;
      if (numIterations > MAX_NUM_FIND_SCALING_TOTAL_ITERATIONS)
      {
        std::cout<<"\n\n COULD NOT FIND THE SCALING FACTOR IN ENOUGH ITERATIONS \n\n";
        assert(false);
      }

      double result = (*fallOffFunction)(meterRange,checking);

      if (result > target)
      {
        //increase the checking;
        low      =         checking;
        checking =   mavg(high,low);
      }
      else
      {
        //reduce the checking
        high     =         checking;
        checking =   mavg(high,low);
      }
    }
    return checking;
  }

}
