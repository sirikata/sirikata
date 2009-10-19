#include "asyncCraqSet.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <string.h>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/asio.hpp>

#include "../asyncCraqUtil.hpp"
#include "asyncConnectionSet.hpp"


namespace CBR
{


//nothing to destroy
AsyncCraqSet::~AsyncCraqSet()
{
  io_service.reset();  
  //  delete mSocket;

  for (int s= 0;s < (int) mConnections.size(); ++s)
  {
    delete mConnections[s];
  }
  
}


//nothing to initialize
AsyncCraqSet::AsyncCraqSet()
{
}



void AsyncCraqSet::initialize(std::vector<CraqInitializeArgs> ipAddPort)
{

  std::cout<<"\n\nbftm debug: asynccraqtwo initialize\n\n";
  
  mIpAddPort = ipAddPort;
  
  boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.

  mCurrentTrackNum = 10;
  
  //  AsyncConnectionTwo tmpConn;

  for (int s=0; s < CRAQ_NUM_CONNECTIONS_SET; ++s)
  {
    AsyncConnectionSet* tmpConn = new AsyncConnectionSet;
    mConnections.push_back(tmpConn);
  }

  boost::asio::ip::tcp::socket* passSocket;
  
  if (((int)ipAddPort.size()) >= CRAQ_NUM_CONNECTIONS_SET)
  {
    //just assign each connection a separate router (in order that they were provided).
    for (int s = 0; s < CRAQ_NUM_CONNECTIONS_SET; ++s)
    {
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[s].ipAdd.c_str(), ipAddPort[s].port.c_str());
      boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
      passSocket   =  new boost::asio::ip::tcp::socket(io_service);
      mConnections[s]->initialize(passSocket,iterator); //note maybe can pass this by reference?
    }
  }
  else
  {
    int whichRouterServingPrevious = -1;
    int whichRouterServing;
    double percentageConnectionsServed;

    boost::asio::ip::tcp::resolver::iterator iterator;
    
    for (int s=0; s < CRAQ_NUM_CONNECTIONS_SET; ++s)
    {
      percentageConnectionsServed = ((double)s)/((double) CRAQ_NUM_CONNECTIONS_SET);
      whichRouterServing = (int)(percentageConnectionsServed*((double)ipAddPort.size()));

      //      whichRouterServing = 0; //bftm debug

      
      if (whichRouterServing  != whichRouterServingPrevious)
      {
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[whichRouterServing].ipAdd.c_str(), ipAddPort[whichRouterServing].port.c_str());
      
        
        iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        
        whichRouterServingPrevious = whichRouterServing;
      }
      passSocket   =  new boost::asio::ip::tcp::socket(io_service);
      mConnections[s]->initialize(passSocket,iterator); //note maybe can pass this by reference?
    }
  }
  
}

void AsyncCraqSet::runTestOfAllConnections()
{
  //temporarily empty.
}

void AsyncCraqSet::runTestOfConnection()
{
  //temporarily empty.
}


//assumes that we're already connected.
int AsyncCraqSet::set(CraqDataSetGet dataToSet)
{
  //force this to be a set message.
  dataToSet.messageType = CraqDataSetGet::SET;

  
  if (dataToSet.trackMessage)
  {
    dataToSet.trackingID = mCurrentTrackNum;
    ++mCurrentTrackNum;

    mQueue.push(dataToSet);
    return mCurrentTrackNum -1;
  }
  
  //we got all the way through without finding a ready connection.  Need to add query to queue.
  mQueue.push(dataToSet);
  return 0;
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




int AsyncCraqSet::get(CraqDataSetGet dataToGet)
{
  
  //force this to be a set message.
  dataToGet.messageType = CraqDataSetGet::GET;
  //we got all the way through without finding a ready connection.  Need to add query to queue.
  mQueue.push(dataToGet);
  
  return 0;
}



/*
  tick processes 
  tick returns all the 
*/
void AsyncCraqSet::tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults)
{
  int numHandled = io_service.poll();

  if (numHandled == 0)
  {
    io_service.reset();
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


    //can optimize by setting separate tracks for 
    mConnections[s]->tick(tickedMessages_getResults,tickedMessages_errorResults,tickedMessages_trackedSetResults);

    for (int t= 0; t < (int) tickedMessages_getResults.size(); ++t)
    {
      getResults.push_back(tickedMessages_getResults[t]);
    }
    for (int t= 0; t < (int) tickedMessages_trackedSetResults.size(); ++t)
    {
      trackedSetResults.push_back(tickedMessages_trackedSetResults[t]);
    }

    processErrorResults(tickedMessages_errorResults);
    
    checkConnections(s); //checks whether connection is ready for an additional query and also checks if it needs a new socket.
  }
}


