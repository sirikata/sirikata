/*  Sirikata
 *  asyncConnectionSet.hpp
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

#ifndef __CRAQ_ASYNC_CONNECTION_SET_HPP__
#define __CRAQ_ASYNC_CONNECTION_SET_HPP__

#include <sirikata/core/util/Platform.hpp>
#include <boost/asio.hpp>
#include "../asyncCraqUtil.hpp"
#include <sirikata/cbrcore/SpaceContext.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include "../../ObjectSegmentation.hpp"
#include "../asyncCraqScheduler.hpp"
#include <sirikata/core/network/Asio.hpp>


//#define ASYNC_CONNECTION_DEBUG

namespace Sirikata
{

class AsyncConnectionSet
{

public:

  enum ConnectionState {READY, NEED_NEW_SOCKET,PROCESSING}; //we'll probably be always processing or need new socket.  (minus the initial connection registration.

  void initialize(Sirikata::Network::TCPSocket* socket,     boost::asio::ip::tcp::resolver::iterator );

  AsyncConnectionSet::ConnectionState ready(); //tells the querier whether I'm processing a message or available for more information.

  void setBound(const CraqObjectID& obj_dataToGet, const CraqEntry& dataToSetTo, const bool&  track, const int& trackNum);
  void set(const CraqDataKey& dataToSet, const CraqEntry& dataToSetTo, const bool&  track, const int& trackNum);

  ~AsyncConnectionSet();
  AsyncConnectionSet(SpaceContext* con, IOStrand* str, IOStrand* error_strand, IOStrand* result_strand, AsyncCraqScheduler* master, ObjectSegmentation* oseg, const std::tr1::function<void()>& readySetChanged);

  int numStillProcessing();
  void stop();
    ///sets the ready state to be in the processing mode, meaning work has been posted to its strand
  void setProcessing();

private:
  Sirikata::Network::TCPSocket* mSocket;
  struct IndividualQueryData
  {
    IndividualQueryData():currentlySettingTo(CraqEntry::null()){}
    enum GetOrSet {GET,SET};
    GetOrSet gs;
    CraqDataKey currentlySearchingFor;
    CraqEntry currentlySettingTo;
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

  volatile ConnectionState mReady;
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
  std::tr1::function<void()> mReadyStateChangedCallback;
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
