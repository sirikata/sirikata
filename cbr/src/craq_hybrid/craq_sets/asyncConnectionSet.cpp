/*  Sirikata
 *  asyncConnectionSet.cpp
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

#include "asyncConnectionSet.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <map>
#include <utility>
#include <sirikata/cbrcore/SpaceContext.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include "../../ObjectSegmentation.hpp"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <sirikata/cbrcore/Timer.hpp>
#include <sirikata/cbrcore/VWTypes.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <functional>

namespace Sirikata
{

  //constructor
AsyncConnectionSet::AsyncConnectionSet(SpaceContext* con, IOStrand* str, IOStrand* error_strand, IOStrand* result_strand, AsyncCraqScheduler* master, ObjectSegmentation* oseg, const std::tr1::function<void()>& readyStateChangedCb)
  : mSocket(NULL),
    ctx(con),
    mStrand(str),
    mErrorStrand(error_strand),
    mResultsStrand(result_strand),
    mSchedulerMaster(master),
    mOSeg(oseg),
    mReceivedStopRequest(false),
    mReadyStateChangedCallback(readyStateChangedCb)
  {
    mReady = NEED_NEW_SOCKET; //starts in the state that it's requesting a new socket.  Presumably asyncCraq reads that we need a new socket, and directly calls "initialize" on this class

  }

  int AsyncConnectionSet::numStillProcessing()
  {
    return (int) (allOutstandingQueries.size());
  }


  AsyncConnectionSet::~AsyncConnectionSet()
  {
    //delete all outstanding queries
    for (MultiOutstandingQueries::iterator outQuerIt = allOutstandingQueries.begin(); outQuerIt != allOutstandingQueries.end(); ++outQuerIt)
    {
      if (outQuerIt->second->deadline_timer != NULL)
      {
        outQuerIt->second->deadline_timer->cancel();
        delete       outQuerIt->second->deadline_timer;
      }
      delete outQuerIt->second;
    }
    allOutstandingQueries.clear();


    if (! NEED_NEW_SOCKET)
    {
      mSocket->cancel();
      mSocket->close();
      delete mSocket;
      mSocket = NULL;
    }
  }

  void AsyncConnectionSet::stop()
  {
    mStrand->post(std::tr1::bind(&AsyncConnectionSet::clear_all_deadline_timers, this));

    mReceivedStopRequest = true;
    if (mSocket!= NULL)
      mSocket->cancel();
  }

  void AsyncConnectionSet::clear_all_deadline_timers()
  {
    MultiOutstandingQueries::iterator it;

    for(it = allOutstandingQueries.begin(); it != allOutstandingQueries.end(); ++it)
    {
      if(it->second->deadline_timer != NULL)
      {
        it->second->deadline_timer->cancel();
        it->second->deadline_timer = NULL;
      }
    }
  }



  AsyncConnectionSet::ConnectionState AsyncConnectionSet::ready()
  {
    return mReady;
  }


  //gives us a socket to connect to
  void AsyncConnectionSet::initialize( Sirikata::Network::TCPSocket* socket,    boost::asio::ip::tcp::resolver::iterator it)
  {
    mSocket = socket;
    mReady = PROCESSING;   //need to run connection routine.  so until we receive an ack that conn has finished, we stay in processing state.

    mSocket->async_connect(*it, mStrand->wrap(boost::bind(&AsyncConnectionSet::connect_handler,this,_1)));  //using that tcp socket for an asynchronous connection.

    mPrevReadFrag = "";
  }


  //connection handler.
  void AsyncConnectionSet::connect_handler(const boost::system::error_code& error)
  {
    if (mReceivedStopRequest)
      return;

    if (error)
    {
      mSocket->cancel();
      mSocket->close();
      delete mSocket;
      mSocket = NULL;
      mReady = NEED_NEW_SOCKET;
      mErrorStrand->post(mReadyStateChangedCallback);
      std::cout<<"\n\nError in connection\n\n";
      return;
    }

#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\n\nbftm debug: asyncConnection: connected\n\n";
#endif
    mReady = READY;
    mErrorStrand->post(mReadyStateChangedCallback);
    set_generic_stored_not_found_error_handler();
  }



  void AsyncConnectionSet::setBound(const CraqObjectID& obj_dataToGet, const CraqEntry& dataToSetTo, const bool&  track, const int& trackNum)
  {
    set(obj_dataToGet.cdk,dataToSetTo,track,trackNum);
  }


  void AsyncConnectionSet::setProcessing() {
    mReady=PROCESSING;
  }
  //public interface for setting data in craq via this connection.
  void AsyncConnectionSet::set(const CraqDataKey& dataToSet, const CraqEntry& dataToSetTo, const bool&  track, const int& trackNum)
  {
    if(mReceivedStopRequest)
      return;

    assert(mReady==PROCESSING);
/*********************
Ready needs to be set as soon as the posting thread posts the set message so that nothing else gets posted to be async_writ'd afterwards

    if (mReady != READY)
    {
#ifdef ASYNC_CONNECTION_DEBUG
      std::cout<<"\n\nI'm not ready yet in asyncConnectionSet\n\n";
#endif
      CraqOperationResult* cor  = new CraqOperationResult(CraqEntry::null(),
                                                          dataToSet,
                                                          trackNum,
                                                          false, //means that the operation has failed
                                                          CraqOperationResult::SET,
                                                          track);


      mErrorStrand->post(std::tr1::bind(&AsyncCraqScheduler::erroredSetValue, mSchedulerMaster, cor));

      return;
    }
*********************************/
    IndividualQueryData* iqd    =    new IndividualQueryData;
    iqd->is_tracking            =                      track;
    iqd->tracking_number        =                   trackNum;
    std::string tmpStringData = dataToSet;
    strncpy(iqd->currentlySearchingFor,tmpStringData.c_str(),tmpStringData.size() + 1);
    iqd->currentlySettingTo     =                dataToSetTo;
    iqd->gs                     =   IndividualQueryData::SET;

    Duration dur = Time::local() - Time::epoch();
    iqd->time_admitted = dur.toMilliseconds();


    std::string index = dataToSet;
    index += STREAM_DATA_KEY_SUFFIX;

    allOutstandingQueries.insert(std::pair<std::string,IndividualQueryData*> (index, iqd));  //logs that this


    iqd->deadline_timer  = new Sirikata::Network::DeadlineTimer(*ctx->ioService);
    iqd->deadline_timer->expires_from_now(boost::posix_time::milliseconds(STREAM_ASYNC_SET_TIMEOUT_MILLISECONDS));
    iqd->deadline_timer->async_wait(mStrand->wrap(boost::bind(&AsyncConnectionSet::queryTimedOutCallbackSet, this, _1, iqd)));




    //generating the query to write.
    std::string query;
    query.append(CRAQ_DATA_SET_PREFIX);
    query.append(dataToSet); //this is the re
    query += STREAM_DATA_KEY_SUFFIX; //bftm changed here.
    query.append(CRAQ_DATA_TO_SET_SIZE);
    query.append(CRAQ_DATA_SET_END_LINE);


    query.append(dataToSetTo.serialize());
    query.append(STREAM_CRAQ_TO_SET_SUFFIX);
    query.append(CRAQ_DATA_SET_END_LINE);


    //sets write handler
    async_write((*mSocket),
                boost::asio::buffer(query),
                boost::bind(&AsyncConnectionSet::write_some_handler_set,this,_1,_2));

  }


  //dummy handler for writing the set instruction.  (Essentially, if we run into an error from doing the write operation of a set, we know what to do.)
  void AsyncConnectionSet::write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred)
  {
    if (mReceivedStopRequest)
      return;

    mReady=READY;
    static int thisWrite = 0;

    ++thisWrite;

    if (error)
    {
      printf("\n\nin write_some_handler_set\n\n");
      fflush(stdout);
      assert(false);
      killSequence();
    }
  }


  //This sequence needs to load all of its outstanding queries into the error results vector.
  void AsyncConnectionSet::killSequence()
  {
    if (mReceivedStopRequest)
      return;

    mReady = NEED_NEW_SOCKET;
    mErrorStrand->post(mReadyStateChangedCallback);
    mSocket->cancel();
    mSocket->close();
    delete mSocket;
    mSocket = NULL;

    printf("\n\n HIT KILL SEQUENCE \n\n");
    fflush(stdout);
    assert(false);
  }


