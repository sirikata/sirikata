#include "asyncCraqGet.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <string.h>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/asio.hpp>


#include "asyncConnectionGet.hpp"
#include "../asyncCraqUtil.hpp"
#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include <sirikata/network/Asio.hpp>

#define CRAQ_MAX_PUSH_GET 10

namespace CBR
{

  void AsyncCraqGet::stop()
  {
    std::cout<<"\n\nReceived a stop in async craq get\n";
    for (int s=0; s < (int) mConnections.size(); ++s)
      mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionGet::stop,mConnections[s]));

       
    PollingService::stop();

  }
  
  AsyncCraqGet::~AsyncCraqGet()
  {
    for (int s= 0;s < (int) mConnections.size(); ++s)
    {
      delete mConnections[s];
      delete mConnectionsStrands[s];
    }
    mConnections.clear();
    mConnectionsStrands.clear();
    
  }

  int AsyncCraqGet::getRespCount()
  {
    int returner = 0;
    for (int s=0; s < (int) mConnections.size(); ++s)
    {
      returner += mConnections[s]->getRespCount();
    }
    return returner;
  }
  

  //nothing to initialize
  AsyncCraqGet::AsyncCraqGet(SpaceContext* con, IOStrand* strand_this_runs_on, IOStrand* strand_to_post_results_to, ObjectSegmentation* parent_oseg_called)
    : PollingService(strand_this_runs_on),
      ctx(con),
      mStrand(strand_this_runs_on),
      mResultsStrand(strand_to_post_results_to),
      mOSeg(parent_oseg_called)
  {
  }
  
  int AsyncCraqGet::runReQuery()
  {
    int returner = 0;
    for (int s=0; s < (int) mConnections.size(); ++s)
    {
      if (mConnections[s]->ready() == AsyncConnectionGet::READY)
      {
        returner += mConnections[s]->runReQuery();
      }
    }
    return returner;
  }

void AsyncCraqGet::initialize(std::vector<CraqInitializeArgs> ipAddPort)
{
  mIpAddPort = ipAddPort;

  Sirikata::Network::TCPResolver resolver((*ctx->ioService)); //a resolver can resolve a query into a series of endpoints.

  for (int s=0; s < STREAM_CRAQ_NUM_CONNECTIONS_GET; ++s)
  {

    IOStrand* tmpStrand         = ctx->ioService->createStrand();
    mConnectionsStrands.push_back(tmpStrand);

    
    AsyncConnectionGet* tmpConn = new AsyncConnectionGet (ctx,             //space context
                                                          tmpStrand,       //strand specific for this connection
                                                          mStrand,         //use asyncCraqGet's strand to post errors back on.
                                                          mResultsStrand,  //strand to post results to.
                                                          this,            //tells connection to post errors back to this.
                                                          mOSeg            //tells connection to post results back to mOSeg.
                                                          );
    mConnections.push_back(tmpConn);
  }

  
  Sirikata::Network::TCPSocket* passSocket;

  
  if (((int)ipAddPort.size()) >= STREAM_CRAQ_NUM_CONNECTIONS_GET)
  {
    //just assign each connection a separate router (in order that they were provided).
    for (int s = 0; s < STREAM_CRAQ_NUM_CONNECTIONS_GET; ++s)
    {
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[s].ipAdd.c_str(), ipAddPort[s].port.c_str());
      boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
      
      passSocket   = new Sirikata::Network::TCPSocket((*ctx->ioService));
      mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionGet::initialize, mConnections[s],passSocket, iterator));
    }
  }
  else
  {
    int whichRouterServingPrevious = -1;
    int whichRouterServing;
    double percentageConnectionsServed;

    boost::asio::ip::tcp::resolver::iterator iterator;
    
    for (int s=0; s < STREAM_CRAQ_NUM_CONNECTIONS_GET; ++s)
    {
      percentageConnectionsServed = ((double)s)/((double) STREAM_CRAQ_NUM_CONNECTIONS_GET);
      whichRouterServing = (int)(percentageConnectionsServed*((double)ipAddPort.size()));

      if (whichRouterServing  != whichRouterServingPrevious)
      {
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[whichRouterServing].ipAdd.c_str(), ipAddPort[whichRouterServing].port.c_str());
      
        
        iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        
        whichRouterServingPrevious = whichRouterServing;
      }

      passSocket   = new Sirikata::Network::TCPSocket((*ctx->ioService));
      mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionGet::initialize, mConnections[s],passSocket, iterator));
    }
  }
  
}


