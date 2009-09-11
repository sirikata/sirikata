#include "asyncCraq.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <string.h>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include "asyncConnection.hpp"



namespace CBR
{

//nothing to destroy
AsyncCraq::~AsyncCraq()
{
  io_service.reset();  
  //  delete mSocket;
}


//nothing to initialize
AsyncCraq::AsyncCraq()
{
}


//void AsyncCraq::initialize(char* ipAdd, char* port)

void AsyncCraq::initialize(std::vector<CraqInitializeArgs> ipAddPort)
{

  mIpAddPort = ipAddPort;
  
  boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.

  mCurrentTrackNum = 10;
  
  AsyncConnection tmpConn;
  for (int s=0; s < CRAQ_NUM_CONNECTIONS; ++s)
  {
    mConnections.push_back(tmpConn);
  }

  boost::asio::ip::tcp::socket* passSocket;
  
  if (((int)ipAddPort.size()) >= CRAQ_NUM_CONNECTIONS)
  {
    //just assign each connection a separate router (in order that they were provided).
    for (int s = 0; s < CRAQ_NUM_CONNECTIONS; ++s)
    {
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[s].ipAdd.c_str(), ipAddPort[s].port.c_str());
      boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
      passSocket   =  new boost::asio::ip::tcp::socket(io_service);
      mConnections[s].initialize(passSocket,iterator); //note maybe can pass this by reference?
    }
  }
  else
  {
    int whichRouterServingPrevious = -1;
    int whichRouterServing;
    double percentageConnectionsServed;

    boost::asio::ip::tcp::resolver::iterator iterator;
    
    for (int s=0; s < CRAQ_NUM_CONNECTIONS; ++s)
    {
      percentageConnectionsServed = ((double)s)/((double) CRAQ_NUM_CONNECTIONS);
      whichRouterServing = (int)(percentageConnectionsServed*((double)ipAddPort.size()));

      if (whichRouterServing  != whichRouterServingPrevious)
      {
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddPort[whichRouterServing].ipAdd.c_str(), ipAddPort[whichRouterServing].port.c_str());
        iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        
        whichRouterServingPrevious = whichRouterServing;
      }
      passSocket   =  new boost::asio::ip::tcp::socket(io_service);
      mConnections[s].initialize(passSocket,iterator); //note maybe can pass this by reference?
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
int AsyncCraq::set(CraqDataSetGet dataToSet)
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
int AsyncCraq::queueSize()
{
  return mQueue.size();
}


int AsyncCraq::get(CraqDataSetGet dataToGet)
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
void AsyncCraq::tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults)
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
    mConnections[s].tick(tickedMessages_getResults,tickedMessages_errorResults,tickedMessages_trackedSetResults);

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
void AsyncCraq::processErrorResults(std::vector <CraqOperationResult*> & errorRes)
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
void AsyncCraq::checkConnections(int s)
{
  if (s >= (int)mConnections.size())
    return;
  


  int numOperations = 0;

  mConnections[s].ready();

  
  if (mConnections[s].ready() == AsyncConnection::READY)
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
        mConnections[s].get(cdSG.dataKey);
      }
      else if (cdSG.messageType == CraqDataSetGet::SET)
      {
        //performing a set in connections.
        mConnections[s].set(cdSG.dataKey, cdSG.dataKeyValue, cdSG.trackMessage, cdSG.trackingID);
      }
    }
  }
  else if (mConnections[s].ready() == AsyncConnection::NEED_NEW_SOCKET)
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

  boost::asio::ip::tcp::socket* passSocket;

  boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.
  
  if ( ((int)mIpAddPort.size()) >= CRAQ_NUM_CONNECTIONS)
  {
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[s].ipAdd.c_str(), mIpAddPort[s].port.c_str());
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
    passSocket   =  new boost::asio::ip::tcp::socket(io_service);
    mConnections[s].initialize(passSocket,iterator); //note maybe can pass this by reference?
  }
  else
  {
    
    boost::asio::ip::tcp::resolver::iterator iterator;

    double percentageConnectionsServed = ((double)s)/((double) CRAQ_NUM_CONNECTIONS);
    int whichRouterServing = (int)(percentageConnectionsServed*((double)mIpAddPort.size()));

    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[whichRouterServing].ipAdd.c_str(), mIpAddPort[whichRouterServing].port.c_str());
    iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        
    passSocket   =  new boost::asio::ip::tcp::socket(io_service);
    mConnections[s].initialize(passSocket,iterator); //note maybe can pass this by reference?
  }
}


}





