

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


//runs the
// void AsyncCraq::tick(std::vector<int> &serverIDs,  std::vector<char*> &objectIDs,std::vector<int>&trackedMessages)  //runs through one iteration of io_service.run_once.
// {
//   //  io_service.run_one();
//   //int numHandled = io_service.poll_one();
//   int numHandled = io_service.poll();
//   if (numHandled ==0)
//   {
//     io_service.reset();
//   }

  
//   serverIDs         =    serverLoc;
//   objectIDs         =     objectId;
//   trackedMessages   =     transIds;
  
//   serverLoc.clear();
//   objectId.clear();
//   transIds.clear();
  
// }


//void AsyncCraq::initialize(char* ipAdd, char* port)

void AsyncCraq::initialize(std::vector<CraqInitializeArgs> ipAddPort)
{

  mIpAddPort = ipAddPort;
  
  boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.

  mCurrentTrackNum = 0;
  
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


// void AsyncCraq::runTestOfAllConnections()
// {
//   std::cout<<"\n\nRunning Connection Test.\n\n";

//   std::vector<CraqOperationResult> tickedMessages;
//   std::vector<std::vector<CraqOperationResult> > allTickedMessages;
//   std::vector<CraqOperationResult> everyTickedMessage;

//   std::string tmper = "12345678901234567890123456789012";
//   CraqDataKey toSet;
//   strncpy(toSet,tmper.c_str(), tmper.size() + 1);

//   std::vector<int> setTo;
  
//   int numHandled;
//   std::vector<bool> setting;
  
//   //populating/initializing recording vectors
//   for (int s=0; s < mConnections.size(); ++s)
//   {
//     allTickedMessages.push_back(tickedMessages);
//     setting.push_back(false);
//   }

//   //running processing loop
//   for (int s=0; s < 50000; ++s) //number of time  iterations to go through.
//   {
//     for (int t=0; t < mConnections.size(); ++t)
//     {
//       tickedMessages.clear();

//       mConnections[t].tick(tickedMessages);

//       for (int u=0; u < tickedMessages.size(); ++u)
//       {
//         allTickedMessages[t].push_back(tickedMessages[u]);
//         everyTickedMessage.push_back(tickedMessages[u]);
//       }
//     }
    
//     numHandled = io_service.poll(); //running poll routine.

//     if (numHandled == 0)
//     {
//       io_service.reset();
//     }


//     for (int t=0; t<mConnections.size(); ++t)
//     {
//       if (mConnections[t].ready() == AsyncConnection::READY)
//       {
//         if (!setting[t])
//         {
//           //set a new value here.
//           mConnections[t].set(toSet,s);
//           setTo.push_back(s);
//           setting[t] = true;
//         }
//         else
//         {
//           //get a new value here.
//           mConnections[t].get(toSet);
//           setting[t] = false;
//         }
//       }
//     }
//   }  //end of controlling for loop

//   //printing results
//   std::vector<int> failures;
//   for (int s=0; s < mConnections.size(); ++s)
//   {
//     failures.push_back(0);
//   }
//   int totalEvents = 0;
//   int totalFailures = 0;

//   std::cout<<"\n\nThis is allTickedMessages.size:   "<<allTickedMessages.size()<<"\n\n";
  
//   for (int s= 0; s < allTickedMessages.size(); ++s)
//   {
//     totalEvents = totalEvents + allTickedMessages[s].size();
//     std::cout<<"\n\tSuccesses in "<<s<<":   "<<allTickedMessages[s].size()<<"\n";
//     for (int t= 0; t < allTickedMessages[s].size(); ++t)
//     {
//       if (! allTickedMessages[s][t].succeeded)
//       {
//         totalFailures = totalFailures + 1;
//         failures[s] = failures[s] + 1;
//       }
//     }
//   }
  
//   std::cout<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
//   std::cout<<"************************************\n";
//   std::cout<<"\tThis is the number of get+set events:     "<<totalEvents<<"\n";
//   std::cout<<"\tThis is the number of failures:           "<<totalFailures<<"\n";
// //   for (int s= 0; s<failures.size(); ++s)
// //   {
// //     std::cout<<"\t\t"<<s<<"\t"<<failures[s]<<"\n";
// //   }

// //   std::cout<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
// //   std::cout<<"************************************\n";
// //   std::cout<<"\n\nSET TO's\n\n";
// //   for (int s= 0; s < setTo.size(); ++s)
// //   {
// //     std::cout<<s<<"   "<<setTo[s]<<"\n";
// //   }


// //   std::cout<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
// //   std::cout<<"************************************\n";
// //   std::cout<<"\n\nREPORTS \n\n";

// //   for (unsigned int t=0; t < everyTickedMessage.size(); ++t)
// //   {
// //     if (everyTickedMessage[t].whichOperation == CraqOperationResult::GET)
// //     {
// //       std::cout<<"\nGET:  ";
// //       if (! everyTickedMessage[t].succeeded)
// //       {
// //         std::cout<<"FAILED \n\n";
// //       }
// //       else
// //       {
// //         std::cout<<"Succeeded.\n";
// //         everyTickedMessage[t].objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
// //         std::cout<<"\tobj_id   "<<everyTickedMessage[t].objID<<"\n";
// //         std::cout<<"\tservID   "<<everyTickedMessage[t].servID<<"\n\n";
// //       }
// //     }
// //     else
// //     {
// //       std::cout<<"\nSET:  ";
// //       if (! everyTickedMessage[t].succeeded)
// //       {
// //         std::cout<<"FAILED \n\n";
// //       }
// //       else
// //       {
// //         std::cout<<"Succeeded.\n";
// //         everyTickedMessage[t].objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
// //         std::cout<<"\tobj_id   "<<everyTickedMessage[t].objID<<"\n";
// //       }
// //     }
// //   }
// //  std::cout<<"\n\n\n\n\n\n";
  
// }



// void AsyncCraq::runTestOfConnection()
// {
//   std::cout<<"\n\nRunning Connection Test.\n\n";

//   std::vector<CraqOperationResult> tickedMessages;
//   std::vector<CraqOperationResult> allTickedMessages;
    
//   std::string tmper = "12345678901234567890123456789012";
//   CraqDataKey toSet;
//   strncpy(toSet,tmper.c_str(), tmper.size() + 1);

//   int numHandled;

//   bool setting = false;
//   for (int s=0; s < 50000; ++s)
//   {
//     mConnection.tick(tickedMessages);
        
//     numHandled = io_service.poll();

//     if (numHandled == 0)
//     {
//       io_service.reset();
//     }
//     if (tickedMessages.size() != 0)
//     {
//       std::cout<<"\n\n**********************\n";
//     }
//     for (unsigned int t=0; t < tickedMessages.size(); ++t)
//     {
//       if (tickedMessages[t].whichOperation == CraqOperationResult::GET)
//       {
//         std::cout<<"\nGET:  ";
//         if (! tickedMessages[t].succeeded)
//         {
//           std::cout<<"FAILED \n\n";
//         }
//         else
//         {
//           std::cout<<"Succeeded.\n";
//           tickedMessages[t].objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
//           std::cout<<"\tobj_id   "<<tickedMessages[t].objID<<"\n";
//           std::cout<<"\tservID   "<<tickedMessages[t].servID<<"\n\n";
//         }
//       }
//       else
//       {
//         std::cout<<"\nSET:  ";
//         if (! tickedMessages[t].succeeded)
//         {
//           std::cout<<"FAILED \n\n";
//         }
//         else
//         {
//           std::cout<<"Succeeded.\n";
//           tickedMessages[t].objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
//           std::cout<<"\tobj_id   "<<tickedMessages[t].objID<<"\n";
//         }
//       }
//       allTickedMessages.push_back(tickedMessages[t]);
//     }
    
//     if (mConnection.ready() == AsyncConnection::READY)
//     {
//       std::cout<<"\n\nConnection is ready  "<<s<<"\n\n";
      
//       if (!setting)
//       {
//         //set a new value here.
//         mConnection.set(toSet,s);
//         setting = true;
//       }
//       else
//       {
//         //get a new value here.
//         mConnection.get(toSet);
//         setting = false;
//       }
//     }
//   }  //end of controlling for loop

//   std::cout<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
//   std::cout<<"************************************\n";
//   std::cout<<"\tThis is the number of get+set events:     "<<allTickedMessages.size()<<"\n";
// }



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
//void AsyncCraq::tick(std::vector<int> &serverIDs, std::vector<CraqObjectID> &objectIds,std::vector<int>&trackedMessages)

void AsyncCraq::tick(std::vector<CraqOperationResult>&getResults, std::vector<CraqOperationResult>&trackedSetResults)
{
  int numHandled = io_service.poll();

  if (numHandled == 0)
  {
    io_service.reset();
  }

  std::vector<CraqOperationResult> tickedMessages_getResults;
  std::vector<CraqOperationResult> tickedMessages_errorResults;
  std::vector<CraqOperationResult> tickedMessages_trackedSetResults;
  
  for (int s=0; s < (int)mConnections.size(); ++s)
  {
    tickedMessages_getResults.clear();
    tickedMessages_errorResults.clear();
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
void AsyncCraq::processErrorResults(std::vector <CraqOperationResult> & errorRes)
{
  for (int s=0;s < (int)errorRes.size(); ++s)
  {
    if (errorRes[s].whichOperation == CraqOperationResult::GET)
    {
      CraqDataSetGet cdSG(errorRes[s].objID,errorRes[s].servID,errorRes[s].tracking, CraqDataSetGet::GET);
      mQueue.push(cdSG);
    }
    else
    {
      CraqDataSetGet cdSG(errorRes[s].objID,errorRes[s].servID,errorRes[s].tracking, CraqDataSetGet::SET);
      mQueue.push(cdSG);      
    }
  }
}






/*
  This function checks connection s to see if it needs a new socket or if it's ready to accept another query.
*/
void AsyncCraq::checkConnections(int s)
{
  if (s >= (int)mConnections.size())
    return;
  
  if (mConnections[s].ready() == AsyncConnection::READY)
  {
    if (mQueue.size() != 0)
    {
      //need to put in another
      CraqDataSetGet cdSG = mQueue.front();
      mQueue.pop();

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







