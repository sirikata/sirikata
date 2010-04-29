#include "asyncCraqSet.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <string.h>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/asio.hpp>

#include "../asyncCraqUtil.hpp"
#include "asyncConnectionSet.hpp"
#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include "../../ObjectSegmentation.hpp"
#include <functional>
#include <sirikata/network/Asio.hpp>


#define CRAQ_MAX_PUSH_SET 5

namespace CBR
{


  void AsyncCraqSet::stop()
  {
#ifdef ASYNC_CRAQ_SET_DEBUG
    std::cout<<"\n\nReceived a stop in async craq set\n";
#endif
    for (int s=0; s < (int) mConnections.size(); ++s)
      mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionSet::stop,mConnections[s]));

    PollingService::stop();
  }


  AsyncCraqSet::~AsyncCraqSet()
  {
    for (int s= 0;s < (int) mConnections.size(); ++s)
    {
      delete mConnectionsStrands[s];
      delete mConnections[s];
    }
    mConnections.clear();
    mConnectionsStrands.clear();
  }

  AsyncCraqSet::AsyncCraqSet(SpaceContext* con, IOStrand* strand_this_runs_on, IOStrand* strand_to_post_results_to, ObjectSegmentation* parent_oseg_called)
   : PollingService(strand_this_runs_on, Duration::milliseconds((int64)1), con, "AsyncCraqSet"),
      ctx(con),
      mStrand(strand_this_runs_on),
      mResultsStrand(strand_to_post_results_to),
      mOSeg(parent_oseg_called)
  {
  }



  void AsyncCraqSet::initialize(std::vector<CraqInitializeArgs> ipAddPort)
  {
#ifdef ASYNC_CRAQ_SET_DEBUG
    std::cout<<"\n\nbftm debug: asynccraqset initialize\n\n";
#endif
    mIpAddPort = ipAddPort;

    Sirikata::Network::TCPResolver resolver(*ctx->ioService);   //a resolver can resolve a query into a series of endpoints.


    for (int s=0; s < STREAM_CRAQ_NUM_CONNECTIONS_SET; ++s)
    {
      IOStrand* tmpStrand = ctx->ioService->createStrand();
      mConnectionsStrands.push_back(tmpStrand);

      AsyncConnectionSet* tmpConn = new AsyncConnectionSet(ctx,                     //space context
                                                           mConnectionsStrands[s],  //strand for this connection to run on.
                                                           mStrand,                 //strand that the connection will return errors on
                                                           mResultsStrand,          //strand that the connection will return results on.
                                                           this,                    //will return errors to this master
                                                           mOSeg,
                                                           std::tr1::bind(&AsyncCraqSet::readyStateChanged,this,s));                   //will return results to this oseg

      mConnections.push_back(tmpConn);
    }

    Sirikata::Network::TCPSocket* passSocket;


    if (((int)ipAddPort.size()) >= STREAM_CRAQ_NUM_CONNECTIONS_SET)
    {
      //just assign each connection a separate router (in order that they were provided).
      for (int s = 0; s < STREAM_CRAQ_NUM_CONNECTIONS_SET; ++s)
      {
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[s].ipAdd.c_str(), ipAddPort[s].port.c_str());
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        passSocket   =  new Sirikata::Network::TCPSocket(*ctx->ioService);
        mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionSet::initialize, mConnections[s],passSocket,iterator));

      }
    }
    else
    {
      int whichRouterServingPrevious = -1;
      int whichRouterServing;
      double percentageConnectionsServed;

      boost::asio::ip::tcp::resolver::iterator iterator;

      for (int s=0; s < STREAM_CRAQ_NUM_CONNECTIONS_SET; ++s)
      {
        percentageConnectionsServed = ((double)s)/((double) STREAM_CRAQ_NUM_CONNECTIONS_SET);
        whichRouterServing = (int)(percentageConnectionsServed*((double)ipAddPort.size()));

        if (whichRouterServing  != whichRouterServingPrevious)
        {
          boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[whichRouterServing].ipAdd.c_str(), ipAddPort[whichRouterServing].port.c_str());


          iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.

          whichRouterServingPrevious = whichRouterServing;
        }

        passSocket   =  new Sirikata::Network::TCPSocket (*ctx->ioService);
        mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionSet::initialize, mConnections[s],passSocket,iterator));

      }
    }
  }


  void AsyncCraqSet::set(CraqDataSetGet dataToSet, uint64 track_num)
  {
    //force this to be a set message.
    dataToSet.messageType = CraqDataSetGet::SET;

    if (dataToSet.trackMessage)
    {
      dataToSet.trackingID = track_num;
    }
    pushQueue(dataToSet);


    int numTries = 0;
    while((mQueue.size() != 0) && (numTries < CRAQ_MAX_PUSH_SET))
    {
      ++numTries;
      int rand_connection = rand() % STREAM_CRAQ_NUM_CONNECTIONS_SET;
      checkConnections(rand_connection);
    }
  }


  /*
    Returns the size of the queue of operations to be processed
  */
  int AsyncCraqSet::queueSize()
  {
    return mQueue.size();
  }



  int AsyncCraqSet::numStillProcessing()
  {
    int returner = 0;

    for (int s=0; s < (int)mConnections.size(); ++s)
    {
      returner = returner + ((int)mConnections[s]->numStillProcessing());
    }
    return returner;
  }


  void AsyncCraqSet::erroredGetValue(CraqOperationResult* cor)
  {
#ifdef ASYNC_CRAQ_SET_DEBUG
    std::cout<<"\n\n\nError: should not receive an errored get value call in asyncCraqSet.cpp\n\n";
#endif
    assert(false);
  }


  void AsyncCraqSet::erroredSetValue(CraqOperationResult* errorRes)
  {
    if (errorRes->whichOperation == CraqOperationResult::GET)
    {
#ifdef ASYNC_CRAQ_SET_DEBUG
      std::cout<<"\n\nFailure: trying to perform a get operation from within asynccraqset\n\n";
#endif
      assert(false);
    }
    else
    {
      CraqDataSetGet cdSG(errorRes->objID,errorRes->servID,errorRes->tracking, CraqDataSetGet::SET);
      pushQueue(cdSG);
    }

    delete errorRes;
  }


  void AsyncCraqSet::poll()
  {
    return;
    int numTries = 0;
    while((mQueue.size()!= 0) && (numTries < CRAQ_MAX_PUSH_SET))
    {
      ++numTries;
      int rand_connection = rand() % STREAM_CRAQ_NUM_CONNECTIONS_SET;
      checkConnections(rand_connection);
    }
  }
  void AsyncCraqSet::readyStateChanged(int s) {
      mReadyConnections.push_back(s);
      while (checkConnections(s)) {
          if (mQueue.empty()) {
              break;
          }
      }
  }
  void AsyncCraqSet::pushQueue(const CraqDataSetGet&dataToSet){
      mQueue.push(dataToSet);//push and try to empty the queue
      while (!mReadyConnections.empty()) {
          unsigned int rnd=rand()%mReadyConnections.size();
          if (checkConnections(mReadyConnections[rnd])) {
              if (mQueue.empty()) {
                  break;
              }
          }else {
              mReadyConnections[rnd]=mReadyConnections.back();
              mReadyConnections.pop_back();
          }
      }
  }
  /*
    This function checks connection s to see if it needs a new socket or if it's ready to accept another query.
  */
  bool AsyncCraqSet::checkConnections(int s)
  {
    if (s>=(int)mConnections.size()) {
        return false;
    }
    assert (s<(int)mConnections.size());

    int numOperations = 0;


    if (mConnections[s]->ready() == AsyncConnectionSet::READY)
    {
      if (mQueue.size() != 0)
      {
        //need to put in another
        CraqDataSetGet cdSG = mQueue.front();
        mQueue.pop();

        ++numOperations;

        if (cdSG.messageType == CraqDataSetGet::GET)
        {
#ifdef ASYNC_CRAQ_SET_DEBUG
          std::cout<<"\n\nTrying to deal with get message in async craq set.cpp\n\n";
#endif
          assert(false);
        }
        else if (cdSG.messageType == CraqDataSetGet::SET)
        {
          //performing a set in connections.
          CraqObjectID tmpCraqID;
          memcpy(tmpCraqID.cdk, cdSG.dataKey, CRAQ_DATA_KEY_SIZE);
          mConnections[s]->setProcessing();
          mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionSet::setBound, mConnections[s], tmpCraqID, cdSG.dataKeyValue, cdSG.trackMessage, cdSG.trackingID));
        }
      }
    }
    else if (mConnections[s]->ready() == AsyncConnectionSet::NEED_NEW_SOCKET)
    {
      //need to create a new socket for the other
      reInitializeNode(s);
#ifdef ASYNC_CRAQ_SET_DEBUG
      std::cout<<"\n\nbftm debug: needed new connection.  How long will this take? \n\n";
#endif
    }
    else
    {
        return false;
    }
    return true;
  }




