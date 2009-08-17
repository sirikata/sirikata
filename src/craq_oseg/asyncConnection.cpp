#include "asyncConnection.hpp"
#include <iostream>
#include <boost/bind.hpp>


namespace CBR
{


AsyncConnection::AsyncConnection()
{
  mReady = NEED_NEW_SOCKET;
  mTrackNumber = 1;
}

AsyncConnection::~AsyncConnection()
{
  if (! NEED_NEW_SOCKET)
  {
    mSocket->close();
    delete mSocket;
  }
}


AsyncConnection::ConnectionState AsyncConnection::ready()
{
  return mReady;
}


//  void tick(std::vector<int> &serverIDs,  std::vector<CraqObjectID> &objectIds,std::vector<int>&trackedMessages);  //runs through one iteration of io_service.run_once.
//void AsyncConnection::tick(std::vector<int> &serverIDs,  std::vector<CraqObjectID> &objectIds,std::vector<int>&trackedMessages)
  
//void AsyncConnection::tick(std::vector<CraqOperationResult>&opResults)
void AsyncConnection::tick(std::vector<CraqOperationResult>&opResults_get, std::vector<CraqOperationResult>&opResults_error, std::vector<CraqOperationResult>&opResults_trackedSets)
{
  opResults_get = mOperationResultVector;
  mOperationResultVector.clear();

  opResults_error = mOperationResultErrorVector;
  mOperationResultErrorVector.clear();

  opResults_trackedSets = mOperationResultTrackedSetsVector;
  mOperationResultTrackedSetsVector.clear();


  /*
  serverIDs = mServerIDs;
  objectIds = mCraqObjectIDs;
  trackedMessages = mTrackedMessages;

  mServerIDs.clear();
  mCraqObjectIDs.clear();
  trackedMessages.clear();
  */
}


void AsyncConnection::initialize( boost::asio::ip::tcp::socket* socket,    boost::asio::ip::tcp::resolver::iterator it)
{
  mSocket = socket;
  mReady = PROCESSING;
  //need to run connection routine.
  mSocket->async_connect(*it, boost::bind(&AsyncConnection::connect_handler,this,_1));  //using that tcp socket for an asynchronous connection.
  
}


//connection handler.
void AsyncConnection::connect_handler(const boost::system::error_code& error)
{
  if (error)
  {
    mSocket->close();
    delete mSocket;
    mReady = NEED_NEW_SOCKET;

    std::cout<<"\n\nError in connection\n\n";
    exit(1);
    
    return;
  }

  std::cout<<"\n\nbftm debug: asyncConnection: connected\n\n";
  
  mReady = READY;
}

bool AsyncConnection::set(CraqDataKey dataToSet, int  dataToSetTo, bool track, int trackNum)
{
  if (mReady != READY)
  {
    std::cout<<"\n\nbftm debug:  huge error\n\n";
    
    return false;
  }

  mTracking          =         track;
  mTrackNumber       =      trackNum;
  currentlySettingTo =   dataToSetTo;

  if (track)
  {
    std::cout<<"\n\nbftm debug: In set of asyncConnection.cpp.  Got a positive tracking.\n\n";
  }
  
  mReady = PROCESSING;
  std::string tmpString = dataToSet;
  strncpy(currentlySearchingFor,tmpString.c_str(),tmpString.size() + 1);
  //  currentlySearchingFor = dataToSet;


  //creating stream buffer
  boost::asio::streambuf* sBuff = new boost::asio::streambuf;

  //creating a read-callback.
  boost::asio::async_read_until((*mSocket),
                                (*sBuff),//     boost::asio::buffer(mReadData,CRAQ_DATA_RESPONSE_SIZE),
                                boost::regex("\r\n"),
                                boost::bind(&AsyncConnection::read_handler_set,this,_1,_2,sBuff));


  //generating the query to write.
  std::string query;
  query.append(CRAQ_DATA_SET_PREFIX);
  query.append(dataToSet); //this is the re

  query.append(CRAQ_DATA_TO_SET_SIZE);
  query.append(CRAQ_DATA_SET_END_LINE);

  //convert from integer to string.
  std::stringstream ss;
  ss << dataToSetTo;
  std::string tmper = ss.str();
  for (int s=0; s< CRAQ_SERVER_SIZE - tmper.size(); ++s)
  {
    query.append("0");
  }
    
  query.append(tmper);
  //  query.append(dataToSet);
  query.append(CRAQ_TO_SET_SUFFIX);
  
  query.append(CRAQ_DATA_SET_END_LINE);
  CraqDataSetQuery dsQuery;    
  strncpy(dsQuery,query.c_str(), CRAQ_DATA_SET_SIZE);

  //  std::cout<<"\n\nbftm debug inside of asyncCraq.cpp in set.  This is the query:  "<<dsQuery<<"\n\n";

  //creating callback for write function
  mSocket->async_write_some(boost::asio::buffer(dsQuery,CRAQ_DATA_SET_SIZE -2),
                            boost::bind(&AsyncConnection::write_some_handler_set,this,_1,_2));
    
  return true;
}



void AsyncConnection::write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error)
  {
    //had trouble with this write.
    std::cout<<"\n\n\nHAD LOTS OF PROBLEMS WITH THIS WRITE\n\n\n";
    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;
    
    CraqOperationResult tmper(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::SET,mTracking); //false means that it didn't succeed.
    mOperationResultErrorVector.push_back(tmper);
  }
}