/*
  Returns the size of the queue of operations to be processed
*/
int AsyncCraqGet::queueSize()
{
  return mQueue.size();
}


  int AsyncCraqGet::numStillProcessing()
  {
    int returner = 0;
    
    for (int s=0; s < (int)mConnections.size(); ++s)
    {
      returner = returner + ((int)mConnections[s]->numStillProcessing());
    }

    return returner;
  }

  void AsyncCraqGet::poll()
  {
    int numTries = 0;
    while((mQueue.size() != 0) && (numTries < CRAQ_MAX_PUSH_GET))
    {
      ++numTries;
      int rand_connection = rand() % STREAM_CRAQ_NUM_CONNECTIONS_GET;
      checkConnections(rand_connection);
    }
  }
  
  void AsyncCraqGet::get(const CraqDataSetGet& dataToGet)
  {
    CraqDataSetGet* cdQuery = new CraqDataSetGet(dataToGet.dataKey,dataToGet.dataKeyValue,dataToGet.trackMessage,CraqDataSetGet::GET);
    mQueue.push(cdQuery);

    int numTries = 0;
    while((mQueue.size()!= 0) && (numTries < CRAQ_MAX_PUSH_GET))
    {
      ++numTries;
      int rand_connection = rand() % STREAM_CRAQ_NUM_CONNECTIONS_GET;
      checkConnections(rand_connection);
    }
  }


void AsyncCraqGet::erroredSetValue(CraqOperationResult* cor)
{
  std::cout<<"\n\nShould not received an errored set inside of asyncCraqGet.cpp\n\n";
  assert(false);
}


  //will be posted to from different connections.  am responsible for deleting.
  void AsyncCraqGet::erroredGetValue(CraqOperationResult* errorRes)
  {
    if (errorRes->whichOperation == CraqOperationResult::GET)
    {
      CraqDataSetGet* cdSG = new CraqDataSetGet (errorRes->objID,errorRes->servID,errorRes->tracking, CraqDataSetGet::GET);
      mQueue.push(cdSG);
    }
    else
    {
      std::cout<<"\n\nShould never be receiving an error result for a set in asyncCraqGet.cpp\n\n";
      assert(false);
    }

    delete errorRes;
  }


/*
  This function checks connection s to see if it needs a new socket or if it's ready to accept another query.
*/
void AsyncCraqGet::checkConnections(int s)
{
  if (s >= (int)mConnections.size())
    return;
  
  int numOperations = 0;

  if (mConnections[s]->ready() == AsyncConnectionGet::READY)
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
        CraqObjectID tmpCraqID;
        memcpy(tmpCraqID.cdk, cdSG->dataKey, CRAQ_DATA_KEY_SIZE);
        mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionGet::getBound,mConnections[s],tmpCraqID));

      }
      else if (cdSG->messageType == CraqDataSetGet::SET)
      {
        std::cout<<"\n\nShould be incapable of performing a set from asyncCraqGet\n\n";
        assert(false);
      }
      delete cdSG;
    }
  }
  else if (mConnections[s]->ready() == AsyncConnectionGet::NEED_NEW_SOCKET)
  {
    //need to create a new socket for the other
    reInitializeNode(s);
    std::cout<<"\n\nbftm debug: needed new connection.  How long will this take? \n\n";
  }
}

//means that we need to connect a new socket to the service.
void AsyncCraqGet::reInitializeNode(int s)
{
  if (s >= (int)mConnections.size())
    return;


  Sirikata::Network::TCPSocket* passSocket;
  Sirikata::Network::TCPResolver resolver(*ctx->ioService);   //a resolver can resolve a query into a series of endpoints.
  
  if ( ((int)mIpAddPort.size()) >= STREAM_CRAQ_NUM_CONNECTIONS_GET)
  {
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[s].ipAdd.c_str(), mIpAddPort[s].port.c_str());
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.

    passSocket   =  new Sirikata::Network::TCPSocket(*ctx->ioService);

    mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionGet::initialize, mConnections[s],passSocket, iterator));
  }
  else
  {
    
    boost::asio::ip::tcp::resolver::iterator iterator;

    double percentageConnectionsServed = ((double)s)/((double) STREAM_CRAQ_NUM_CONNECTIONS_GET);
    int whichRouterServing = (int)(percentageConnectionsServed*((double)mIpAddPort.size()));

    //    whichRouterServing = 0; //bftm debug
    
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[whichRouterServing].ipAdd.c_str(), mIpAddPort[whichRouterServing].port.c_str());
    iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        
    passSocket   =  new Sirikata::Network::TCPSocket(*ctx->ioService);
    
    mConnectionsStrands[s]->post(std::tr1::bind(&AsyncConnectionGet::initialize, mConnections[s],passSocket, iterator));
  }
}



} //end namespace

