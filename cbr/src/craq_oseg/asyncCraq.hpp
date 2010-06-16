/*  Sirikata
 *  asyncCraq.hpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Asio.hpp>
#include "asyncUtil.hpp"
#include "asyncConnection.hpp"
#include <sirikata/cbrcore/Timer.hpp>
#include <sirikata/cbrcore/SpaceContext.hpp>


#ifndef __ASYNC_CRAQ_CLASS_H__
#define __ASYNC_CRAQ_CLASS_H__


namespace Sirikata
{


class AsyncCraq
{
public:
  AsyncCraq(SpaceContext* spc, Network::IOStrand* );
  ~AsyncCraq();

  enum AsyncCraqReqStatus{REQUEST_PROCESSED, REQUEST_NOT_PROCESSED};

  void initialize(std::vector<CraqInitializeArgs>);

  Network::IOService* io_service;

  int set(const CraqDataSetGet& cdSet);
  int get(const CraqDataSetGet& cdGet);

  void runTestOfConnection();
  void runTestOfAllConnections();
  void tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);

  int queueSize();


private:

  void processGetResults       (std::vector <CraqOperationResult*> & getRes);
  void processErrorResults     (std::vector <CraqOperationResult*> & errorRes);
  void processTrackedSetResults(std::vector <CraqOperationResult*> & trackedSetRes);


  std::vector<CraqInitializeArgs> mIpAddPort;
  std::vector<AsyncConnection*> mConnections;
  int mCurrentTrackNum;


  SpaceContext* ctx;
  bool connected;
  CraqDataResponseBuffer mReadData;
  CraqDataGetResp mReadSomeData;

  std::queue<CraqDataSetGet*> mQueue;


  void reInitializeNode(int s);
  void checkConnections(int s);

  Timer mTimer;
  SpaceContext* mContext;
  Network::IOStrand* mStrand;


};

}//namespece


#endif
