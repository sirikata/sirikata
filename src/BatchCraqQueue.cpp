/*  Sirikata
 *  BatchCraqQueue.cpp
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

#include "ObjectSegmentation.hpp"
#include "Utility.hpp"
#include "Statistics.hpp"
#include "Message.hpp"
#include "Statistics.hpp"
#include "OSegLookupTraceToken.hpp"
#include <queue>

#include <sirikata/core/util/ThreadSafeQueueWithNotification.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include "BatchCraqQueue.hpp"

namespace Sirikata
{

  BatchCraqQueue::BatchCraqQueue(IOStrand* strand, ObjectSegmentation* oseg, SpaceContext* con)
    :mOSeg(oseg),
     mStrand(strand),
     ctx(con)
  {
    mQueryQueue = new QueryQueue();
  }


  BatchCraqQueue::~BatchCraqQueue()
  {
    //take off each query and delete it.
    while (! mQueryQueue->empty())
    {
      Query* toDelete = mQueryQueue->front();
      mQueryQueue->pop();
      delete toDelete;
    }
    //delete mQueryQueue;
    delete mQueryQueue;

    //delete my own strand as well.
    delete mStrand;
  }

  //just a wrapper for posting to internalAddToQueue through mStrand
  void BatchCraqQueue::addToQueue(Query* q)
  {
    //note: may have problems because this is a private function
    mStrand->post(boost::bind(&BatchCraqQueue::internalAddToQueue,this, q));
  }

  void BatchCraqQueue::internalAddToQueue(Query* q)
  {
    mQueryQueue->push(q);
    if((int)mQueryQueue->size() > MAX_LENGTH_QUEUE)
      flushQueue();
  }

  void BatchCraqQueue::flushQueue()
  {
    QueryQueue* toPass = mQueryQueue;
    mOSeg->lookupQueue(toPass);
    mQueryQueue = new QueryQueue();
  }



}
