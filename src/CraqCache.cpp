/*  Sirikata
 *  CraqCache.cpp
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


#include "CraqCache.hpp"
#include <algorithm>
#include <map>
#include <list>
#include <functional>
#include <iostream>
#include <iomanip>
#include "Timer.hpp"


namespace Sirikata
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
    mTimer.start();
  }

  CraqCache::~CraqCache()
  {

  }


  void CraqCache::maintain()
  {
    if ((int)mMap.size()  > LARGEST_CRAQ_CACHE_SIZE)
    {

      //sort list
      mAgeList.sort( compareIDAge);

      //grab oldest and kill
      for (int s=0; s < NUM_CRAQ_CACHE_REMOVE; ++s)
      {
        IDAge element = mAgeList.back();

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
    inserter.age = (int)dur.toMilliseconds();
    mAgeList.push_back(inserter);

    //maintain();
  }




  ServerID CraqCache::get(const UUID& uuid)
  {
    ServIDMap::iterator it;
    it = mMap.find(uuid);


    if (it == mMap.end())
    {
      return NullServerID;
    }

    //means it is in map.  Must decide if too old to return
    AgeList::iterator result;
    result = std::find_if( mAgeList.begin(), mAgeList.end(), boost::bind(compareFindIDAge, _1, uuid));

    Duration dur = mTimer.elapsed();


    if (((int)dur.toMilliseconds()) - ((int)result->age) < MAXIMUM_CRAQ_AGE)
    {
      return it->second;
    }


    //maybe I could delete it here.

    return NullServerID;

  }

}
