
#include "CraqCache.hpp"
#include "ServerNetwork.hpp"
#include <algorithm>
#include <map>
#include <list>
#include <functional>
#include <iostream>
#include <iomanip>
#include "Timer.hpp"


namespace CBR
{
  bool compareIDAge(const IDAge& a, const IDAge& b)
  {
    return a.age > b.age;
  }

  bool compareFindIDAge(const IDAge& a, const UUID& uid)
  {
    return a.id.toString() == uid.toString();
  }


  CraqCache::CraqCache()
  {

  }

  CraqCache::~CraqCache()
  {

  }

  
  void CraqCache::maintain()
  {
    if (mMap.size()  > LARGEST_CRAQ_CACHE_SIZE)
    {
      //sort list
      mAgeList.sort( compareIDAge);

      //grab oldest 10 and kill

      for (int s=0; s < NUM_CRAQ_CACHE_REMOVE; ++s)
      {
        IDAge element = mAgeList.back();
        //        std::cout<<"\n\nMaintain: removing:  "<<element.id<<"\n\n";
        mMap.erase(element.id);
        mAgeList.pop_back();
      }
    }
  }


  void CraqCache::insert(const UUID& uuid, const ServerID& sID)
  {
    mMap[uuid] = sID;

    AgeList::iterator result;

    
    result = std::find_if( mAgeList.begin(), mAgeList.end(), boost::bind(compareFindIDAge, _1, uuid));
    if (result != mAgeList.end())
    {
      //already exists.  must remove it first.
      mAgeList.erase(result);
    }

    //inserting back into age lists
    Duration dur = mTimer.elapsed();
    IDAge inserter;
    inserter.id = uuid;
    inserter.age = dur.toMilliseconds();
    mAgeList.push_back(inserter);    

    maintain();
  }


  

  ServerID CraqCache::get(const UUID& uuid)
  {
    ServIDMap::iterator it;
    it = mMap.find(uuid);

    if (it == mMap.end())
      return NullServerID;

    //means it is in map.  Must decide if too old to return
    AgeList::iterator result;
    result = std::find_if( mAgeList.begin(), mAgeList.end(), boost::bind(compareFindIDAge, _1, uuid));
    
    Duration dur = mTimer.elapsed();

    if (dur.toMilliseconds() - result->age < MAXIMUM_CRAQ_AGE)
    //    if (dur - result->age < MAXIMUM_CRAQ_AGE)
    {
      return it->second;
    }

    //maybe I could delete it here.

    return NullServerID;      
    
  }

}  
