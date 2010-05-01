#ifndef __CRAQ_CACHE_HPP__
#define __CRAQ_CACHE_HPP__

#include "../craq_oseg/CraqEntry.hpp"
#include "../Context.hpp"


namespace CBR
{

  class CraqCache
  {
    public:
      virtual void insert(const UUID& uuid, const CraqEntry& sID) = 0;
      virtual const CraqEntry& get(const UUID& uuid)              = 0;
      virtual void remove(const UUID& uuid)                       = 0;
  };

}

#endif 
