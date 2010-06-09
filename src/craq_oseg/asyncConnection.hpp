
#ifndef __CRAQ_ASYNC_CONNECTION_HPP__
#define __CRAQ_ASYNC_CONNECTION_HPP__

#include <boost/asio.hpp>
#include "asyncUtil.hpp"
#include "../SpaceContext.hpp"


//#define ASYNC_CONNECTION_DEBUG
namespace Sirikata
{
class CraqEntry;

class AsyncConnection
{
  
public:

  enum ConnectionState {READY, NEED_NEW_SOCKET,PROCESSING};
  
  void initialize(boost::asio::ip::tcp::socket* socket, boost::asio::ip::tcp::resolver::iterator, SpaceContext* spc, IOStrand*  );

  void tick(std::vector<CraqOperationResult*>&opResults_get, std::vector<CraqOperationResult*>&opResults_error, std::vector<CraqOperationResult*>&opResults_trackedSets);  //runs through one iteration of io_service.run_once.
  
  AsyncConnection::ConnectionState ready(); //tells the querier whether I'm processing a message or available for more information.


  bool set(CraqDataKey& dataToSet, const CraqEntry&  dataToSetTo, bool& track, int& trackNum);
  bool get(CraqDataKey& dataToGet);
  
  ~AsyncConnection();
  AsyncConnection();

private:
  
  boost::asio::ip::tcp::socket* mSocket;

  CraqDataKey currentlySearchingFor;
  CraqEntry currentlySettingTo;
  ConnectionState mReady;

  SpaceContext* ctx;
  IOStrand* mStrand;
  
  bool mTracking;
  int mTrackNumber;
  std::vector<CraqOperationResult*> mOperationResultVector;
  std::vector<CraqOperationResult*> mOperationResultErrorVector;
  std::vector<CraqOperationResult*> mOperationResultTrackedSetsVector;
  
  //connect_handler
  void connect_handler(const boost::system::error_code& error);

  //set handler
  void read_handler_set ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
  void finish_read_handler_set(std::string line, boost::asio::streambuf* sBuff);
  void write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred);
  //get handler
  void write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred);
  void read_handler_get ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff);
};

}//end namespace
  
#endif
