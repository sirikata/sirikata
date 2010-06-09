
#include <iostream>
#include <iomanip>
#include "Utility.hpp"
#include <string>
#include "../SpaceContext.hpp"
#include "../craq_oseg/CraqEntry.hpp"

#ifndef __CACHE_HPP__
#define __CACHE_HPP__

namespace Sirikata
{

  
  class Cache
  {
  private:

    
  protected:
    static const int CACHE_MAX_SIZE          = 1000;
    static const int CACHE_NUM_CACHE_REMOVE  =  100;


  
  public:

    virtual void insert(const UUID& toInsert, ServerID bid, CacheTimeMS tms, double vMag, double weight, double distance, double radius,double lookupWeight,double unitsScaling) = 0;
    virtual const CraqEntry& lookup(const UUID& lookingFor)                     = 0;
    virtual void  remove(const UUID& toRemove)                         = 0;
    virtual std::string getCacheName()                           = 0;
  };
}

  
#endif
