#include "asyncConnection.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include "../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include "CraqEntry.hpp"
namespace CBR
{

AsyncConnection::AsyncConnection():currentlySettingTo(NullServerID,0)
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



void AsyncConnection::tick(std::vector<CraqOperationResult*>&opResults_get, std::vector<CraqOperationResult*>&opResults_error, std::vector<CraqOperationResult*>&opResults_trackedSets)
{
  //  opResults_get = mOperationResultVector;

  if (mOperationResultVector.size() != 0)
  {
    opResults_get.swap(mOperationResultVector);
    if (mOperationResultVector.size() != 0)
    {
      mOperationResultVector.clear();
    }
  }

  //  opResults_error = mOperationResultErrorVector;
  if (mOperationResultErrorVector.size() !=0)
  {
    opResults_error.swap( mOperationResultErrorVector);
    if (mOperationResultErrorVector.size() != 0)
    {
      mOperationResultErrorVector.clear();
    }
  }

  //  opResults_trackedSets = mOperationResultTrackedSetsVector;
  if (mOperationResultTrackedSetsVector.size() != 0)
  {
    opResults_trackedSets.swap(mOperationResultTrackedSetsVector);
    if (mOperationResultTrackedSetsVector.size() != 0)
    {
      mOperationResultTrackedSetsVector.clear();
    }
  }
}


//void AsyncConnection::initialize( boost::asio::ip::tcp::socket* socket,    boost::asio::ip::tcp::resolver::iterator it)
void AsyncConnection::initialize(boost::asio::ip::tcp::socket* socket, boost::asio::ip::tcp::resolver::iterator it, SpaceContext* spc, IOStrand* strand )
{
  mSocket =       socket;
  mReady  =   PROCESSING;
  ctx     =          spc;
  mStrand =       strand;

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

    std::cout<<"\n\nOSEG Error in connection. This probably means the CRAQ router/chains are down.\n\n" << error;
    exit(1);
    return;
  }

#ifdef ASYNC_CONNECTION_DEBUG
  std::cout<<"\n\nbftm debug: asyncConnection: connected\n\n";
#endif
  mReady = READY;
}

bool AsyncConnection::set(CraqDataKey& dataToSet, const CraqEntry& dataToSetTo, bool& track, int& trackNum)
{
  if (mReady != READY)
  {
#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\n\nbftm debug:  huge set error\n\n";
#endif
    return false;
  }

  mTracking          =         track;
  mTrackNumber       =      trackNum;
  currentlySettingTo =   dataToSetTo;

#ifdef ASYNC_CONNECTION_DEBUG
  if (track)
  {
#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\n\nbftm debug: In set of asyncConnection.cpp.  Got a positive tracking.\n\n";
#endif
  }
#endif

  mReady = PROCESSING;
  std::string tmpString = dataToSet;
  strncpy(currentlySearchingFor,tmpString.c_str(),tmpString.size() + 1);

  //creating stream buffer
  boost::asio::streambuf* sBuff = new boost::asio::streambuf;


  //creating a read-callback.
  boost::asio::async_read_until((*mSocket),
                                (*sBuff),
                                boost::regex("\r\n"),
                                mStrand->wrap(boost::bind(&AsyncConnection::read_handler_set,this,_1,_2,sBuff)));
                                //                                ctx->osegStrand->wrap(boost::bind(&AsyncConnection::read_handler_set,this,_1,_2,sBuff)));

  //
  //  boost::asio::async_read_until((*mSocket),
  //                                (*sBuff),
  //                                boost::regex("\r\n"),
  //                                boost::bind(&AsyncConnection::read_handler_set,this,_1,_2,sBuff));


  //generating the query to write.
  
  std::string query;
  query.append(CRAQ_DATA_SET_PREFIX);
  query.append(dataToSet); //this is the re

  query.append(CRAQ_DATA_TO_SET_SIZE);
  query.append(CRAQ_DATA_SET_END_LINE);
  float radius=235.23523523;

  query.append(dataToSetTo.serialize());
  query.append(CRAQ_TO_SET_SUFFIX);

  query.append(CRAQ_DATA_SET_END_LINE);
  CraqDataSetQuery dsQuery;
  strncpy(dsQuery,query.c_str(), CRAQ_DATA_SET_SIZE);

  //  std::cout<<"\nbftm debug:  this is the set query:  "<<dsQuery<<"\n";


  //creating callback for write function

  async_write((*mSocket),
              boost::asio::buffer(query),
              boost::bind(&AsyncConnection::write_some_handler_set,this,_1,_2));


  //  mSocket->async_write_some(boost::asio::buffer(dsQuery,CRAQ_DATA_SET_SIZE -2),
  //                            boost::bind(&AsyncConnection::write_some_handler_set,this,_1,_2));


  return true;
}