//means that we need to connect a new socket to the service.
void AsyncCraqSet::reInitializeNode(int s)
{
  if (s >= (int)mConnections.size())
    return;

  Sirikata::Network::TCPSocket* passSocket;

  Sirikata::Network::TCPResolver resolver(*ctx->ioService);   //a resolver can resolve a query into a series of endpoints.

  if ( ((int)mIpAddPort.size()) >= STREAM_CRAQ_NUM_CONNECTIONS_SET)
  {
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[s].ipAdd.c_str(), mIpAddPort[s].port.c_str());
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
    passSocket   =  new Sirikata::Network::TCPSocket(*ctx->ioService);

    mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionSet::initialize, mConnections[s],passSocket,iterator));
  }
  else
  {

    boost::asio::ip::tcp::resolver::iterator iterator;

    double percentageConnectionsServed = ((double)s)/((double) STREAM_CRAQ_NUM_CONNECTIONS_SET);
    int whichRouterServing = (int)(percentageConnectionsServed*((double)mIpAddPort.size()));

    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[whichRouterServing].ipAdd.c_str(), mIpAddPort[whichRouterServing].port.c_str());
    iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.


    passSocket   =  new Sirikata::Network::TCPSocket(*ctx->ioService);
    mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionSet::initialize, mConnections[s],passSocket,iterator));
  }
}

}//end namespace
