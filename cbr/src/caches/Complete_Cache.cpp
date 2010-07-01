/*  Sirikata
 *  Complete_Cache.cpp
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

#include <algorithm>
#include <map>
#include <list>
#include <functional>
#include <iostream>
#include <iomanip>
#include "Complete_Cache.hpp"
#include "Cache.hpp"
#include <stdio.h>
#include <cassert>
#include <string>
#include "CacheRecords.hpp"
#include "../craq_oseg/CraqEntry.hpp"


namespace Sirikata
{

  Complete_Cache::Complete_Cache(double avgPopPar,std::string complete_name, CompleteCacheScoreFunc ccScorer,CompleteCacheScoreFuncPrint ccScorerPrint,SpaceContext* spctx, float insideRadiusInsert)
    : ewmaPopPar(avgPopPar),
      mName(complete_name),
      mMaxSize(CACHE_MAX_SIZE),
      mScoreFunc(ccScorer),
      mScoreFuncPrint(ccScorerPrint),
      ctx(spctx),
      mPrevTime((spctx->simTime() - Time::null()).toMilliseconds()),
      mInsideRadiusInsert(insideRadiusInsert),
      mCraqEntry(CraqEntry::null())
  {
  }

  Complete_Cache::Complete_Cache(double avgPopPar,std::string complete_name,CompleteCacheScoreFunc ccScorer,CompleteCacheScoreFuncPrint ccScorerPrint,SpaceContext* spctx,int complete_size, float insideRadiusInsert)
    : ewmaPopPar(avgPopPar),
      mName(complete_name),
      mMaxSize(complete_size),
      mScoreFunc(ccScorer),
      mScoreFuncPrint(ccScorerPrint),
      ctx(spctx),
      mPrevTime((spctx->simTime() - Time::null()).toMilliseconds()),
      mInsideRadiusInsert(insideRadiusInsert),
      mCraqEntry(CraqEntry::null())
  {
  }

  void Complete_Cache::checkUpdate()
  {
    if (mPrevTime == (ctx->simTime()- Time::null()).toMilliseconds())
      return;

    mPrevTime = (ctx->simTime() - Time::null()).toMilliseconds();

    //delete the entries in the entire time rec structure
    TimeRecordMap::iterator timeRecMapIter;
    for (timeRecMapIter = timeRecMap.begin(); timeRecMapIter != timeRecMap.end(); ++timeRecMapIter)
    {
      delete timeRecMapIter->second;
    }
    timeRecMap.clear();


    //update the popularities
    IDRecordMap::iterator idrecmapit;
    for (idrecmapit = idRecMap.begin(); idrecmapit != idRecMap.end(); ++idrecmapit)
    {

      idrecmapit->second->popAvg = idrecmapit->second->popAvg*(1-ewmaPopPar) + idrecmapit->second->reqsSinceEvict*ewmaPopPar;
      idrecmapit->second->reqsSinceEvict = 0;

      //refill the time rec structure
      FCacheRecord* rcdTimeRecMap = new FCacheRecord(idrecmapit->second);

      timeRecMap.insert(std::pair<double,FCacheRecord*>((*mScoreFunc)(rcdTimeRecMap),rcdTimeRecMap));
    }
  }

  void Complete_Cache::printAll()
  {
    IDRecordMap::iterator idrecmapit;
    for (idrecmapit = idRecMap.begin(); idrecmapit != idRecMap.end(); ++idrecmapit)
    {
      std::cout<<"\n\nObjID: "<<idrecmapit->second->objid.toString()<<"     reqsSinceEvict: "<<idrecmapit->second->reqsSinceEvict<<"    popavg: "<<idrecmapit->second->popAvg<<"\n\n";
    }
  }



  Complete_Cache::~Complete_Cache()
  {
  }


  //runs through and removes all the
  void Complete_Cache::remove(const UUID& oid)
  {
    IDRecordMap::iterator idrecmapit = idRecMap.find(oid);
    if (idrecmapit == idRecMap.end())
      return; //means that we do not have the object to remove

    //  std::pair<TimeRecordMap::iterator, TimeRecordMap::iterator> eqRange = timeRecMap.equal_range(idrecmapit->second->age);
    std::pair<TimeRecordMap::iterator, TimeRecordMap::iterator> eqRange = timeRecMap.equal_range((*mScoreFunc)(idrecmapit->second));

    TimeRecordMap::iterator timeRecMapIter=eqRange.first;

    for(timeRecMapIter = eqRange.first; timeRecMapIter != eqRange.second; ++timeRecMapIter)
    {
      //run through looking for the corresponding
      if(timeRecMapIter->second->objid == oid)
      {
        //delete its entry and remove the iterator from timeRecMap
        delete timeRecMapIter->second;
        timeRecMap.erase(timeRecMapIter);

        //delete the entry and remove the iterator from idRecMap
        delete idrecmapit->second;
        idRecMap.erase(idrecmapit);

        return;
      }
    }
  }


  //kill the last x objects that were in that map
  void Complete_Cache::maintain()
  {
    if ((int)idRecMap.size() > mMaxSize)
    {
      TimeRecordMap::iterator tMapIter = timeRecMap.begin();

      for (int s=0; (s < 1) && (tMapIter != timeRecMap.end()); ++s)
      {
        //remove the object from the map
        IDRecordMap::iterator idRecMapIterator =  idRecMap.find(tMapIter->second->objid);
        if (idRecMapIterator == idRecMap.end())
        {
          std::cout<<"\n\nInconsistent ids\n\n";
          std::cout<<"This is object id that I was looking for: "<<tMapIter->second->objid.toString()<<"\n\n";
          std::cout.flush();
          assert(false);
        }
        delete idRecMapIterator->second;
        idRecMap.erase(idRecMapIterator);

        //delete this record from the timeRecMap
        delete tMapIter->second;

        //delete the record from the multimap
        timeRecMap.erase(tMapIter++);
      }
    }
  }

  void Complete_Cache::insert(const UUID& toInsert, ServerID bid, CacheTimeMS tms, double vMag, double weight, double distance, double radius,double lookupWeight,double scaler)
  {
    if (distance > mInsideRadiusInsert)
      return;

    IDRecordMap::iterator idRecMapIter = idRecMap.find(toInsert);

    if (idRecMapIter != idRecMap.end())
    {
      std::cout<<"\n\nRemoved the functionality for inserting the same record multiple times\n\n";
      assert(false);
    }
    else
    {
      //means that the object did not already exist in the cache.  We're going to have to specifically add it

      //insert into time-record multimap
      FCacheRecord* rcdTimeRecMap = new FCacheRecord(toInsert,bid,0,0,0,weight,distance,radius,lookupWeight,scaler,ctx,vMag);



      timeRecMap.insert(std::pair<double,FCacheRecord*>((*mScoreFunc)(rcdTimeRecMap),rcdTimeRecMap));


      //insert into id-record map
      FCacheRecord* rcdIDRecMap = new FCacheRecord(toInsert,bid,0,0,0,weight,distance,radius,lookupWeight,scaler,ctx,vMag);
      idRecMap.insert(std::pair<UUID,FCacheRecord*>(toInsert,rcdIDRecMap));

      maintain();
    }
  }



  //static lookup function: looking something up does not change its rank
  const CraqEntry& Complete_Cache::lookup(const UUID& uuid)
  {
    IDRecordMap::iterator idRecMapIter = idRecMap.find(uuid);
    if (idRecMapIter != idRecMap.end())
    {
      mCraqEntry.setServer( idRecMapIter->second->bID);
      mCraqEntry.setRadius( idRecMapIter->second->radius);
      return mCraqEntry;
    }

    static CraqEntry justnothin(CraqEntry::null());
    return justnothin;
  }


  //
  //For lookup, need to do the following: if find the object, change the age of the object to the current time.
  //Delete the oldest
  ServerID Complete_Cache::lookup_dynamic(UUID uuid)
  {
    //note: will not work with popularities now.
    checkUpdate();

    IDRecordMap::iterator idRecMapIter = idRecMap.find(uuid);

    if (idRecMapIter != idRecMap.end())
    {
      //means that we have a record of the object
      ServerID toReturn = idRecMapIter->second->bID;

      //clone record first
      FCacheRecord* toInsertA = new FCacheRecord(idRecMapIter->second);

      //delete the entry first and then enter it again...hackish, but it'll do for now.
      remove(idRecMapIter->second->objid);

      //increase requests
      ++(toInsertA->reqsSinceEvict);
      //clone another record
      FCacheRecord* toInsertB = new FCacheRecord(toInsertA);

      //re-insert
      timeRecMap.insert(std::pair<double,FCacheRecord*>((*mScoreFunc)(toInsertA),toInsertA));
      idRecMap.insert(std::pair<UUID,FCacheRecord*>(uuid,toInsertB));

      return toReturn;
    }

    return NullServerID;
  }


  std::string Complete_Cache::getCacheName()
  {
    return mName;
  }

}
