
#include <boost/asio.hpp>
#include "asyncCraq.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <string.h>
#include <sstream>

//nothing to destroy
AsyncCraq::~AsyncCraq()
{
  delete mSocket;
}


//nothing to initialize
AsyncCraq::AsyncCraq()
{
  connected = false;
  transId   = 1;
}




void AsyncCraq::write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred,int trans_id)
{
  if(trans_id != 0)
  {
    transIds.push_back(trans_id);
  }
  
  if (error)
  {
    //had trouble with this write.
    std::cout<<"\n\n\nHAD LOTS OF PROBLEMS WITH THIS WRITE\n\n\n";
    exit(1);
  }
}


void AsyncCraq::write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error)
  {
    //had trouble with this write.
    std::cout<<"\n\n\nHAD LOTS OF PROBLEMS WITH THIS WRITE\n\n\n";
    exit(1);
  }
}




void AsyncCraq::connect_handler2(const boost::system::error_code& error)
{
  if (! error)
  {
    connected = true;
  }
  else
  {
    std::cout<<"\n\nThe error is probably that we're not connecting to the router correctly.  (Check port #, ip address, and that the router is actually on.  This is the error:  "<<error<<"\n\n";
  }
}


void AsyncCraq::read_handler2( const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (bytes_transferred >= CRAQ_GET_RESP_SIZE)
  {
    bool getResp = true;
    for (int s=0; s<CRAQ_GET_RESP_SIZE; ++s)
    {
      getResp = getResp && (CRAQ_GET_RESP[s] == mReadData[s]);
    }

    if (getResp)//means that this is a getResp
    {
      std::string value = "";
      std::string object = "";
      std::cout<<"\n\n********************************\n";
      for (int s=CRAQ_GET_RESP_SIZE; s < bytes_transferred; ++s)
      {
        if (s-CRAQ_GET_RESP_SIZE < CRAQ_SERVER_SIZE)
        {
          value += mReadData[s];
        }
        if ((s-CRAQ_GET_RESP_SIZE >= CRAQ_SERVER_SIZE) && ((s-CRAQ_GET_RESP_SIZE) < (CRAQ_DATA_KEY_SIZE-1)))
        {
          object += mReadData[s];
        }
      }
      serverLoc.push_back(std::atoi(value.c_str()));

      CraqDataKey t_object;
      strncpy(t_object,object.c_str(),CRAQ_DATA_KEY_SIZE);
      
      //      objectId.push_back((CraqDataKey)object.c_str());
      objectId.push_back(t_object);

      //      std::cout<<"\n\t\t"<<value<<"  "<<std::atoi(value.c_str())<<"\n\n";
      //      std::cout<<"\n\t\t"<<object<<"\n\n";
    }
  }     
}


//runs the
void AsyncCraq::tick(std::vector<int> &serverIDs,  std::vector<char*> &objectIDs,std::vector<int>&trackedMessages)  //runs through one iteration of io_service.run_once.
{
  //  io_service.run_one();
  int numHandled = io_service.poll_one();
  if (numHandled ==0)
  {
    io_service.reset();
  }

  serverIDs         =    serverLoc;
  objectIDs         =     objectId;
  trackedMessages   =     transIds;
  
  serverLoc.clear();
  objectId.clear();
  transIds.clear();
  
}


void AsyncCraq::initialize(char* ipAdd, char* port)
{
  try
  {
    boost::asio::ip::tcp::resolver resolver(io_service);   //a resolver can resolve a query into a series of endpoints.
    //    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), "127.0.0.1", "4999");  //this is the query that we have.  It's ipv4, looking for localhost on port 80.
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAdd, port);  //this is the query that we have.  It's ipv4, looking for localhost on port 80.
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  //creates a list of endpoints that we can try to connect to.
    mSocket = new boost::asio::ip::tcp::socket(io_service);
    mSocket->async_connect(*iterator, boost::bind(&AsyncCraq::connect_handler2,this,_1));  //using that tcp socket for an asynchronous connection.

    
  }
  catch(std::exception &e)
  {
    std::cout<<"\n"<<e.what()<<"\n\n";
    std::cout<<"\n\nCaught error.\n\n";
    exit(1);
  }
}



//assumes that we're already connected.
int AsyncCraq::set(CraqDataKey dataToSet, int  dataToSetTo, bool trackMessage)
{
  //need to convert dataToSetTo to DataValue [DATA_VALUE_SIZE];

  boost::asio::async_read((*mSocket),
                          boost::asio::buffer(mReadData,CRAQ_DATA_RESPONSE_SIZE),
                          boost::asio::transfer_at_least(4),
                          boost::bind(&AsyncCraq::read_handler2,this,_1,_2));

  std::string query;
  query.append(CRAQ_DATA_SET_PREFIX);
  //  query.append(dataToSet); //this is the re
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
  query.append(dataToSet);
  
  query.append(CRAQ_DATA_SET_END_LINE);
  CraqDataSetQuery dsQuery;    
  strncpy(dsQuery,query.c_str(), CRAQ_DATA_SET_SIZE);

  if(trackMessage)
  {
    mSocket->async_write_some(boost::asio::buffer(dsQuery,CRAQ_DATA_SET_SIZE -2),
                              boost::bind(&AsyncCraq::write_some_handler_set,this,_1,_2,transId));
  }
  else
  {
    mSocket->async_write_some(boost::asio::buffer(dsQuery,CRAQ_DATA_SET_SIZE -2),
                              boost::bind(&AsyncCraq::write_some_handler_get,this,_1,_2));
  }




  if(trackMessage)
  {
    ++transId; //increments transId
    return transId -1;
  }
  return 0;
  
  
  
}




//datakey should have a null termination character.
AsyncCraq::AsyncCraqReqStatus AsyncCraq::get(CraqDataKey dataToGet)
{
  if (!connected)
  {
    return REQUEST_NOT_PROCESSED;
  }
  
  boost::asio::async_read((*mSocket),
                          boost::asio::buffer(mReadData,CRAQ_DATA_RESPONSE_SIZE),
                          boost::asio::transfer_at_least(4),
                          boost::bind(&AsyncCraq::read_handler2,this,_1,_2));

  std::string query;
  query.append(CRAQ_DATA_KEY_QUERY_PREFIX);
  query.append(dataToGet); //this is the re
  query.append(CRAQ_DATA_KEY_QUERY_SUFFIX);

  CraqDataKeyQuery dkQuery;
  strncpy(dkQuery,query.c_str(), CRAQ_DATA_KEY_QUERY_SIZE);
  
  mSocket->async_write_some(boost::asio::buffer(dkQuery,CRAQ_DATA_KEY_QUERY_SIZE-1),
                            boost::bind(&AsyncCraq::write_some_handler_get,this,_1,_2));
  
  return REQUEST_PROCESSED;
}
  





