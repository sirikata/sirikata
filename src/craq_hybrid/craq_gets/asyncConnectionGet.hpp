#ifndef __CRAQ_ASYNC_CONNECTION_GET_HPP__
#define __CRAQ_ASYNC_CONNECTION_GET_HPP__

#include <boost/asio.hpp>
#include "../asyncCraqUtil.hpp"


namespace CBR
{

//static const int MAX_TIME_BETWEEN_RESULTS = 10000;
static const int MAX_TIME_BETWEEN_RESULTS = 50000;


class AsyncConnectionGet
{
  
public:

  enum ConnectionState {READY, NEED_NEW_SOCKET,PROCESSING}; //we'll probably be always processing or need new socket.  (minus the initial connection registration.
  
  void initialize(boost::asio::ip::tcp::socket* socket,     boost::asio::ip::tcp::resolver::iterator );  

  void tick(std::vector<CraqOperationResult*>&opResults_get, std::vector<CraqOperationResult*>&opResults_error, std::vector<CraqOperationResult*>&opResults_trackedSets);  //runs through one iteration of io_service.run_once.
  
  AsyncConnectionGet::ConnectionState ready(); //tells the querier whether I'm processing a message or available for more information.

  bool set(CraqDataKey dataToSet, int  dataToSetTo, bool track, int trackNum);
  bool get(CraqDataKey dataToGet);
  
  ~AsyncConnectionGet();
  AsyncConnectionGet();


  int numStillProcessing();
  void printOutstanding();

  int runReQuery(); //re-query everything remaining in outstanding results.
  
private:

  int mTimesBetweenResults;
  bool mHandlerState;
  
  boost::asio::ip::tcp::socket* mSocket;

  struct IndividualQueryData
  {
    enum GetOrSet {GET,SET};
    GetOrSet gs;
    CraqDataKey currentlySearchingFor;
    int currentlySettingTo;
    bool is_tracking;
    int tracking_number;
  };
  typedef std::multimap<std::string, IndividualQueryData*> MultiOutstandingQueries;   //the string represents the obj id of the data.
  MultiOutstandingQueries allOutstandingQueries;  //we can be getting and setting so we need this to be a multimap
  
  std::string mAllResps;
  
  ConnectionState mReady;

  bool getQuery(CraqDataKey dataToGet);
  
  //this function is responsible for elegantly killing connections and telling the controlling asyncCraq that that's what it's doing.
  void killSequence();
  
  std::vector<CraqOperationResult*> mOperationResultVector;
  std::vector<CraqOperationResult*> mOperationResultErrorVector;
  std::vector<CraqOperationResult*> mOperationResultTrackedSetsVector;

  void set_generic_read_result_handler();
  void set_generic_read_error_handler();


  void generic_read_handler       ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);

  void generic_read_error_handler ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);

  
  bool processGet(const std::string& response);
  bool processSet(const std::string& response);

  
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
};

}
  
#endif
