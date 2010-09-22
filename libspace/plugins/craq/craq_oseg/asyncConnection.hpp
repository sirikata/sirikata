/*  Sirikata
 *  asyncConnection.hpp
 *
 *  Copyright (c) 2010, Behram Mistree
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __CRAQ_ASYNC_CONNECTION_HPP__
#define __CRAQ_ASYNC_CONNECTION_HPP__

#include <sirikata/core/util/Platform.hpp>
#include <boost/asio.hpp>
#include "asyncUtil.hpp"
#include <sirikata/space/SpaceContext.hpp>


//#define ASYNC_CONNECTION_DEBUG
namespace Sirikata
{
class CraqEntry;

class AsyncConnection
{

public:

  enum ConnectionState {READY, NEED_NEW_SOCKET,PROCESSING};

  void initialize(boost::asio::ip::tcp::socket* socket, boost::asio::ip::tcp::resolver::iterator, SpaceContext* spc, Network::IOStrand*  );

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
  Network::IOStrand* mStrand;

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
