/*  Sirikata
 *  CacheLRUOriginal.hpp
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

#ifndef ___SIRIKATA_CRAQ_CACHE_GOOD_HPP___
#define ___SIRIKATA_CRAQ_CACHE_GOOD_HPP___




#include <map>
#include <functional>
#include <vector>
#include <list>
#include <boost/bind.hpp>
#include <sirikata/cbrcore/Timer.hpp>
#include <boost/thread/mutex.hpp>
#include "../craq_oseg/CraqEntry.hpp"
#include <sirikata/cbrcore/Context.hpp>
#include "CraqCache.hpp"
#include <sirikata/core/util/UUID.hpp>

namespace Sirikata
{
  struct CraqCacheRecordLRUOriginal
  {
    UUID obj_id;
    int age;
    CraqEntry sID;
    CraqCacheRecordLRUOriginal():sID(CraqEntry::null()){}
  };


  class CacheLRUOriginal : public CraqCache
  {
  private:
    Context* mContext;

      typedef std::tr1::unordered_map<UUID,CraqCacheRecordLRUOriginal*,UUID::Hasher> IDRecordMap;
    IDRecordMap idRecMap;

    typedef std::multimap<int,CraqCacheRecordLRUOriginal*> TimeRecordMap;
    TimeRecordMap timeRecMap;
    Network::IOStrand* mStrand;

    Timer mTimer;
    void maintain();
    bool satisfiesCacheAgeCondition(int inAge);

    boost::mutex mMutex;

    double insertMilliseconds;
    int numInserted;
    double maintainDur;
    int numMaintained;


      uint32 mMaxCacheSize; //what is the most number of objects that we can have in the craq cache before we start deleting them
      uint32 mCleanGroupSize; //how many should delete at a time when we get to our limit.
      Duration mEntryLifetime; //maximum age of a cache entry

  public:
    CacheLRUOriginal(Context* ctx, uint32 maxSize,uint32 cleanGroupSize,Duration entryLifetime);
    virtual ~CacheLRUOriginal();

    virtual void insert(const UUID& uuid, const CraqEntry& sID);
    virtual const CraqEntry& get(const UUID& uuid);
    virtual void remove(const UUID& uuid);
  };
}

#endif
