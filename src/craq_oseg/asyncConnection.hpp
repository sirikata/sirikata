
#ifndef __CRAQ_ASYNC_CONNECTION_HPP__
#define __CRAQ_ASYNC_CONNECTION_HPP__

#include <boost/asio.hpp>
#include "asyncUtil.hpp"




namespace CBR
{


class AsyncConnection
{
  
public:

  enum ConnectionState {READY, NEED_NEW_SOCKET,PROCESSING};
  
  void initialize(boost::asio::ip::tcp::socket* socket,     boost::asio::ip::tcp::resolver::iterator );  
  //  void tick(std::vector<int> &serverIDs,  std::vector<CraqObjectID> &objectIds,std::vector<int>&trackedMessages);  //runs through one iteration of io_service.run_once.

  void tick(std::vector<CraqOperationResult>&opResults_get, std::vector<CraqOperationResult>&opResults_error, std::vector<CraqOperationResult>&opResults_trackedSets);
  //  void tick(std::vector<CraqOperationResult>&opResults_get);
  
  AsyncConnection::ConnectionState ready(); //tells the querier whether I'm processing a message or available for more information.

  //  bool set(CraqDataKey dataToSet, int  dataToSetTo, bool track=false, int trackNum = 0);
  bool set(CraqDataKey dataToSet, int  dataToSetTo, bool track, int trackNum);
  //  bool set(CraqDataKey dataToSet, int  dataToSetTo);
  bool get(CraqDataKey dataToGet);

  
  ~AsyncConnection();
  AsyncConnection();

private:

  boost::asio::ip::tcp::socket* mSocket;

  CraqDataKey currentlySearchingFor;
  int currentlySettingTo;
  ConnectionState mReady;

  bool mTracking;
  int mTrackNumber;
  std::vector<CraqOperationResult> mOperationResultVector;
  std::vector<CraqOperationResult> mOperationResultErrorVector;
  std::vector<CraqOperationResult> mOperationResultTrackedSetsVector;
  


  
  //connect_handler
  void connect_handler(const boost::system::error_code& error);

  //set handler
  void read_handler_set ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
  void write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred);
  //get handler
  void write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred);
  void read_handler_get ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
};


}
  
#endif
