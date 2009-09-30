#ifndef ___CBR_CRAQ_CACHE_GOOD_HPP___
#define ___CBR_CRAQ_CACHE_GOOD_HPP___




#include <map>
#include <functional>
#include <vector>
#include <list>
#include <boost/bind.hpp>
#include "Utility.hpp"
#include "ServerNetwork.hpp"
#include "Timer.hpp"



namespace CBR
{
  static const int LARGEST_CRAQ_CACHE_SIZE =  3000; //what is the most number of objects that we can have in the craq cache before we start deleting them
  static const int NUM_CRAQ_CACHE_REMOVE   =   25; //how many should delete at a time when we get to our limit.

  static const int MAXIMUM_CRAQ_AGE        = 8800; //maximum age is 8.8 seconds


  struct CraqCacheRecord
  {
    UUID obj_id;
    int age;
    ServerID sID;
  };
  
  
  class CraqCacheGood
  {
  private:
    typedef std::map<UUID,CraqCacheRecord*> IDRecordMap;
    IDRecordMap idRecMap;

    typedef std::multimap<int,CraqCacheRecord*> TimeRecordMap;
    TimeRecordMap timeRecMap;

    
    Timer mTimer;
    void maintain();
    bool satisfiesCacheAgeCondition(int inAge);
    
    
  public:
    CraqCacheGood();
    ~CraqCacheGood();
    
    void insert(const UUID& uuid, const ServerID& sID);
    ServerID get(const UUID& uuid);
  };
}
  
#endif
