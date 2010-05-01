#include <map>
#include <functional>
#include <vector>
#include <list>
#include "Cache.hpp"
#include "Utility.hpp"
#include <string>
#include <stdint.h>
#include "CacheRecords.hpp"
#include <float.h>
#include "../SpaceContext.hpp"
#include "../craq_oseg/CraqEntry.hpp"


#ifndef __COMPLETE_CACHE_HPP__
#define __COMPLETE_CACHE_HPP__

namespace CBR
{

  typedef double (*CompleteCacheScoreFunc) (const FCacheRecord* a);
  typedef double (*CompleteCacheScoreFuncPrint) (const FCacheRecord* a,bool toPrint);


  class Complete_Cache : public Cache
  {
  private:
    typedef std::map<UUID,FCacheRecord*> IDRecordMap;
    IDRecordMap idRecMap;

    typedef std::multimap<double,FCacheRecord*> TimeRecordMap; 
    TimeRecordMap timeRecMap;

    void maintain();


    double ewmaPopPar;
    std::string mName;
    int mMaxSize;

    CompleteCacheScoreFunc mScoreFunc;
    CompleteCacheScoreFuncPrint mScoreFuncPrint;
    SpaceContext* ctx;
    CacheTimeMS mPrevTime;

    float mInsideRadiusInsert;
  
    void checkUpdate();

  
  public:
    CraqEntry mCraqEntry;
    Complete_Cache(double avgPopPar,std::string complete_name, CompleteCacheScoreFunc ccScorer,CompleteCacheScoreFuncPrint ccScorerPrint, SpaceContext* spctx, float insideRadiusInsert= FLT_MAX);
    Complete_Cache(double avgPopPar,std::string complete_name,CompleteCacheScoreFunc ccScorer,CompleteCacheScoreFuncPrint ccScorerPrint,SpaceContext* spctx,int complete_size, float insideRadiusInsert = FLT_MAX);

    ~Complete_Cache();

    //    void insert(UUID toInsert, ServerID bid, CacheTimeMS tms, double vMag);
    virtual void insert(const UUID& toInsert, ServerID bid, CacheTimeMS tms, double vMag, double weight, double distance, double radius,double lookupWeight,double scaler);
  
    virtual const CraqEntry& lookup (const UUID& lookingFor);
    ServerID lookup_dynamic(UUID uuid);
    virtual std::string getCacheName();
    virtual void remove(const UUID& oid);

    void printAll();
  
  };
  
}
#endif
