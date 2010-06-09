#ifndef ___SIRIKATA_CRAQ_CACHE_HPP___
#define ___SIRIKATA_CRAQ_CACHE_HPP___




#include <map>
#include <functional>
#include <vector>
#include <list>
#include <boost/bind.hpp>
#include "Utility.hpp"
#include "Timer.hpp"

namespace Sirikata
{
  static const int LARGEST_CRAQ_CACHE_SIZE =  100; //what is the most number of objects that we can have in the craq cache before we start deleting them
  static const int NUM_CRAQ_CACHE_REMOVE   =   10; //how many should delete at a time when we get to our limit.

  static const int MAXIMUM_CRAQ_AGE        = 8800; //maximum age is 8.8 seconds


  struct IDAge
  {
    UUID id;
    int age;
  };

  bool compareIDAge(const IDAge& a, const IDAge& b);
  static bool compareFindIDAge(const IDAge& a, const UUID& uid);
  static bool compareEvts(IDAge A, IDAge B);

  class CraqCache
  {
  private:
    typedef std::map<UUID,CraqEntry> ServIDMap;
    ServIDMap mMap;
    typedef std::list < IDAge > AgeList;
    AgeList mAgeList;

    Timer mTimer;

    void maintain();


  public:
    CraqCache();
    ~CraqCache();

    void insert(const UUID& uuid, const CraqEntry& sID);
    CraqEntry get(const UUID& uuid);
  };
}

#endif
