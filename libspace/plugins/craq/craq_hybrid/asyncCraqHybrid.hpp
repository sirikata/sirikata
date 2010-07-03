/*  Sirikata
 *  asyncCraqHybrid.hpp
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

#include "craq_gets/asyncCraqGet.hpp"
#include "craq_sets/asyncCraqSet.hpp"
#include "asyncCraqUtil.hpp"
#include <sirikata/space/SpaceContext.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include "asyncCraqScheduler.hpp"
#include <sirikata/space/OSegLookupTraceToken.hpp>

#ifndef  __ASYNC_CRAQ_HYBRID_HPP__
#define  __ASYNC_CRAQ_HYBRID_HPP__

namespace Sirikata
{

class CraqObjectSegmentation;

  class AsyncCraqHybrid
{
public:
  AsyncCraqHybrid(SpaceContext* con, Network::IOStrand* strand_to_post_results_to, CraqObjectSegmentation* oseg);
  ~AsyncCraqHybrid();

  void initialize(std::vector<CraqInitializeArgs>);

  void set(CraqDataSetGet cdSet, uint64 tracking_number = 0);
  void get(CraqDataSetGet cdGet, OSegLookupTraceToken* traceToken);

  int queueSize();
  int numStillProcessing();

  void stop();

private:

  SpaceContext*  ctx;

  Network::IOStrand* mGetStrand;
  Network::IOStrand* mSetStrand;

  AsyncCraqGet aCraqGet;
  AsyncCraqSet aCraqSet;



};

}//end namespace

#endif
