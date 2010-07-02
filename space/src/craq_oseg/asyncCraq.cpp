/*  Sirikata
 *  asyncCraq.cpp
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

#include "asyncCraq.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <string.h>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include "asyncConnection.hpp"
#include <sirikata/core/network/IOServiceFactory.hpp>

using Sirikata::Network::TCPSocket;
using Sirikata::Network::TCPResolver;

namespace Sirikata
{


//nothing to destroy
AsyncCraq::~AsyncCraq()
{
    Network::IOServiceFactory::destroyIOService(io_service);
  //  delete mSocket;
}


//nothing to initialize
AsyncCraq::AsyncCraq(SpaceContext* spx, Network::IOStrand* str)
{
  io_service = Network::IOServiceFactory::makeIOService();

  mContext = spx;
  mStrand  = str;
  mTimer.start();
}


void AsyncCraq::initialize(std::vector<CraqInitializeArgs> ipAddPort)
{

  mIpAddPort = ipAddPort;

  TCPResolver resolver(*io_service);   //a resolver can resolve a query into a series of endpoints.

  mCurrentTrackNum = 10;

  AsyncConnection* tmpConn = new AsyncConnection;
  for (int s=0; s < CRAQ_NUM_CONNECTIONS; ++s)
  {
    mConnections.push_back(tmpConn);
  }

  TCPSocket* passSocket;

  if (((int)ipAddPort.size()) >= CRAQ_NUM_CONNECTIONS)
  {
    //just assign each connection a separate router (in order that they were provided).
    for (int s = 0; s < CRAQ_NUM_CONNECTIONS; ++s)
    {
      TCPResolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[s].ipAdd.c_str(), ipAddPort[s].port.c_str());
      TCPResolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
      passSocket   =  new TCPSocket(*io_service);
      mConnections[s]->initialize(passSocket,iterator,ctx,mStrand); //note maybe can pass this by reference?
    }
  }
  else
  {
    int whichRouterServingPrevious = -1;
    int whichRouterServing;
    double percentageConnectionsServed;

    TCPResolver::iterator iterator;

    for (int s=0; s < CRAQ_NUM_CONNECTIONS; ++s)
    {
      percentageConnectionsServed = ((double)s)/((double) CRAQ_NUM_CONNECTIONS);
      whichRouterServing = (int)(percentageConnectionsServed*((double)ipAddPort.size()));

      //      whichRouterServing = 0; //bftm debug


      if (whichRouterServing  != whichRouterServingPrevious)
      {
        TCPResolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[whichRouterServing].ipAdd.c_str(), ipAddPort[whichRouterServing].port.c_str());

        iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        whichRouterServingPrevious = whichRouterServing;
      }
      passSocket   =  new TCPSocket(*io_service);
      mConnections[s]->initialize(passSocket,iterator,ctx,mStrand); //note maybe can pass this by reference?
    }
  }

}

void AsyncCraq::runTestOfAllConnections()
{
  //temporarily empty.
}

void AsyncCraq::runTestOfConnection()
{
  //temporarily empty.
}


//assumes that we're already connected.
int AsyncCraq::set(const CraqDataSetGet& dataToSet)
{
  CraqDataSetGet* push_queue = new CraqDataSetGet(dataToSet.dataKey, dataToSet.dataKeyValue, dataToSet.trackMessage, CraqDataSetGet::SET );
  push_queue->messageType = CraqDataSetGet::SET;//force this to be a set message.


  if (push_queue->trackMessage)
  {
    push_queue->trackingID = mCurrentTrackNum;
    ++mCurrentTrackNum;

    mQueue.push(push_queue);
    return mCurrentTrackNum -1;
  }

  //we got all the way through without finding a ready connection.  Need to add query to queue.
  mQueue.push(push_queue);
  return 0;
}


/*
  Returns the size of the queue of operations to be processed
*/
int AsyncCraq::queueSize()
{
  return mQueue.size();
}


int AsyncCraq::get(const CraqDataSetGet& dataToGet)
{
  CraqDataSetGet* push_queue = new CraqDataSetGet(dataToGet.dataKey, dataToGet.dataKeyValue, dataToGet.trackMessage, CraqDataSetGet::GET);

  //force this to be a set message.
  //dataToGet.messageType = CraqDataSetGet::GET;
  //we got all the way through without finding a ready connection.  Need to add query to queue.
  mQueue.push(push_queue);

  return 0;
}



