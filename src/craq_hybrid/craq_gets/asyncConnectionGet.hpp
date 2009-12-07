#ifndef __CRAQ_ASYNC_CONNECTION_GET_HPP__
#define __CRAQ_ASYNC_CONNECTION_GET_HPP__

#include <boost/asio.hpp>
#include "../asyncCraqUtil.hpp"
#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include <sirikata/network/Asio.hpp>
#include "../../Timer.hpp"
#include "../../ObjectSegmentation.hpp"
#include "../asyncCraqScheduler.hpp"
#include "../../Utility.hpp"


namespace CBR
{

static const int MAX_TIME_BETWEEN_RESULTS = 50000;


class AsyncConnectionGet
{
private:
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

  
public:

  enum ConnectionState {READY, NEED_NEW_SOCKET,PROCESSING}; //we'll probably be always processing or need new socket.  (minus the initial connection registration.

  void initialize(Sirikata::Network::TCPSocket* socket,     boost::asio::ip::tcp::resolver::iterator );  
  
  AsyncConnectionGet::ConnectionState ready(); //tells the querier whether I'm processing a message or available for more information.

  void get(const CraqDataKey& dataToGet);
  void getBound(const CraqObjectID& obj_dataToGet);
  
  ~AsyncConnectionGet();
  AsyncConnectionGet(SpaceContext* con, IOStrand* str, IOStrand* error_strand, IOStrand* result_strand, AsyncCraqScheduler* master, ObjectSegmentation* oseg );
  
  int numStillProcessing();
  void printOutstanding();

  int runReQuery(); //re-query everything remaining in outstanding results.
  void printStatisticsTimesTaken();

  int getRespCount();
  
private:
  
  int mAllResponseCount;
  
  std::vector<double> mTimesTaken;
  
  int mTimesBetweenResults;
  bool mHandlerState;

  
  Sirikata::Network::TCPSocket* mSocket;

  
  void outputLargeOutstanding();
  
  typedef std::multimap<std::string, IndividualQueryData*> MultiOutstandingQueries;   //the string represents the obj id of the data.
  MultiOutstandingQueries allOutstandingQueries;  //we can be getting and setting so we need this to be a multimap
  
  ConnectionState mReady;

  bool getQuery(const CraqDataKey& dataToGet);


  void queryTimedOutCallbackGet(const boost::system::error_code& e, IndividualQueryData* iqd);
  void queryTimedOutCallbackGetPrint(const boost::system::error_code& e, IndividualQueryData* iqd);
  
  //this function is responsible for elegantly killing connections and telling the controlling asyncCraq that that's what it's doing.
  void killSequence();
  
  
  void processValueNotFound(std::string dataKey); //takes in 
  void processValueFound(std::string dataKey, int sID);
  void processStoredValue(std::string dataKey);

  
  bool parseValueNotFound(std::string response, std::string& dataKey);
  bool parseValueValue(std::string response, std::string& dataKey,int& sID);
  bool parseStoredValue(const std::string& response, std::string& dataKey);

  bool processEntireResponse(std::string response);

  bool checkStored(std::string& response);
  bool checkValue(std::string& response);
  bool checkNotFound(std::string& response);
  bool checkError(std::string& response);


  size_t smallestIndex(std::vector<size_t> sizeVec);

  
  std::string mPrevReadFrag;


  //***********handlers**************
  
  //connect_handler
  void connect_handler(const boost::system::error_code& error);

  void generic_read_stored_not_found_error_handler ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
  void set_generic_stored_not_found_error_handler();

  
  //set handler
  void read_handler_set      ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
  void write_some_handler_set( const boost::system::error_code& error, std::size_t bytes_transferred);
  //get handler
  void write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred);
  void read_handler_get      (  const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);


  //***strand and context
  SpaceContext* ctx;
  IOStrand* mStrand;
  IOStrand* mPostErrorsStrand;
  IOStrand* mResultStrand;
  AsyncCraqScheduler* mSchedulerMaster;  
  ObjectSegmentation* mOSeg;
  Timer mTimer;
  Timer mSpecificTimer;
  double getTime;
  int numGets;
  Duration mBeginDur;
  
};

}
  
#endif
