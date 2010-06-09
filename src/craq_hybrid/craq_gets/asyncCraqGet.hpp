/*  Sirikata
 *  asyncCraqGet.hpp
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


#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "../asyncCraqUtil.hpp"
#include "asyncConnectionGet.hpp"

#include "../../SpaceContext.hpp"
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/network/Asio.hpp>
#include "../../OSegLookupTraceToken.hpp"


#ifndef __ASYNC_CRAQ_GET_CLASS_H__
#define __ASYNC_CRAQ_GET_CLASS_H__

namespace Sirikata
{


class AsyncCraqGet : public AsyncCraqScheduler
  {
  private:

    struct QueueValue
    {
      CraqDataSetGet* cdQuery;
      OSegLookupTraceToken* traceToken;
    };


  public:
    AsyncCraqGet(SpaceContext* con, IOStrand* strand_this_runs_on, IOStrand* strand_to_post_results_to, ObjectSegmentation* parent_oseg_called);
    ~AsyncCraqGet();

    int runReQuery();
    void initialize(std::vector<CraqInitializeArgs>);

    virtual void erroredGetValue(CraqOperationResult* cor);
    virtual void erroredSetValue(CraqOperationResult* cor);


    void get(const CraqDataSetGet& cdGet, OSegLookupTraceToken* traceToken);

    int queueSize();
    int numStillProcessing();
    int getRespCount();
    virtual void stop();

  private:

    void straightPoll();
    std::vector<CraqInitializeArgs> mIpAddPort;
    std::vector<AsyncConnectionGet*> mConnections;
    std::vector<IOStrand*> mConnectionsStrands;



    std::queue<QueueValue*>mQueue;

    void reInitializeNode(int s);
    void readyStateChanged(int s);
    bool checkConnections(int s);
    void pushQueue(QueueValue*qv);
    std::vector<int> mReadyConnections;

    SpaceContext* ctx;
    IOStrand* mStrand;        //strand that the asyncCraqGet is running on.
    IOStrand* mResultsStrand; //strand that we post our results to.
    ObjectSegmentation* mOSeg;

  };


}//end namespace

#endif