/*
  tick processes
  tick returns all the
*/
void AsyncCraq::tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults)
{
  Duration tickBeginDur = mTimer.elapsed();

  int numHandled = io_service->poll();

  if (numHandled == 0)
  {
    io_service->reset();
  }


  if (tickBeginDur.toMilliseconds() > 100000)
  {
    Duration tickEndDur = mTimer.elapsed();
    int procPollDur = tickEndDur.toMilliseconds() - tickBeginDur.toMilliseconds();
    if (procPollDur > 1)
    {
      printf("\n\nHUGEPOLL  %i\n\n", procPollDur);
    }
  }


  std::vector<CraqOperationResult*> tickedMessages_getResults;
  std::vector<CraqOperationResult*> tickedMessages_errorResults;
  std::vector<CraqOperationResult*> tickedMessages_trackedSetResults;

  for (int s=0; s < (int)mConnections.size(); ++s)
  {
    if (tickedMessages_getResults.size() != 0)
      tickedMessages_getResults.clear();

    if (tickedMessages_errorResults.size() != 0)
      tickedMessages_errorResults.clear();

    if (tickedMessages_trackedSetResults.size() != 0)
      tickedMessages_trackedSetResults.clear();


    mConnections[s]->tick(tickedMessages_getResults,tickedMessages_errorResults,tickedMessages_trackedSetResults);

    getResults.insert(getResults.end(), tickedMessages_getResults.begin(), tickedMessages_getResults.end());
    trackedSetResults.insert(trackedSetResults.end(), tickedMessages_trackedSetResults.begin(), tickedMessages_trackedSetResults.end());

    processErrorResults(tickedMessages_errorResults);

    checkConnections(s); //checks whether connection is ready for an additional query and also checks if it needs a new socket.
  }

#ifdef DEBUG_CRAQ
  if (tickBeginDur.toMilliseconds() > 100000)
  {
    Duration tickEnder = mTimer.elapsed();
    int tickTime = tickEnder.toMilliseconds() - tickBeginDur.toMilliseconds();
    if(tickTime > 2)
      printf("\n\nHUGECRAQ %i, %i, %i \n\n", tickTime, (int) getResults.size(), (int)trackedSetResults.size());
  }
#endif

}


/*
  errorRes is full of results that went bad from a craq connection.  In the future, we may do something more intelligent, but for now, we are just going to put the request back in mQueue
*/
void AsyncCraq::processErrorResults(std::vector <CraqOperationResult*> & errorRes)
{
  for (int s=0;s < (int)errorRes.size(); ++s)
  {
    if (errorRes[s]->whichOperation == CraqOperationResult::GET)
    {
      CraqDataSetGet* push_queue = new CraqDataSetGet (errorRes[s]->objID,errorRes[s]->servID,errorRes[s]->tracking, CraqDataSetGet::GET);
      //      mQueue.push(cdSG);
      mQueue.push(push_queue);
    }
    else
    {
      CraqDataSetGet* push_queue = new CraqDataSetGet(errorRes[s]->objID,errorRes[s]->servID,errorRes[s]->tracking, CraqDataSetGet::SET);
      mQueue.push(push_queue);
    }

    delete errorRes[s];

  }
}


/*
  This function checks connection s to see if it needs a new socket or if it's ready to accept another query.
*/
void AsyncCraq::checkConnections(int s)
{
  //  Duration checkConnBeginDur = mTimer.elapsed();


  if (s >= (int)mConnections.size())
    return;

  int numOperations = 0;

  mConnections[s]->ready();


  if (mConnections[s]->ready() == AsyncConnection::READY)
  {
    if (mQueue.size() != 0)
    {
      //need to put in another
      CraqDataSetGet* cdSG = mQueue.front();
      mQueue.pop();

      ++numOperations;

      if (cdSG->messageType == CraqDataSetGet::GET)
      {
        //perform a get in  connections.
        mConnections[s]->get(cdSG->dataKey);
      }
      else if (cdSG->messageType == CraqDataSetGet::SET)
      {
        //performing a set in connections.
        mConnections[s]->set(cdSG->dataKey, cdSG->dataKeyValue, cdSG->trackMessage, cdSG->trackingID);
      }

      delete cdSG;
    }
  }
  else if (mConnections[s]->ready() == AsyncConnection::NEED_NEW_SOCKET)
  {
    //need to create a new socket for the other
    reInitializeNode(s);
    std::cout<<"\n\nbftm debug: needed new connection.  How long will this take? \n\n";
  }
}




//means that we need to connect a new socket to the service.
void AsyncCraq::reInitializeNode(int s)
{
  if (s >= (int)mConnections.size())
    return;

  TCPSocket* passSocket;

  TCPResolver resolver(*io_service);   //a resolver can resolve a query into a series of endpoints.

  if ( ((int)mIpAddPort.size()) >= CRAQ_NUM_CONNECTIONS)
  {
    TCPResolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[s].ipAdd.c_str(), mIpAddPort[s].port.c_str());
    TCPResolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
    passSocket   =  new TCPSocket(*io_service);
    mConnections[s]->initialize(passSocket,iterator,ctx,mStrand); //note maybe can pass this by reference?
  }
  else
  {

    TCPResolver::iterator iterator;

    double percentageConnectionsServed = ((double)s)/((double) CRAQ_NUM_CONNECTIONS);
    int whichRouterServing = (int)(percentageConnectionsServed*((double)mIpAddPort.size()));

    //    whichRouterServing = 0; //bftm debug

    TCPResolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[whichRouterServing].ipAdd.c_str(), mIpAddPort[whichRouterServing].port.c_str());
    iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.

    passSocket   =  new TCPSocket(*io_service);
    mConnections[s]->initialize(passSocket,iterator,ctx,mStrand); //note maybe can pass this by reference?
  }
}



}
