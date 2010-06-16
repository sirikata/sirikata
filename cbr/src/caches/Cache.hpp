/*  Sirikata
 *  Cache.hpp
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
#include "Utility.hpp"
#include <string>
#include <sirikata/cbrcore/SpaceContext.hpp>
#include "../craq_oseg/CraqEntry.hpp"

#ifndef __CACHE_HPP__
#define __CACHE_HPP__

namespace Sirikata
{


  class Cache
  {
  private:


  protected:
    static const int CACHE_MAX_SIZE          = 1000;
    static const int CACHE_NUM_CACHE_REMOVE  =  100;



  public:
      virtual ~Cache() {}

    virtual void insert(const UUID& toInsert, ServerID bid, CacheTimeMS tms, double vMag, double weight, double distance, double radius,double lookupWeight,double unitsScaling) = 0;
    virtual const CraqEntry& lookup(const UUID& lookingFor)                     = 0;
    virtual void  remove(const UUID& toRemove)                         = 0;
    virtual std::string getCacheName()                           = 0;
  };
}


#endif