//looks through the entire response string and processes out relevant information:
//  "ABCDSTORED000000000011000000000000000000000ZVALUE000000000000000000000000000000000Z120000000000YYSTORED000000000022000000000000000000000ZSTORED000000000000003300000000000000000ZNOT_FOUND000000000011000000000000000000000ZERROR000000000011000000000000000000000Z"
// returns true if anything matches the basic template.  false otherwise
bool AsyncConnectionSet::processEntireResponse(std::string response)
{
  if (mReceivedStopRequest)
    return false;

  bool returner = false;
  bool firstTime = true;

  bool keepChecking = true;
  bool secondChecking;

  response = mPrevReadFrag + response;  //see explanation at end when re-setting mPrevReadFrag
  mPrevReadFrag = "";

  while(keepChecking)
  {
    keepChecking   =                            false;

    //checks to see if there are any responses to set responses that worked (stored) (also processes)
    secondChecking =            checkStored(response);
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any error responses.  (also processes)
    secondChecking =             checkError(response);
    keepChecking   =   keepChecking || secondChecking;

    if (firstTime)
    {
      returner  = keepChecking;  //states whether or not there were any full-formed expressions in this read
      firstTime =        false;
    }
  }

  mPrevReadFrag = response;  //apparently I've been running into the problem of what happens when data gets interrupted mid-stream
                             //The solution is to save the end bit of data that couldn't be parsed correctly (now in "response" variable and save it for appending to the next read.

  if ((int)mPrevReadFrag.size() > MAX_SET_PREV_READ_FRAG_SIZE)
    mPrevReadFrag.substr(((int)mPrevReadFrag.size()) - CUT_SET_PREV_READ_FRAG);

  return returner;
}


