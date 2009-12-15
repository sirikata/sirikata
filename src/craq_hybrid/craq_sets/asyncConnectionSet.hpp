#ifndef __CRAQ_ASYNC_CONNECTION_SET_HPP__
#define __CRAQ_ASYNC_CONNECTION_SET_HPP__

#include <boost/asio.hpp>
#include "../asyncCraqUtil.hpp"
#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include "../../ObjectSegmentation.hpp"
#include "../asyncCraqScheduler.hpp"
#include <sirikata/network/Asio.hpp>


//#define ASYNC_CONNECTION_DEBUG

namespace CBR
{

class AsyncConnectionSet
{
  
public:

  enum ConnectionState {READY, NEED_NEW_SOCKET,PROCESSING}; //we'll probably be always processing or need new socket.  (minus the initial connection registration.
  
  void initialize(Sirikata::Network::TCPSocket* socket,     boost::asio::ip::tcp::resolver::iterator );  

  AsyncConnectionSet::ConnectionState ready(); //tells the querier whether I'm processing a message or available for more information.

  void setBound(const CraqObjectID& obj_dataToGet, const int& dataToSetTo, const bool&  track, const int& trackNum);
  void set(const CraqDataKey& dataToSet, const int& dataToSetTo, const bool&  track, const int& trackNum);
  
  ~AsyncConnectionSet();
  AsyncConnectionSet(SpaceContext* con, IOStrand* str, IOStrand* error_strand, IOStrand* result_strand, AsyncCraqScheduler* master, ObjectSegmentation* oseg);
  
  int numStillProcessing();
  void stop();
  
  
private:
  Sirikata::Network::TCPSocket* mSocket;

  struct IndividualQueryData
  {
    enum GetOrSet {GET,SET};
    GetOrSet gs;
    CraqDataKey currentlySearchingFor;
    int currentlySettingTo;
    bool is_tracking;
    int tracking_number;
    uint64 time_admitted; //in milliseconds what time was when lookup was requested.
    Sirikata::Network::DeadlineTimer* deadline_timer;
  };
  typedef std::multimap<std::string, IndividualQueryData*> MultiOutstandingQueries;   //the string represents the obj id of the data.
  MultiOutstandingQueries allOutstandingQueries;  //we can be getting and setting so we need this to be a multimap

  SpaceContext*                      ctx;
  IOStrand*                      mStrand;
  IOStrand*                 mErrorStrand;
  IOStrand*               mResultsStrand;
  AsyncCraqScheduler*   mSchedulerMaster;
  ObjectSegmentation*              mOSeg;
  
  ConnectionState mReady;

  //this function is responsible for elegantly killing connections and telling the controlling asyncCraq that that's what it's doing.
  void killSequence();


  void set_generic_read_result_handler();
  void set_generic_read_error_handler();


  bool processEntireResponse(std::string response);

  void processStoredValue(std::string dataKey);
  bool parseStoredValue(const std::string& response, std::string& dataKey);


  bool checkStored(std::string& response);
  bool checkError(std::string& response);


  size_t smallestIndex(std::vector<size_t> sizeVec);

  
  std::string mPrevReadFrag;
  bool mReceivedStopRequest;
  
  void clear_all_deadline_timers();  
  //***********handlers**************
  //timeout callback handler
  void queryTimedOutCallbackSet(const boost::system::error_code& e, IndividualQueryData* iqd);
  
  //connect_handler
  void connect_handler(const boost::system::error_code& error);

  void generic_read_stored_not_found_error_handler ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
  void set_generic_stored_not_found_error_handler();

  
  //set handler
  void read_handler_set      ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
  void write_some_handler_set( const boost::system::error_code& error, std::size_t bytes_transferred);
};



}//end namespace
  
#endif
