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

namespace CBR
{


//nothing to destroy
AsyncCraqGet::~AsyncCraqGet()
{
  io_service.reset();  
  //  delete mSocket;

  for (int s= 0;s < (int) mConnections.size(); ++s)
  {
    delete mConnections[s];
  }
  
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
AsyncCraqGet::AsyncCraqGet(SpaceContext* con, IOStrand* str)
  : ctx(con),
    mStrand(str)
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
  
  boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.

  mCurrentTrackNum = 10;
  
  //  AsyncConnectionTwo tmpConn;

  for (int s=0; s < STREAM_CRAQ_NUM_CONNECTIONS_GET; ++s)
  {
    AsyncConnectionGet* tmpConn = new AsyncConnectionGet (ctx,mStrand, &io_service);
    mConnections.push_back(tmpConn);
  }

  boost::asio::ip::tcp::socket* passSocket;
  
  if (((int)ipAddPort.size()) >= STREAM_CRAQ_NUM_CONNECTIONS_GET)
  {
    //just assign each connection a separate router (in order that they were provided).
    for (int s = 0; s < STREAM_CRAQ_NUM_CONNECTIONS_GET; ++s)
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
    
    for (int s=0; s < STREAM_CRAQ_NUM_CONNECTIONS_GET; ++s)
    {
      percentageConnectionsServed = ((double)s)/((double) STREAM_CRAQ_NUM_CONNECTIONS_GET);
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

void AsyncCraqGet::runTestOfAllConnections()
{
  //temporarily empty.
}

void AsyncCraqGet::runTestOfConnection()
{
  //temporarily empty.
}


//assumes that we're already connected.
int AsyncCraqGet::set(const CraqDataSetGet& dataToSet)
{
  std::cout<<"\n\nShould not be calling set from inside of asyncCraqGet\n\n";
  assert(false);  
  
  //force this to be a set message.

  CraqDataSetGet* cdQuery = new CraqDataSetGet(dataToSet.dataKey,dataToSet.dataKeyValue,dataToSet.trackMessage,CraqDataSetGet::SET);

  if (dataToSet.trackMessage)
  {
    cdQuery->trackingID = mCurrentTrackNum;
    ++mCurrentTrackNum;

    mQueue.push(cdQuery);
    straightPoll();
    return mCurrentTrackNum - 1;
      
  }
  
  mQueue.push(cdQuery);
  straightPoll();
  checkConnections(0);

  return 0;
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




int AsyncCraqGet::getMulti(CraqDataSetGet& dataToGet)
{
  CraqDataSetGet* cdQuery = new CraqDataSetGet(dataToGet.dataKey,dataToGet.dataKeyValue,dataToGet.trackMessage,CraqDataSetGet::GET);

  mQueue.push(cdQuery);

  int rand_connection = rand() % STREAM_CRAQ_NUM_CONNECTIONS_GET;
  checkConnectionsMulti(rand_connection);
  return 0;
}

  
int AsyncCraqGet::get(const CraqDataSetGet& dataToGet)
{
  CraqDataSetGet* cdQuery = new CraqDataSetGet(dataToGet.dataKey,dataToGet.dataKeyValue,dataToGet.trackMessage,CraqDataSetGet::GET);
  mQueue.push(cdQuery);

  int rand_connection = rand() % STREAM_CRAQ_NUM_CONNECTIONS_GET;
  checkConnections(rand_connection);
  return 0;
}


void AsyncCraqGet::straightPoll()
{
  int numHandled = io_service.poll();
  
  if (numHandled == 0)
    io_service.reset();
}


/*
  tick processes 
  tick returns all the 
*/
void AsyncCraqGet::tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults)
{

  straightPoll();
  
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


    getResults.insert(getResults.end(), tickedMessages_getResults.begin(), tickedMessages_getResults.end());
    trackedSetResults.insert(trackedSetResults.end(),tickedMessages_trackedSetResults.begin(), tickedMessages_trackedSetResults.end());

    processErrorResults(tickedMessages_errorResults);
    
    checkConnections(s); //checks whether connection is ready for an additional query and also checks if it needs a new socket.
  }
}






/*
  tick processes 
  tick returns all the 
*/
void AsyncCraqGet::tickMulti(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults)
{

  straightPoll();
  
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

    getResults.insert(getResults.end(), tickedMessages_getResults.begin(), tickedMessages_getResults.end());
    trackedSetResults.insert(trackedSetResults.end(), tickedMessages_trackedSetResults.begin(), tickedMessages_trackedSetResults.end());

    
    processErrorResults(tickedMessages_errorResults);
    
    checkConnectionsMulti(s); //checks whether connection is ready for an additional query and also checks if it needs a new socket.
  }
}








/*
  errorRes is full of results that went bad from a craq connection.  In the future, we may do something more intelligent, but for now, we are just going to put the request back in mQueue
*/
void AsyncCraqGet::processErrorResults(std::vector <CraqOperationResult*> & errorRes)
{
  for (int s=0;s < (int)errorRes.size(); ++s)
  {
    if (errorRes[s]->whichOperation == CraqOperationResult::GET)
    {
      CraqDataSetGet* cdSG = new CraqDataSetGet (errorRes[s]->objID,errorRes[s]->servID,errorRes[s]->tracking, CraqDataSetGet::GET);
      mQueue.push(cdSG);
    }
    else
    {
      CraqDataSetGet* cdSG = new CraqDataSetGet (errorRes[s]->objID,errorRes[s]->servID,errorRes[s]->tracking, CraqDataSetGet::SET);
      mQueue.push(cdSG);      
    }

    delete errorRes[s];
    
  }
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
  else if (mConnections[s]->ready() == AsyncConnectionGet::NEED_NEW_SOCKET)
  {
    //need to create a new socket for the other
    reInitializeNode(s);
    std::cout<<"\n\nbftm debug: needed new connection.  How long will this take? \n\n";
  }
}



void AsyncCraqGet::checkConnectionsMulti(int s)
{
  if (s >= (int)mConnections.size())
    return;
  
  int numOperations = 0;

  
  //  if (mConnections[s].ready() == AsyncConnectionTwo::READY)
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
        mConnections[s]->getMulti(cdSG->dataKey);
      }
      else if (cdSG->messageType == CraqDataSetGet::SET)
      {
        //performing a set in connections.
        mConnections[s]->set(cdSG->dataKey, cdSG->dataKeyValue, cdSG->trackMessage, cdSG->trackingID);
      }

      delete cdSG;
    }
  }
  //  else if (mConnections[s].ready() == AsyncConnectionTwo::NEED_NEW_SOCKET)
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

  boost::asio::ip::tcp::socket* passSocket;

  boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.
  
  if ( ((int)mIpAddPort.size()) >= STREAM_CRAQ_NUM_CONNECTIONS_GET)
  {
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[s].ipAdd.c_str(), mIpAddPort[s].port.c_str());
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
    passSocket   =  new boost::asio::ip::tcp::socket(io_service);
    mConnections[s]->initialize(passSocket,iterator); //note maybe can pass this by reference?
  }
  else
  {
    
    boost::asio::ip::tcp::resolver::iterator iterator;

    double percentageConnectionsServed = ((double)s)/((double) STREAM_CRAQ_NUM_CONNECTIONS_GET);
    int whichRouterServing = (int)(percentageConnectionsServed*((double)mIpAddPort.size()));

    //    whichRouterServing = 0; //bftm debug
    
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), mIpAddPort[whichRouterServing].ipAdd.c_str(), mIpAddPort[whichRouterServing].port.c_str());
    iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
        
    passSocket   =  new boost::asio::ip::tcp::socket(io_service);
    mConnections[s]->initialize(passSocket,iterator); //note maybe can pass this by reference?
  }
}



} //end namespace