void AsyncConnection::write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error)
  {
    //had trouble with this write.
#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\nHAD PROBLEMS IN ASYNC_CONNECTION WITH THIS WRITE\n";
#endif
    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;

    CraqOperationResult* tmper = new CraqOperationResult (currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::SET,mTracking); //false means that it didn't succeed.
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

  //  ctx->osegStrand->wrap(boost::bind(&AsyncConnection::finish_read_handler_set,this,line,sBuff));
  //  ctx->osegStrand->wrap(std::tr1::bind(&AsyncConnection::finish_read_handler_set,this,line,sBuff));
  //  ctx->osegStrand->wrap(std::tr1::bind(&AsyncConnection::finish_read_handler_set,this));

//void AsyncConnection::finish_read_handler_set(std::string line, boost::asio::streambuf* sBuff)


  //process this line.
  if (line.find("STORED") != std::string::npos)
  {
    if (mTracking)
    {
      //means that we need to save this query
      //      std::cout<<"\nbftm debug.  Wrote value for "<<currentlySearchingFor<<".  This is line:  "<<line <<"\n";

      CraqOperationResult* tmper = new CraqOperationResult(currentlySettingTo,currentlySearchingFor, mTrackNumber,true,CraqOperationResult::SET, mTracking);
      mOperationResultTrackedSetsVector.push_back(tmper);
    }

    //    std::cout<<"\n\nSTORED object:  "<<currentlySearchingFor<<".  On to the next one.\n\n";

    mReady  = READY;
  }
  else
  {
    //something besides stored was returned...indicates an error.
#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\n\nbftm debug: There was an error in asyncConnection.cpp under read_handler_set:  ";
    std::cout<<"This is line:   "<<line<<" ";
    std::cout<<"The error was for object with id:   "<<currentlySearchingFor<<"\n\n";
#endif

    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;

    CraqOperationResult* tmper = new CraqOperationResult(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::SET,mTracking); //false means that it didn't succeed.
    mOperationResultErrorVector.push_back(tmper);
  }
  //deletes buffer
  delete sBuff;

}



//datakey should have a null termination character.
bool AsyncConnection::get(CraqDataKey& dataToGet)
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

  const boost::regex reg ("(ND\r\n|ERROR\r\n)");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.


  //sets read handler
  boost::asio::async_read_until((*mSocket),
                                (*sBuff),
                                reg,
                                mStrand->wrap(boost::bind(&AsyncConnection::read_handler_get,this,_1,_2,sBuff)));


  //  boost::asio::async_read_until((*mSocket),
  //                                (*sBuff),
  //                                reg,
  //                                boost::bind(&AsyncConnection::read_handler_get,this,_1,_2,sBuff));

  //crafts query
  std::string query;
  query.append(CRAQ_DATA_KEY_QUERY_PREFIX);
  query.append(dataToGet); //this is the re
  query.append(CRAQ_DATA_KEY_QUERY_SUFFIX);

  CraqDataKeyQuery dkQuery;
  strncpy(dkQuery,query.c_str(), CRAQ_DATA_KEY_QUERY_SIZE);


  //  std::cout<<"\nThis is get query:  "<<dkQuery<<"\n";


  //sets write handler
  async_write((*mSocket),
              boost::asio::buffer(query),
              boost::bind(&AsyncConnection::write_some_handler_get,this,_1,_2));

  //  mSocket->async_write_some(boost::asio::buffer(dkQuery,CRAQ_DATA_KEY_QUERY_SIZE-1),
  //                            boost::bind(&AsyncConnection::write_some_handler_get,this,_1,_2));


  return true;
}


void AsyncConnection::write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error)
  {
    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;

    CraqOperationResult* tmper = new CraqOperationResult(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
    mOperationResultErrorVector.push_back(tmper);

    //    load error vector into both.

    //had trouble with this write.
#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\n\n\nHAD LOTS OF PROBLEMS WITH WRITE in write_some_handler_get of asyncConnection.cpp\n\n\n";
#endif
  }
}



