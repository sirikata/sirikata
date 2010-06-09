/*  Sirikata
 *  FCache.hpp
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
#include "Cache.hpp"
#include <map>
#include <vector>
#include <string>
#include "CacheRecords.hpp"
#include "../SpaceContext.hpp"
#include "../craq_oseg/CraqEntry.hpp"

#ifndef __FCACHE_HPP__
#define __FCACHE_HPP__


namespace Sirikata
{


  typedef double (*FCacheScoreFunc) (const FCacheRecord* a);
  typedef double (*FCacheScoreFuncPrint) (const FCacheRecord* a,bool toPrint);


  class FCache : public Cache
  {
  private:

    typedef std::map<UUID, FCacheRecord*> FCacheDataMap;
    typedef std::pair<UUID,FCacheRecord*> FCacheInsertionPair;
    typedef std::vector<UUID> FCacheObjVec;
    typedef std::map<int,bool> FCacheValidMap;

    class FCachePropDataStruct
    {
    private:
      FCacheDataMap   cacheProper;
      FCacheObjVec       cPropVec;
      FCacheValidMap   invalidMap;
      int                mMaxSize;
      FCacheRecord*  allRecordMem;
      SpaceContext*           ctx;

      const static int MAX_NUM_ITERATIONS_GET_RANDOM = 20;

      void sanityCheck();
      void printInvalidMap();
      void printCPropVec();

    public:

      FCachePropDataStruct(SpaceContext* spctx, int max_size);
      ~FCachePropDataStruct();
      FCacheRecord* getRandom();
      bool insert(const UUID& oid, ServerID bid,int globEvNum,double weight, double distance, double radius,double lookupWeight,double scalingUnits,double vMag);
      bool remove(FCacheRecord* rec);
      int getSize();
      bool hasRoom();
      FCacheRecord* getRecord(const UUID& oid);
      FCacheRecord* popFirst();
      bool empty();
      void clear();
      void printCacheProper();
    };


    static const int FCACHE_RAND_SELECT_NUMBER      = 15;
    static const int FCACHE_KEEP_NUMBER             =  3;



  private:
    double ewmaRecPopPar;
    std::string mName;

    //score function
    FCacheScoreFunc mScoreFunc;
    FCacheScoreFuncPrint mScoreFuncPrint;

  public:
    SpaceContext* ctx;
    CraqEntry mCraqEntry;

  private:
    unsigned int MAX_SIZE_CACHE_PROPER;
    FCachePropDataStruct cacheProper;

    std::vector<FCacheRecord*>worstRecords;


    void updatePopAndIncrement(FCacheRecord* );
    void updatePopNoIncrement(FCacheRecord* );
    void updatePopNoIncrement(std::vector<FCacheRecord*>&);
    void updateRecord(FCacheRecord* rec, ServerID bid,double weight,double distance, double radius, double vMag);

    void generateRandomCacheProper(int numToSelect,std::vector<FCacheRecord*>&randRecs);//generates from cacheProper


    //debugging sanity checks.

    void   printCacheRecordVec(const std::vector<FCacheRecord*>& vec);
    void   checkStaticTimeSanity();
    void   checkCacheProperSanity();

    void   printCacheProper();
    void   removeCacheProper(FCacheRecord* rec);
    void   printWorstRecords();

    void   killRecord();

  public:

    FCache(double avgPopPar,std::string cacheName,FCacheScoreFunc scoreFunc, FCacheScoreFuncPrint scoreFuncPrint, SpaceContext* contex);

    FCache(double avgPopPar,std::string cacheName,FCacheScoreFunc scoreFunc, FCacheScoreFuncPrint scoreFuncPrint, SpaceContext* context,int CACHE_SIZE);



    ~FCache();

    virtual void    insert(const UUID& toInsert, ServerID bid, CacheTimeMS tms, double vMag, double weight, double distance, double radius,double lookupWeight,double unitsScaling);

    virtual const CraqEntry& lookup(const UUID& lookingFor);
    virtual std::string getCacheName();
    virtual void remove(const UUID& toRemove);

    static void   checkThisRecord(const FCacheRecord* bcr);
    static void   printVecRecs(const std::vector<FCacheRecord*> &recs);
  };


  //for scoring/ordering cache records.
  void findMins(std::vector<FCacheRecord*>&vecRecs, FCacheScoreFunc scoreFunc);
}

#endif
