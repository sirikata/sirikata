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