/*
  errorRes is full of results that went bad from a craq connection.  In the future, we may do something more intelligent, but for now, we are just going to put the request back in mQueue
*/
void AsyncCraqSet::processErrorResults(std::vector <CraqOperationResult*> & errorRes)
{
  for (int s=0;s < (int)errorRes.size(); ++s)
  {
    if (errorRes[s]->whichOperation == CraqOperationResult::GET)
    {
      CraqDataSetGet cdSG(errorRes[s]->objID,errorRes[s]->servID,errorRes[s]->tracking, CraqDataSetGet::GET);
      mQueue.push(cdSG);
    }
    else
    {
      CraqDataSetGet cdSG(errorRes[s]->objID,errorRes[s]->servID,errorRes[s]->tracking, CraqDataSetGet::SET);
      mQueue.push(cdSG);      
    }

    delete errorRes[s];
    
  }
}


/*
  This function checks connection s to see if it needs a new socket or if it's ready to accept another query.
*/
void AsyncCraqSet::checkConnections(int s)
{
  if (s >= (int)mConnections.size())
    return;
  
  int numOperations = 0;

  //  mConnections[s].ready();

  
  //  if (mConnections[s].ready() == AsyncConnectionTwo::READY)
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
        //perform a get in  connections.
        mConnections[s]->get(cdSG.dataKey);
      }
      else if (cdSG.messageType == CraqDataSetGet::SET)
      {
        //performing a set in connections.
        mConnections[s]->set(cdSG.dataKey, cdSG.dataKeyValue, cdSG.trackMessage, cdSG.trackingID);
      }
    }
  }
  //  else if (mConnections[s].ready() == AsyncConnectionTwo::NEED_NEW_SOCKET)
  else if (mConnections[s]->ready() == AsyncConnectionSet::NEED_NEW_SOCKET)
  {
    //need to create a new socket for the other
    reInitializeNode(s);
    std::cout<<"\n\nbftm debug: needed new connection.  How long will this take? \n\n";
  }
}




//means that we need to connect a new socket to the service.
void AsyncCraqSet::reInitializeNode(int s)
{
  if (s >= (int)mConnections.size())
    return;

  boost::asio::ip::tcp::socket* passSocket;

  boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.
  
  if ( ((int)mIpAddPort.size()) >= CRAQ_NUM_CONNECTIONS_SET)
  {
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[s].ipAdd.c_str(), mIpAddPort[s].port.c_str());
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
    passSocket   =  new boost::asio::ip::tcp::socket(io_service);
    mConnections[s]->initialize(passSocket,iterator); //note maybe can pass this by reference?
  }
  else
  {
    
    boost::asio::ip::tcp::resolver::iterator iterator;

    double percentageConnectionsServed = ((double)s)/((double) CRAQ_NUM_CONNECTIONS_SET);
    int whichRouterServing = (int)(percentageConnectionsServed*((double)mIpAddPort.size()));

    //    whichRouterServing = 0; //bftm debug
    
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[whichRouterServing].ipAdd.c_str(), mIpAddPort[whichRouterServing].port.c_str());
    iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        
    passSocket   =  new boost::asio::ip::tcp::socket(io_service);
    mConnections[s]->initialize(passSocket,iterator); //note maybe can pass this by reference?
  }
}

}//end namespace