void AsyncConnectionSet::processStoredValue(std::string dataKey)
{
  if (mReceivedStopRequest)
    return;

  //look through multimap to find
  std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;

  int numberPostedHere   = 0;
  outQueriesIter = eqRange.first;
  while (outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::SET)  //we only need to delete from multimap if it's a set response.
    {
      ++numberPostedHere;


      CraqOperationResult* cor  = new CraqOperationResult(outQueriesIter->second->currentlySettingTo,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          true,
                                                          CraqOperationResult::SET,
                                                          outQueriesIter->second->is_tracking); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      mResultsStrand->post(std::tr1::bind(&ObjectSegmentation::craqSetResult,mOSeg,cor));




      //cancel the callback
      if (outQueriesIter->second->deadline_timer != NULL)
      {
        outQueriesIter->second->deadline_timer->cancel();
        delete outQueriesIter->second->deadline_timer;
      }


      delete outQueriesIter->second;  //delete this query from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //note that this returns the value before
    }
    else
    {
      ++outQueriesIter;
    }
  }
}

//runs through the response in response and says whether the value you gave it completely matches the signature of a stored value resp.
bool AsyncConnectionSet::parseStoredValue(const std::string& response, std::string& dataKey)
{
  if (mReceivedStopRequest)
    return false;

  size_t storedIndex = response.find(STREAM_CRAQ_STORED_RESP);

  if (storedIndex == std::string::npos)
    return false;//means that there isn't actually a not found tag in this

  if (storedIndex != 0)
    return false;//means that not found was in the wrong place.  return false so that can initiate kill sequence.

  //the not_found value was upfront.
  dataKey = response.substr(STREAM_CRAQ_STORED_RESP_SIZE, CRAQ_DATA_KEY_SIZE);

  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  return true;
}


void AsyncConnectionSet::set_generic_stored_not_found_error_handler()
{
  if (mReceivedStopRequest)
    return;

  boost::asio::streambuf * sBuff = new boost::asio::streambuf;

  const boost::regex reg ("Z\r\n");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.


  boost::asio::async_read_until((*mSocket),
                                (*sBuff),
                                reg,
                                mStrand->wrap(boost::bind(&AsyncConnectionSet::generic_read_stored_not_found_error_handler,this,_1,_2,sBuff)));

}



void AsyncConnectionSet::generic_read_stored_not_found_error_handler ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff)
{
  if (mReceivedStopRequest)
  {
    delete sBuff;
    return;
  }

  if (error)
  {
    killSequence();
    return;
  }

  std::istream is(sBuff);
  std::string response = "";
  std::string tmpLine;

  is >> tmpLine;

  while (tmpLine.size() != 0)
  {
    response.append(tmpLine);
    tmpLine = "";
    is >> tmpLine;
  }

  bool anything = processEntireResponse(response); //this will go through everything that we read out.  And sort it by errors, storeds, not_founds, and values.

  delete sBuff;

  if (!anything) //means that the response wasn't one that we could reasonably parse.
  {
    printf ("\n\nThis is response:  %s\n\n", response.c_str());
    fflush(stdout);
    killSequence();
  }
  else
  {
  }

  set_generic_stored_not_found_error_handler();
}