void AsyncConnection::read_handler_get ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff)
{
  if (error)
  {
    mReady = NEED_NEW_SOCKET;

    mSocket->close();
    delete mSocket;

    CraqOperationResult* tmper = new CraqOperationResult(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
    mOperationResultErrorVector.push_back(tmper);

#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\n\nGot an error in read_handler_get\n\n";
#endif
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


    bool getResp = false;

    if (((int)line.size()) >= CRAQ_GET_RESP_SIZE) //if long enough to be a get response.
    {
      //      getResp = true;
      //should be a better way to do this  (check if first few characters are those of a get resp
//       for (int s=0; s < CRAQ_GET_RESP_SIZE; ++s)
//       {
//         getResp = getResp && (CRAQ_GET_RESP[s] == line[s]);
//       }

      size_t foundHeaderPos = line.find(CRAQ_GET_RESP);
      if (foundHeaderPos != std::string::npos)
      {
        if ((int) foundHeaderPos == 0)
        {
          getResp = true;
        }
      }

      if (getResp)
      {

//         std::string value = "";
//         for (int s=CRAQ_GET_RESP_SIZE; s < (int) bytes_transferred; ++s)
//         {
//           if (s-CRAQ_GET_RESP_SIZE < CRAQ_SERVER_SIZE)
//           {
//             value += line[s];
//           }
//         }


        std::string value = line.substr(CRAQ_GET_RESP_SIZE, CRAQ_SERVER_SIZE);

        if ((int)value.size() != CRAQ_SERVER_SIZE)
        {
          //means that the string wasn't long enough.  Throw an error.
          mReady = READY;
#ifdef ASYNC_CONNECTION_DEBUG
          std::cout<<"\n\nLINE NOT LONG ENOUGH!  "<<"line:  "<<line<<"  Will try again later to find object:  " << currentlySearchingFor  <<"\n\n";
#endif
          CraqOperationResult* tmper = new CraqOperationResult(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
          mOperationResultErrorVector.push_back(tmper);
        }
        else
        {
          //means that the line was long enough to be a response
          CraqEntry entry((unsigned char*)value.data());
          CraqOperationResult* tmper  = new CraqOperationResult (entry,currentlySearchingFor, 0,true,CraqOperationResult::GET,mTracking);
          mOperationResultVector.push_back(tmper);
        }
      } //added here.
    }


    if (!getResp)
    {

      if (line.find(CRAQ_NOT_FOUND_RESP) != std::string::npos)
      {
        mReady = READY;
#ifdef ASYNC_CONNECTION_DEBUG
        std::cout<<"\n\nUnknown!  "<<"line:  "<<line<<"  Will try again later to find object:  " << currentlySearchingFor  <<"\n\n";
#endif
        //        CraqOperationResult* tmper = new CraqOperationResult(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
        //        mOperationResultErrorVector.push_back(tmper);

        CraqOperationResult* tmper = new CraqOperationResult(CraqEntry::null(),currentlySearchingFor, mTrackNumber,true,CraqOperationResult::GET,mTracking); //false means that it didn't succeed...but we're just saying that the index was at 0
        mOperationResultVector.push_back(tmper);
        delete sBuff;
      }
      else
      {
        //    mOperationResultErrorVector();
#ifdef ASYNC_CONNECTION_DEBUG
        std::cout<<"\n\nbftm debug: ERROR in asyncConnection.cpp under read_handler_get ";
        std::cout<<"This is line:    "<<line<<" while looking for:  "<<  currentlySearchingFor<<"\n\n";
#endif
        mReady = NEED_NEW_SOCKET;

        mSocket->close();
        delete mSocket;

        //        CraqOperationResult* tmper = new CraqOperationResult(currentlySettingTo,currentlySearchingFor, mTrackNumber,false,CraqOperationResult::GET,mTracking); //false means that it didn't succeed.
        //        mOperationResultErrorVector.push_back(tmper);
        //bftm modified

        CraqOperationResult* tmper = new CraqOperationResult(CraqEntry::null(),currentlySearchingFor, mTrackNumber,true,CraqOperationResult::GET,mTracking); //false means that it didn't succeed...but we're just saying that the index was at 0
        mOperationResultVector.push_back(tmper);

        delete sBuff;
      }
    }
    else
    {
      //need to reset ready flag so that I'm ready for more
      mReady = READY;
      delete sBuff;
    }
  }

}

}//namespace
