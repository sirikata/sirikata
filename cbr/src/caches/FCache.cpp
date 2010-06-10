/*  Sirikata
 *  FCache.cpp
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
#include <stdlib.h>
#include "Utility.hpp"
#include "FCache.hpp"
#include "Cache.hpp"
#include "Geometry.hpp"
#include <functional>
#include <algorithm>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include <map>
#include <cassert>
#include <string>
#include <cmath>
#include "CacheRecords.hpp"
#include <stdint.h>
#include <sirikata/cbrcore/SpaceContext.hpp>
#include "../craq_oseg/CraqEntry.hpp"

//#define FCACHE_LOG_REMOVALS


namespace Sirikata
{

  FCache::~FCache()
  {
    std::cout<<"\nWould probably be better if removed the cache records from the end of properRecords vec during maintain rather than beginning\n";

    std::cout<<"\nConsider whether I need a better way of calculating variances for removed times and popularities\n";
    std::cout<<"\nChanged code so that it will always insert a new record.  new records also take on zero popularity, etc.\n";

    std::cout<<"\n\nNeeed to change to other scaling units\n\n";

    worstRecords.clear();
  }


  void FCache::updatePopAndIncrement(FCacheRecord* bcRec)
  {
    CacheTimeMS currTime   = (ctx->simTime() - Time::null()).toMilliseconds();
    bcRec->mLastAccessTime = currTime;

    while (bcRec->lastEvictNum != currTime)
    {
      bcRec->popAvg = ewmaRecPopPar*bcRec->reqsSinceEvict  + (1-ewmaRecPopPar)*bcRec->popAvg;
      ++(bcRec->lastEvictNum);
      bcRec->reqsSinceEvict = 0;

      //debug
      if (bcRec->lastEvictNum > currTime)
      {
        bcRec->printRecord();
        assert(false);
      }
    }
    ++(bcRec->reqsSinceEvict);
  }

  void FCache::updatePopNoIncrement(FCacheRecord* bcRec)
  {
    CacheTimeMS currTime= (ctx->simTime() - Time::null()).toMilliseconds();

    while (bcRec->lastEvictNum != currTime)
    {
      bcRec->popAvg = ewmaRecPopPar*bcRec->reqsSinceEvict  + (1-ewmaRecPopPar)*bcRec->popAvg;
      ++(bcRec->lastEvictNum);
      bcRec->reqsSinceEvict = 0;

      //debug
      if (bcRec->lastEvictNum > currTime)
      {
        std::cout<<"\n\n\nThis is the record";
        bcRec->printRecord();
        std::cout<<"\nGlob evict num:  "<<currTime<<"     last evict num:  "<<bcRec->lastEvictNum<<"\n\n";
        std::cout.flush();
        assert(false);
      }
    }
  }


  void FCache::updatePopNoIncrement(std::vector<FCacheRecord*>& bcRecVec)
  {
    for (int s=0; s < (int)bcRecVec.size(); ++s)
      updatePopNoIncrement(bcRecVec[s]);
  }



  void FCache::updateRecord(FCacheRecord* rec, ServerID bid,double weight,double distance, double radius, double vMag)
  {
    rec->bID      =       bid;
    rec->weight   =    weight;
    rec->distance =  distance;
    rec->radius   =    radius;
    rec->vMag     =      vMag;
  }


  void FCache::insert(const UUID& toInsert, ServerID bid, CacheTimeMS tms, double vMag,double weight, double distance, double radius,double lookupWeight,double unitsScaling)
  {
    //first check to make sure that don't already have a record for this
    FCacheRecord* foundRec = cacheProper.getRecord(toInsert);

    if (foundRec != NULL)    //object existed in cache proper.  we need to update its values
    {
      updateRecord(foundRec,bid,weight,distance,radius, vMag);
    }
    else
    {
      //object did not exist in cache proper: insert it

      if (cacheProper.hasRoom())
      {
        //cacheProper isn't full yet.  Put it there first
        if (!cacheProper.insert(toInsert,bid,(ctx->simTime() - Time::null()).toMilliseconds(),weight,distance,radius,lookupWeight,unitsScaling, vMag))
          assert(false);
      }
      else
      {
        killRecord();

        if (!cacheProper.insert(toInsert,bid,(ctx->simTime()- Time::null()).toMilliseconds(),weight,distance,radius,lookupWeight,unitsScaling, vMag))
          assert(false);
      }
    }
  }

  void FCache::printWorstRecords()
  {
    std::cout<<"\n\nWORST RECORDS\n";
    for(int s=0;s<(int)worstRecords.size();++s)
    {
      std::cout<<"\n"<<worstRecords[s]->objid.toString()<<"\t score:  "<<  (*mScoreFunc)(worstRecords[s]);
    }
    std::cout<<"\n\n";
  }


  void FCache::killRecord()
  {
    generateRandomCacheProper(FCACHE_RAND_SELECT_NUMBER,worstRecords);//generates from cacheProper

    //update globals
    updatePopNoIncrement(worstRecords);

    findMins(worstRecords,mScoreFunc);

    //remove bad record from cacheProper
    FCacheRecord* toRemove = worstRecords.back();

    if (! cacheProper.remove(toRemove))
      assert(false);


    std::vector<FCacheRecord*>::iterator worstIt = worstRecords.begin();
    while(worstIt != worstRecords.end())
    {
      if( (*worstIt)->objid == toRemove->objid)
        worstIt = worstRecords.erase(worstIt);
      else
        ++worstIt;
    }
  }


  const CraqEntry& FCache::lookup(const UUID& lookingFor)
  {
    FCacheRecord* rec = cacheProper.getRecord(lookingFor);

    if (rec != NULL)
    {
      //record is in cacheProper
      updatePopAndIncrement(rec);

      mCraqEntry.setServer(rec->bID);
      mCraqEntry.setRadius(rec->radius);
      return mCraqEntry;
    }

    static CraqEntry justnothin(CraqEntry::null());
    return justnothin;
  }



  //this function just generates numToSelect random values into randIndices.
  void FCache::generateRandomCacheProper(int numToSelect,std::vector<FCacheRecord*>&randRecs)
  {
    if (cacheProper.empty())
      assert(false);

    for (int s=0; s < numToSelect; ++s)
    {
      FCacheRecord* rec = cacheProper.getRandom();
      if (rec == NULL)
      {
        int sizer = cacheProper.getSize();
        std::cout<<"\n\n"<<sizer<<"\n\n";
        assert(false);
      }

      randRecs.push_back(rec);
    }
  }

  void FCache::printCacheRecordVec(const std::vector<FCacheRecord*>& vec)
  {
    std::cout<<"\nPrinting entire cache record vector\n";
    for (int s=0; s < (int) vec.size(); ++s)
    {
      vec[s]->printRecord();
      std::cout<<"\n";
    }
  }


  FCache::FCache(double avgPopPar,std::string cacheName,FCacheScoreFunc scoreFunc, FCacheScoreFuncPrint scoreFuncPrint, SpaceContext* spctx)
    : ewmaRecPopPar(avgPopPar),
      mName(cacheName),
      mScoreFunc(scoreFunc),
      mScoreFuncPrint(scoreFuncPrint),
      ctx(spctx),
      MAX_SIZE_CACHE_PROPER (CACHE_MAX_SIZE),
      cacheProper(spctx, MAX_SIZE_CACHE_PROPER),
      mCraqEntry(CraqEntry::null())
  {
  }

  FCache::FCache(double avgPopPar,std::string cacheName,FCacheScoreFunc scoreFunc, FCacheScoreFuncPrint scoreFuncPrint, SpaceContext* spctx,int CACHE_SIZE)
    : ewmaRecPopPar(avgPopPar),
      mName(cacheName),
      mScoreFunc(scoreFunc),
      mScoreFuncPrint(scoreFuncPrint),
      ctx(spctx),
      MAX_SIZE_CACHE_PROPER (CACHE_SIZE),
      cacheProper(spctx, CACHE_SIZE),
      mCraqEntry(CraqEntry::null())
  {
  }


  std::string FCache::getCacheName()
  {
    return mName;
  }

  //remove the object id associated with oid
  void FCache::remove(const UUID& oid)
  {
    FCacheRecord* rec;

    //check if record exists in cacheProper
    rec = cacheProper.getRecord(oid);
    if (rec != NULL)
    {

      //check if the record is in worst records
      std::vector<FCacheRecord*>::iterator it = worstRecords.begin();
      while(it != worstRecords.end())
      {
        if ( (*it)->objid == oid)
          it = worstRecords.erase(it);
        else
          ++it;
      }

      //record exists in cacheProper
      removeCacheProper(rec);
    }
  }

  void FCache::removeCacheProper(FCacheRecord* rec)
  {
    //Time to remove it.
    if (! cacheProper.remove(rec))
      assert(false); //means that we're trying to remove something that's not there.
  }



  void FCache::printVecRecs(const std::vector<FCacheRecord*> &recs)
  {
    for (int s=0; s < (int) recs.size(); ++s)
      std::cout<<"Pointer:  "<<PTR_AS_INT(recs[s])<<"   objid:"<<recs[s]->objid.toString()<<"\n";
  }


  //changes the vector so that it is stuffed with the worst three (the worst is at the end).
  void findMins(std::vector<FCacheRecord*>&vec,FCacheScoreFunc scoreFunc)
  {
    double s1 = -1;
    FCacheRecord* obj1 = NULL;
    bool foundS1 = false;

    double s2 = -1;
    FCacheRecord* obj2 = NULL;
    bool foundS2 = false;

    double score = 0;

    for (int s=0; s < (int)vec.size(); ++s)
    {
      score = (*scoreFunc)(vec[s]);

      if (! foundS1)
      {
        s1 = score;
        obj1 = vec[s];
        foundS1 = true;
      }
      else if (s1 >= score)
      {
        s2 = s1;
        obj2 = obj1;
        if (foundS1)
          foundS2 = true;

        s1 = score;
        obj1 = vec[s];
        foundS1 = true;
      }
      else if (! foundS2)
      {
        s2 = score;
        obj2 = vec[s];
        foundS2 = true;
      }
      else if (s2 >= score)
      {
        s2 = score;
        obj2 = vec[s];
        foundS2 = true;
      }
    }
    vec.clear();
    vec.push_back(obj2);
    vec.push_back(obj1);

  }


  //this is where slab allocating is a good idea;
  FCache::FCachePropDataStruct::FCachePropDataStruct(SpaceContext* spctx, int max_size)
    : mMaxSize(max_size),
      allRecordMem(new FCacheRecord[max_size])
  {
    ctx = spctx;
    //initialize the structures.
    for (int s=0; s < max_size; ++s)
    {
      cPropVec.push_back(UUID());
      invalidMap[s] = true; //an index for cPropVec as well as for allRecordData
    }
    sanityCheck();
  }

  FCacheRecord* FCache::FCachePropDataStruct::getRandom()
  {
    sanityCheck();
    int index;
    int numAttempts= 0;

    FCacheRecord* returner = NULL;

    if(empty())
    {
      sanityCheck();
      return NULL;
    }

    while (numAttempts < MAX_NUM_ITERATIONS_GET_RANDOM)
    {
      ++numAttempts;
      index = rand() % mMaxSize;
      if (invalidMap.find(index) == invalidMap.end())  //means that the object is valid and can be returned
      {
        UUID oid = cPropVec[index];
        returner = cacheProper[oid];

        if (returner == NULL)
          assert(false);

        sanityCheck();
        return returner;
      }
    }

    //if can't find something after that many attempts, just return front of the map
    returner = cacheProper.begin()->second;
    if (returner == NULL)
      assert(false);

    sanityCheck();
    return returner;
  }



  void FCache::FCachePropDataStruct::printCacheProper()
  {
    FCacheDataMap::iterator cpIt;
    int s=0;
    for (cpIt = cacheProper.begin(); cpIt != cacheProper.end(); ++cpIt)
    {
      std::cout<<"\n"<< cpIt->second->objid.toString()<< "    "<<s;
      std::cout.flush();
      ++s;
    }
  }



  void FCache::FCachePropDataStruct::sanityCheck()
  {
    int propVecSize = (int)     cPropVec.size();
    int cacheSize   = (int)  cacheProper.size();
    int invalidSize = (int)   invalidMap.size();

    if (propVecSize != mMaxSize)
      assert(false);
    if (invalidSize + cacheSize != mMaxSize)
      assert(false);

  }

  bool FCache::FCachePropDataStruct::insert(const UUID& oid, ServerID bid,int globEvNum,double weight, double distance, double radius, double lookupWeight,double mscalingUnits, double vMag)
  {
    sanityCheck();
    if ((int)cacheProper.size() >= mMaxSize)
      return false;  //full, user must request some removes before we can process things.

    //not full, can proceed with insert
    if (invalidMap.empty())
    {
      std::cout<<"\nThis is the size of cacheProper:  "<<cacheProper.size()<<"\n";
      std::cout<<"\nThis is mMaxSize:   "<<mMaxSize<<"\n\n";
      assert(false); //shouldn't happen if above check passed
    }

    int index = invalidMap.begin()->first; //need to take first because first contains the indexing scheme
    invalidMap.erase(invalidMap.begin());

    allRecordMem[index].copy(oid,bid,0,0,globEvNum,index,weight,distance,radius,lookupWeight,mscalingUnits, ctx, (ctx->simTime()-Time::null()).toMilliseconds(), vMag);


    cPropVec    [index]  =                  oid;
    cacheProper   [oid]  = allRecordMem + index;

    sanityCheck();
    return true;
  }



  bool FCache::FCachePropDataStruct::remove(FCacheRecord* rec)
  {
    sanityCheck();
    FCacheDataMap::iterator cPropIt = cacheProper.find(rec->objid);

    if (cPropIt == cacheProper.end()) //cannot remove object that we do not own
    {
      assert(false);
      sanityCheck();
      return false;
    }

    //erase from cache proper
    cacheProper.erase(cPropIt);

    //say that the entry in cPropVec is now worthless.
    invalidMap[rec->vecIndex] = true;
    cPropVec[rec->vecIndex  ] = UUID();  //should not be necessary, but will aid in debugging

    sanityCheck();
    return true;
  }

  int FCache::FCachePropDataStruct::getSize()
  {
    sanityCheck();
    return (int)cacheProper.size();
  }



  FCache::FCachePropDataStruct::~FCachePropDataStruct()
  {
    delete [] allRecordMem;

    sanityCheck();

    cacheProper.clear();
    cPropVec.clear();
    invalidMap.clear();
  }


  //who I send this record to is not allowed to delete it!
  FCacheRecord* FCache::FCachePropDataStruct::getRecord(const UUID&oid)
  {
    sanityCheck();
    FCacheDataMap::iterator propIt;

    propIt = cacheProper.find(oid);
    if(propIt == cacheProper.end())
    {
      sanityCheck();
      return NULL;
    }

    sanityCheck();
    return propIt->second;
  }


  bool FCache::FCachePropDataStruct::hasRoom()
  {
    sanityCheck();
    return ((int)cacheProper.size() < mMaxSize);
  }


  //This function removes the first record from the map
  //whomever I send the record to is not allowed to delete it.
  FCacheRecord*  FCache::FCachePropDataStruct::popFirst()
  {
    sanityCheck();
    if (cacheProper.empty())  //nothing to pop
    {
      assert(false);
      sanityCheck();
      return NULL;
    }

    FCacheRecord* rec = cacheProper.begin()->second;
    if (rec == NULL)
      assert (false);


    invalidMap [rec->vecIndex]       =                            true;
    cPropVec   [rec->vecIndex]       =                       UUID();  //should not be necessary, but will aid in debugging
    FCacheDataMap::iterator finder   =    cacheProper.find(rec->objid);
    if(finder == cacheProper.end())
      assert(false);

    cacheProper.erase(finder);

    sanityCheck();
    return rec;
  }

  bool FCache::FCachePropDataStruct::empty()
  {
    sanityCheck();
    return cacheProper.empty();
  }


  void FCache::FCachePropDataStruct::clear()
  {
    sanityCheck();
    cacheProper.clear();
    invalidMap.clear();

    for (int s=0; s < mMaxSize; ++s)
      invalidMap[s] = true;


    //debugging
    if (! empty())
      assert (false);

    std::cout.flush();

    sanityCheck();
  }


  void FCache::FCachePropDataStruct::printInvalidMap()
  {
    FCacheValidMap::iterator it;
    for (it = invalidMap.begin(); it != invalidMap.end(); ++it)
      std::cout<<"\n"<<it->first;
  }


  void FCache::FCachePropDataStruct::printCPropVec()
  {
    std::cout<<"\n\n";
//     for (int s=0; s < (int)cPropVec.size(); ++s)
//       std::cout<<"\t"<<cPropVec[s];
    std::cout<<"\n\n";
  }

}