void AsyncConnection::read_handler_set ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff)
{
  //read strings from set;
  std::istream is(sBuff);
  std::string line = "";
  std::string tmpLine;
  
  is >> tmpLine;
  
  while (tmpLine.size() != 0)
  {
    line.append(tmpLine);
    tmpLine = "";
    is >> tmpLine;
  }
  
  //process this line.
  if (line.find("STORED") != std::string::npos)
  {

    if (mTracking)
    {
      //means that we need to save this

      std::cout<<"\n\nbftm debug:  inside of read_handler_set of asyncConnection.cpp.  Got a request to track.\n\n";
      
      CraqOperationResult tmper(currentlySettingTo,currentlySearchingFor, mTrackNumber,true,CraqOperationResult::SET, mTracking);      
      mOperationResultTrackedSetsVector.push_back(tmper);
    }
    
    mReady  = READY;
  }
  else
  {
    //something besides stored was returned...indicates an error.
    std::cout<<"\n\nbftm debug: There was an error in asyncConnection.cpp under read_handler_set:\n\n";
    std::cout<<"\n\nThis is line:   "<<line<<"\n\n";

    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;
    
    CraqOperationResult tmper(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::SET,mTracking); //false means that it didn't succeed.
    mOperationResultErrorVector.push_back(tmper);
  }
  //deletes buffer
  delete sBuff;
}



//datakey should have a null termination character.
bool AsyncConnection::get(CraqDataKey dataToGet)
{
  if (mReady != READY)
  {
    return false;
  }

  mReady = PROCESSING;
  std::string tmpString = dataToGet;
  strncpy(currentlySearchingFor,tmpString.c_str(),tmpString.size() + 1);
  //  currentlySearchingFor = dataToSet;

  mReady = PROCESSING;

  //  std::cout<<"\n\nbftm debug: in get of asyncConnection.cpp\n\n";
  
  boost::asio::streambuf * sBuff = new boost::asio::streambuf;

  //sets read handler
  boost::asio::async_read_until((*mSocket),
                                (*sBuff),
                                boost::regex("YY\r\n"),
                                boost::bind(&AsyncConnection::read_handler_get,this,_1,_2,sBuff));

  //crafts query
  std::string query;
  query.append(CRAQ_DATA_KEY_QUERY_PREFIX);
  query.append(dataToGet); //this is the re
  query.append(CRAQ_DATA_KEY_QUERY_SUFFIX);

  CraqDataKeyQuery dkQuery;
  strncpy(dkQuery,query.c_str(), CRAQ_DATA_KEY_QUERY_SIZE);


  //sets write handler
  mSocket->async_write_some(boost::asio::buffer(dkQuery,CRAQ_DATA_KEY_QUERY_SIZE-1),
                            boost::bind(&AsyncConnection::write_some_handler_get,this,_1,_2));
  
  return true;
}


void AsyncConnection::write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error)
  {
    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;
    
    CraqOperationResult tmper(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
    mOperationResultErrorVector.push_back(tmper);
    
    //    load error vector into both.

    //had trouble with this write.
    std::cout<<"\n\n\nHAD LOTS OF PROBLEMS WITH WRITE in write_some_handler_get of asyncConnection.cpp\n\n\n";
  }
  //  std::cout<<"\n\nbftm debug in asyncCraq.cpp:  Get wrote:   "<<bytes_transferred<<" bytes.\n\n";
}



void AsyncConnection::read_handler_get ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff)
{
  if (error)
  {
    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;
    
    CraqOperationResult tmper(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
    mOperationResultErrorVector.push_back(tmper);
    
    std::cout<<"\n\nGot an error in read_handler_get\n\n";

    delete sBuff;
  }
  else
  {
    std::istream is(sBuff);
    std::string line = "";
    std::string tmpLine;
  
    is >> tmpLine;
  
    while (tmpLine.size() != 0)
    {
      line.append(tmpLine);
      tmpLine = "";
      is >> tmpLine;
    }

    //    std::cout<<"\n\nbftm debug: in read_handler_get of asyncConnection.cpp.  This is line: "<<line<<"   and its size is:  "<<line.size()<<"     and this is the currentlySearchingFor:    "<<currentlySearchingFor <<"\n\n";
    //  std::cout<<"\n\nWill need to do some processing to ensure that I got the write line and write it back to tick\n\n";
    
    bool getResp = false;
  
    if (line.size() >= CRAQ_GET_RESP_SIZE)
    {
      getResp = true;
      
      for (int s=0; s < CRAQ_GET_RESP_SIZE; ++s)
      {
        getResp = getResp && (CRAQ_GET_RESP[s] == line[s]);
      }
    
      if (getResp)
      {

        std::string value = "";
        for (int s=CRAQ_GET_RESP_SIZE; s < bytes_transferred; ++s)
        {
          if (s-CRAQ_GET_RESP_SIZE < CRAQ_SERVER_SIZE)
          {
            value += line[s];
          }
        }

        CraqOperationResult tmper (std::atoi(value.c_str()),currentlySearchingFor, 0,true,CraqOperationResult::GET,mTracking);
        mOperationResultVector.push_back(tmper);
      }
    }
    
    
    if (!getResp)
    {
      //    mOperationResultErrorVector();
      std::cout<<"\n\nbftm debug: ERROR in asyncConnection.cpp under read_handler_get\n\n";
      std::cout<<"This is line:    "<<line<<"\n\n";

      mReady = NEED_NEW_SOCKET;

      mSocket->close();
      delete mSocket;
    
      CraqOperationResult tmper(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
      mOperationResultErrorVector.push_back(tmper);
    
      std::cout<<"\n\nGot an error in read_handler_get. not a get response\n\n";

      delete sBuff;
    }
    else
    {
      //need to reset ready flag so that I'm ready for more
      mReady = READY;
      delete sBuff;
    }
  }
}





}