// Looks for and removes all instances of complete stored messages
// Processes them as well
bool AsyncConnectionSet::checkStored(std::string& response)
{
  bool returner = false;
  size_t storedIndex = response.find(STREAM_CRAQ_STORED_RESP);

  std::string prefixed = "";
  std::string suffixed = "";


  if (storedIndex != std::string::npos)
  {
    prefixed = response.substr(0,storedIndex); //prefixed will be everything before the first STORED tag


    suffixed = response.substr(storedIndex); //first index should start with STORED_______

    size_t suffixStoredIndex = suffixed.find(STREAM_CRAQ_STORED_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP,STREAM_CRAQ_STORED_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));

    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string storedPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      storedPhrase = suffixed.substr(suffixStoredIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      storedPhrase = suffixed.substr(suffixStoredIndex);
      response = prefixed;
    }

    std::string dataKey;
    if (parseStoredValue(storedPhrase, dataKey))
    {
      returner = true;
      processStoredValue(dataKey);
    }
    else
    {
      response = prefixed + storedPhrase;
      return false;
    }
  }
  return returner;
}


//returns the smallest entry in the entered vector.
size_t AsyncConnectionSet::smallestIndex(std::vector<size_t> sizeVec)
{
  std::sort(sizeVec.begin(), sizeVec.end());
  return sizeVec[0];
}



bool AsyncConnectionSet::checkError(std::string& response)
{
  bool returner = false;
  size_t errorIndex = response.find(STREAM_CRAQ_ERROR_RESP);

  std::string prefixed = "";
  std::string suffixed = "";


  if (errorIndex != std::string::npos)
  {
    prefixed = response.substr(0,errorIndex); //prefixed will be everything before the first STORED tag
    suffixed = response.substr(errorIndex); //first index should start with STORED_______

    size_t suffixErrorIndex = suffixed.find(STREAM_CRAQ_ERROR_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP,STREAM_CRAQ_ERROR_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));

    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string errorPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      errorPhrase = suffixed.substr(suffixErrorIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      returner = true;
      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      errorPhrase = suffixed.substr(suffixErrorIndex);
      returner = true;
      response = prefixed;
    }


    printf("\n\nBFTM big problem:  got error:  %s\n\n", errorPhrase.c_str());
    std::string mmer = prefixed + suffixed;
    printf("\n\nThis is line:  %s\n\n",mmer.c_str() );
    fflush(stdout);
  }
  return returner;
}



//This gets called with a pointer to query data as its argument.  The query data inside of this has waited too long to be processed.
//Therefore, we must return an error as our operation result and remove the query from our list of outstanding queries.
void AsyncConnectionSet::queryTimedOutCallbackSet(const boost::system::error_code& e, IndividualQueryData* iqd)
{
  if (mReceivedStopRequest)
    return;

  if (e == boost::asio::error::operation_aborted)
    return;

#ifdef ASYNC_CONNECTION_DEBUG
  std::cout<<"\n\nQuery timeout callback set\n";
#endif

  bool foundInIterator = false;

  //look through multimap to find
  std::pair <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange =  allOutstandingQueries.equal_range(iqd->currentlySearchingFor);


  MultiOutstandingQueries::iterator outQueriesIter;
  outQueriesIter = eqRange.first;

  while(outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::SET )
    {
      CraqOperationResult* cor  = new CraqOperationResult(outQueriesIter->second->currentlySettingTo,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          false, //means that it failed.
                                                          CraqOperationResult::SET,
                                                          outQueriesIter->second->is_tracking);

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      mErrorStrand->post(boost::bind(&AsyncCraqScheduler::erroredSetValue, mSchedulerMaster, cor));

#ifdef ASYNC_CONNECTION_DEBUG
      std::cout<<"\n\nSending error from set\n\n";
#endif

      if (outQueriesIter->second->deadline_timer != NULL)
      {
        outQueriesIter->second->deadline_timer->cancel();
        delete outQueriesIter->second->deadline_timer;
      }

      foundInIterator = true;

      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //
    }
    else
    {
      outQueriesIter++;
    }
  }

  if (! foundInIterator) {
#ifdef ASYNC_CONNECTION_DEBUG
    std::cout<<"\n\nAsyncConnectionGet CALLBACK.  Not found in iterator.\n\n";
#endif
  }
}

}//end namespace
