/*  Sirikata
 *  Complete_Cache.hpp
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

namespace Sirikata
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
