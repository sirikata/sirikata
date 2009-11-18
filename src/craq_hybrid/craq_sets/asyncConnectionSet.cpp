#include "asyncConnectionSet.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <map>
#include <utility>
#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>


namespace CBR
{


//constructor
AsyncConnectionSet::AsyncConnectionSet(SpaceContext* con, IOStrand* str)
  : ctx(con),
    mStrand(str)
{
  mReady = NEED_NEW_SOCKET; //starts in the state that it's requesting a new socket.  Presumably asyncCraq reads that we need a new socket, and directly calls "initialize" on this class
 
}

int AsyncConnectionSet::numStillProcessing()
{
  return (int) (allOutstandingQueries.size());
}


AsyncConnectionSet::~AsyncConnectionSet()
{
  if (! NEED_NEW_SOCKET)
  {
    mSocket->close();
    delete mSocket;
  }
}



AsyncConnectionSet::ConnectionState AsyncConnectionSet::ready()
{
  return mReady;
}


//servicing function.  gets results of all the operations that we were processing.
void AsyncConnectionSet::tick(std::vector<CraqOperationResult*>&opResults_get, std::vector<CraqOperationResult*>&opResults_error, std::vector<CraqOperationResult*>&opResults_trackedSets)
{
  
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

//gives us a socket to connect to
void AsyncConnectionSet::initialize( boost::asio::ip::tcp::socket* socket,    boost::asio::ip::tcp::resolver::iterator it)
{
  mSocket = socket;
  mReady = PROCESSING;   //need to run connection routine.  so until we receive an ack that conn has finished, we stay in processing state.

  mSocket->async_connect(*it, boost::bind(&AsyncConnectionSet::connect_handler,this,_1));  //using that tcp socket for an asynchronous connection.
  
  mPrevReadFrag = ""; 
}


//connection handler.
void AsyncConnectionSet::connect_handler(const boost::system::error_code& error)
{
  if (error)
  {
    mSocket->close();
    delete mSocket;
    mReady = NEED_NEW_SOCKET;

    std::cout<<"\n\nError in connection\n\n";
    return;
  }

  std::cout<<"\n\nbftm debug: asyncConnection: connected\n\n";
  mReady = READY;

  //  set_generic_read_result_handler();
  //  set_generic_read_error_handler();
  set_generic_stored_not_found_error_handler();
}




//public interface for setting data in craq via this connection.
bool AsyncConnectionSet::set(CraqDataKey dataToSet, int  dataToSetTo, bool track, int trackNum)
{
  if (mReady != READY)
  {
    std::cout<<"\n\nbftm debug:  huge error\n\n";
    exit(1);
    return false;
  }

  IndividualQueryData* iqd    =    new IndividualQueryData;
  iqd->is_tracking            =                      track;
  iqd->tracking_number        =                   trackNum;
  std::string tmpStringData = dataToSet;
  strncpy(iqd->currentlySearchingFor,tmpStringData.c_str(),tmpStringData.size() + 1);
    //  iqd->currentlySearchingFor  =                  dataToSet;
  iqd->currentlySettingTo     =                dataToSetTo;
  iqd->gs                     =   IndividualQueryData::SET;
  
  std::string index = dataToSet;
  index += STREAM_DATA_KEY_SUFFIX;

  allOutstandingQueries.insert(std::pair<std::string,IndividualQueryData*> (index, iqd));  //logs that this 

  mReady = PROCESSING;

  //generating the query to write.
  std::string query;
  query.append(CRAQ_DATA_SET_PREFIX);
  query.append(dataToSet); //this is the re
  query += STREAM_DATA_KEY_SUFFIX; //bftm changed here.
  query.append(CRAQ_DATA_TO_SET_SIZE);
  query.append(CRAQ_DATA_SET_END_LINE);

  //convert from integer to string.
  std::stringstream ss;
  ss << dataToSetTo;
  std::string tmper = ss.str();
  for (int s=0; s< CRAQ_SERVER_SIZE - ((int) tmper.size()); ++s)
  {
    query.append("0");
  }
    
  query.append(tmper);
  query.append(STREAM_CRAQ_TO_SET_SUFFIX);
  
  query.append(CRAQ_DATA_SET_END_LINE);
  StreamCraqDataSetQuery dsQuery;    
  strncpy(dsQuery,query.c_str(), STREAM_CRAQ_DATA_SET_SIZE);

  //  std::cout<<"\n\nThis is set query:  "<<query<<"\n\n";

  
  //creating callback for write function
  mSocket->async_write_some(boost::asio::buffer(dsQuery,STREAM_CRAQ_DATA_SET_SIZE -2),
                            boost::bind(&AsyncConnectionSet::write_some_handler_set,this,_1,_2));

  return true;
}


//dummy handler for writing the set instruction.  (Essentially, if we run into an error from doing the write operation of a set, we know what to do.)
void AsyncConnectionSet::write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
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


//datakey should have a null termination character.
//public interface for the get command
bool AsyncConnectionSet::get(CraqDataKey dataToGet)
{
  if (mReady != READY)
  {
    return false;
  }

  IndividualQueryData* iqd = new IndividualQueryData;
  iqd->is_tracking = false;
  iqd->tracking_number = 0;
  std::string tmpString = dataToGet;
  tmpString += STREAM_DATA_KEY_SUFFIX;
  strncpy(iqd->currentlySearchingFor,tmpString.c_str(),tmpString.size() + 1);
  iqd->gs = IndividualQueryData::GET;

  //need to add the individual query data to allOutstandingQueries.
  allOutstandingQueries.insert(std::pair<std::string, IndividualQueryData*> (tmpString, iqd));

  
  //crafts query
  std::string query;
  query.append(CRAQ_DATA_KEY_QUERY_PREFIX);
  query.append(dataToGet); //this is the re
  query += STREAM_DATA_KEY_SUFFIX; //bftm changed here.
  query.append(CRAQ_DATA_KEY_QUERY_SUFFIX);

  StreamCraqDataKeyQuery dkQuery;
  strncpy(dkQuery,query.c_str(), STREAM_CRAQ_DATA_KEY_QUERY_SIZE);
    
  //sets write handler
  mSocket->async_write_some(boost::asio::buffer(dkQuery,STREAM_CRAQ_DATA_KEY_QUERY_SIZE-1),
                            boost::bind(&AsyncConnectionSet::write_some_handler_get,this,_1,_2));

  mReady = PROCESSING;
  
  return true;
}


void AsyncConnectionSet::write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error)
  {
    printf("\n\nin write_some_handler_get\n\n");
    fflush(stdout);
    assert(false);
    killSequence();
  }
}


//This sequence needs to load all of its outstanding queries into the error results vector.
//
void AsyncConnectionSet::killSequence()
{
  mReady = NEED_NEW_SOCKET;
  mSocket->close();
  delete mSocket;


  printf("\n\n HIT KILL SEQUENCE \n\n");
  fflush(stdout);
  assert(false);

  //  still need to load all the outstanding stuff into error vector
}



void AsyncConnectionSet::printOutstanding()
{
  MultiOutstandingQueries::iterator multiIter;

  std::cout<<"\n\n\n\n**************PRINTING OUTSTANDING*************************\n\n";
  
  for (multiIter = allOutstandingQueries.begin(); multiIter != allOutstandingQueries.end(); ++multiIter)
  {
    std::cout<<"\t"<<multiIter->first<<"      "<<multiIter->second<<"\n";
  }

  std::cout<<"\n\n";
  
}



//looks through the entire response string and processes out relevant information:
//  "ABCDSTORED000000000011000000000000000000000ZVALUE000000000000000000000000000000000Z120000000000YYSTORED000000000022000000000000000000000ZSTORED000000000000003300000000000000000ZNOT_FOUND000000000011000000000000000000000ZERROR000000000011000000000000000000000Z"
// returns true if anything matches the basic template.  false otherwise
bool AsyncConnectionSet::processEntireResponse(std::string response)
{
  //index from stored
  //not_found
  //value
  //error
  bool returner = false;
  bool firstTime = true;
  
  bool keepChecking = true;
  bool secondChecking;

  response = mPrevReadFrag + response;  //see explanation at end when re-setting mPrevReadFrag
  mPrevReadFrag = "";
  
  while(keepChecking)
  {
  
    keepChecking   =                            false;

    //checks to see if there are any responses to get queries with data (also processes);
    secondChecking =             checkValue(response);  
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any responses to set responses that worked (stored) (also processes)
    secondChecking =            checkStored(response);  
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any responses to get queries that were not found  (also processes)
    secondChecking =          checkNotFound(response);  
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
  
  return returner;
}




/*
get abc
NOT_FOUND abc
set abc 3
def
STORED abc
get abc
VALUE abc 3
def
*/

// This function takes in a mutable string
// If there is a *full* not found message in response:
//  1) Removes it from response;
//  2) Add it to operation result queue
//  3) returns true
//
//Otherwise, returns false.
//
bool AsyncConnectionSet::checkNotFound(std::string& response)
{
  bool returner = false;
  size_t notFoundIndex = response.find(CRAQ_NOT_FOUND_RESP);
  
  std::string prefixed = "";
  std::string suffixed = "";
  
  
  if (notFoundIndex != std::string::npos)
  {
    prefixed = response.substr(0,notFoundIndex); //prefixed will be everything before the first STORED tag

    
    suffixed = response.substr(notFoundIndex); //first index should start with STORED_______

    size_t suffixNotFoundIndex = suffixed.find(CRAQ_NOT_FOUND_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP,STREAM_CRAQ_NOT_FOUND_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));
   
    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string notFoundPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      notFoundPhrase = suffixed.substr(suffixNotFoundIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      notFoundPhrase = suffixed.substr(suffixNotFoundIndex);
      response = prefixed;
    }
    //the above should have grabbed a phrase starting with "NOT_FOUND" from a sequence of characters
    //notFoundPhrase should store everything from NOT_FOUND up until either the end of response, or until a phrase keyed from a new keyword begins.  (eg. another "NOT_FOUND" or "STORED" or "VALUE" or "ERROR".)
    

    std::string dataKey;
    if ( parseValueNotFound(notFoundPhrase,dataKey))
    {
      //checks to make sure that the notFoundPhrase is fully and correctly formed
      processValueNotFound(dataKey);
      returner = true;
    }
    else
    {
      //the notfound phrase was incomplete.  we can't process it.  we return the response to its original state and return false immediately.
      response = response + notFoundPhrase;
      return false;
    }
    
    
  }
  return returner;
}

//checks a string to see if it's a correctly formatted not_found message.  If it is, grab data key from it, and return it in dataKey tab.
//if it is not formatted correctly, returns false
bool AsyncConnectionSet::parseValueNotFound(std::string response, std::string& dataKey)
{
  size_t notFoundIndex = response.find(CRAQ_NOT_FOUND_RESP);

  if (notFoundIndex == std::string::npos)
    return false;//means that there isn't actually a not found tag in this 

  if (notFoundIndex != 0)
    return false;//means that not found was in the wrong place.  return false so that can initiate kill sequence.

  //the not_found value was upfront.
  dataKey = response.substr(STREAM_CRAQ_NOT_FOUND_RESP_SIZE, CRAQ_DATA_KEY_SIZE);

  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  return true;
}



//takes the data key associated with a not found message, and loads it into operation result vector.
// void AsyncConnectionSet::processValueNotFound(std::string dataKey)
// {
//   //look through multimap to find 
//   std::pair <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange =  allOutstandingQueries.equal_range(dataKey);

//   MultiOutstandingQueries::iterator outQueriesIter;

//   for(outQueriesIter = eqRange.first; outQueriesIter != eqRange.second; ++outQueriesIter)
//   {
//     if (outQueriesIter->second->gs == IndividualQueryData::GET )
//     {
//       //says that this is a get.
//       CraqOperationResult* cor  = new CraqOperationResult(0,
//                                                           outQueriesIter->second->currentlySearchingFor,
//                                                           outQueriesIter->second->tracking_number,
//                                                           true,
//                                                           CraqOperationResult::GET,
//                                                           false); //this is a not_found, means that we add 0 for the id found

//       cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      
//       mOperationResultVector.push_back(cor); //loads this not found result into the results vector.


//       delete outQueriesIter->second;  //delete this from a memory perspective
//       allOutstandingQueries.erase(outQueriesIter); //
//     }
//   }
// }


void AsyncConnectionSet::processValueNotFound(std::string dataKey)
{
  //look through multimap to find 
  std::pair <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange =  allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;

  outQueriesIter = eqRange.first;
  while(outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::GET)
    {
      CraqOperationResult* cor  = new CraqOperationResult(0,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          true,
                                                          CraqOperationResult::GET,
                                                          false); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      
      mOperationResultVector.push_back(cor); //loads this not found result into the results vector.

      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //note the order of the ++ iterator is so that previous iterator gets erased.
    }
    else
    {
      ++outQueriesIter;
    }
  }
}



//just see comments for not found stuff
//looks to see if we received a value in the response.
//eg: VALUE000000000000000000000000000000000Z120000000000YY
bool AsyncConnectionSet::checkValue(std::string& response)
{
  bool returner = false;
  size_t valueIndex = response.find(STREAM_CRAQ_VALUE_RESP);
  
  std::string prefixed = "";
  std::string suffixed = "";
  
  if (valueIndex != std::string::npos)
  {
    prefixed = response.substr(0,valueIndex); //prefixed will be everything before the first STORED tag

    suffixed = response.substr(valueIndex); //first index should start with STORED_______

    size_t suffixValueIndex = suffixed.find(STREAM_CRAQ_VALUE_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP,STREAM_CRAQ_VALUE_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
   
    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string valuePhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      valuePhrase = suffixed.substr(suffixValueIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      valuePhrase = suffixed.substr(suffixValueIndex);
      response = prefixed;
    }
    
    std::string dataKey;
    int sID;
    if (parseValueValue(valuePhrase,dataKey,sID)) //sends it
    {
      processValueFound(dataKey,sID);
      returner = true;  //set returner here because we know we got a correct response.
    }
    else
    {
      response = response + valuePhrase;
      return false;
    }

  }
  return returner;
}


//takes the string associated with the datakey of a value found message and inserts it into operation value found
// void AsyncConnectionSet::processValueFound(std::string dataKey, int sID)
// {
//   //look through multimap to find
//   std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

//   MultiOutstandingQueries::iterator outQueriesIter;


//   for(outQueriesIter = eqRange.first; outQueriesIter != eqRange.second; ++outQueriesIter)
//   {
//     if (outQueriesIter->second->gs == IndividualQueryData::GET) //we only need to 
//     {
      
//       CraqOperationResult* cor  = new CraqOperationResult(sID,
//                                                           outQueriesIter->second->currentlySearchingFor,
//                                                           outQueriesIter->second->tracking_number,
//                                                           true,
//                                                           CraqOperationResult::GET,
//                                                           false); //this is a not_found, means that we add 0 for the id found

//       cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
//       mOperationResultVector.push_back(cor);

//       delete outQueriesIter->second;  //delete this from a memory perspective
//       allOutstandingQueries.erase(outQueriesIter); //


//       need to change this erase sequence around with the ++ iterator at the end;
      
//     }
//   }
// }


void AsyncConnectionSet::processValueFound(std::string dataKey, int sID)
{
  //look through multimap to find
  std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;

  outQueriesIter =  eqRange.first;

  while (outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::GET) //we only need to 
    {
      
      CraqOperationResult* cor  = new CraqOperationResult(sID,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          true,
                                                          CraqOperationResult::GET,
                                                          false); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      mOperationResultVector.push_back(cor);

      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //
    }
    else
    {
      outQueriesIter++;
    }
  }
}




//VALUE|CRAQ KEY|SIZE|VALUE
//returns the datakey and id associated with a value found response.
bool AsyncConnectionSet::parseValueValue(std::string response, std::string& dataKey,int& sID)
{
  size_t valueIndex = response.find(STREAM_CRAQ_VALUE_RESP);

    
  if (valueIndex == std::string::npos)
    return false;//means that value isn't actually in the response

  if (valueIndex != 0)
    return false;  //means that the value is in the wrong place.  return false so that can initiate kill sequence.

  
  //********Parse data key
  
  dataKey = response.substr(STREAM_CRAQ_VALUE_RESP_SIZE, CRAQ_DATA_KEY_SIZE);
  
  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  //*******Parse server id
  
  //next two characters are size


  if (STREAM_CRAQ_VALUE_RESP_SIZE + CRAQ_DATA_KEY_SIZE + STREAM_SIZE_SIZE_TAG_GET_RESPONSE > (int)response.size())
  {
    //    printf("\n\nThis is the response we're trying to substring ERROR:  %s\n\n", response.c_str());
    //    fflush(stdout);
    return false;
  }
    
  std::string tmpSID = response.substr(STREAM_CRAQ_VALUE_RESP_SIZE + CRAQ_DATA_KEY_SIZE + STREAM_SIZE_SIZE_TAG_GET_RESPONSE, CRAQ_SERVER_SIZE); // the +2 is from 

  //the above is calling sig abrt

  
    
  if ((int)tmpSID.size() != CRAQ_SERVER_SIZE)
    return false; //didn't read enough of the key header to find server id

  //parse tmpSID to int
  sID = std::atoi(tmpSID.c_str());

  return true;
}


// void AsyncConnectionSet::processStoredValue(std::string dataKey)
// {
//   //look through multimap to find
//   std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

//   MultiOutstandingQueries::iterator outQueriesIter;

//   for(outQueriesIter = eqRange.first; outQueriesIter != eqRange.second; ++outQueriesIter)
//   {
//     if (outQueriesIter->second->gs == IndividualQueryData::SET) //we only need to 
//     {
//       CraqOperationResult* cor  = new CraqOperationResult(outQueriesIter->second->currentlySettingTo,
//                                                           outQueriesIter->second->currentlySearchingFor,
//                                                           outQueriesIter->second->tracking_number,
//                                                           true,
//                                                           CraqOperationResult::SET,
//                                                           outQueriesIter->second->is_tracking); //this is a not_found, means that we add 0 for the id found

//       cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
//       //      mOperationResultVector.push_back(cor);

//       mOperationResultTrackedSetsVector.push_back(cor);
      
//       delete outQueriesIter->second;  //delete this from a memory perspective
//       allOutstandingQueries.erase(outQueriesIter); //
      
//     }
//   }
// }


void AsyncConnectionSet::processStoredValue(std::string dataKey)
{
  //look through multimap to find
  std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;

  outQueriesIter = eqRange.first;
  while (outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::SET)  //we only need to delete from multimap if it's a set response.
    {
      
      CraqOperationResult* cor  = new CraqOperationResult(outQueriesIter->second->currentlySettingTo,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          true,
                                                          CraqOperationResult::SET,
                                                          outQueriesIter->second->is_tracking); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      mOperationResultTrackedSetsVector.push_back(cor);
      
      delete outQueriesIter->second;  //delete this query from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //note that this returns the value before 
    }
    else
    {
      ++outQueriesIter;
    }
  }
}



bool AsyncConnectionSet::parseStoredValue(const std::string& response, std::string& dataKey)
{
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
  boost::asio::streambuf * sBuff = new boost::asio::streambuf;
  //  const boost::regex reg ("(ND\r\n|ERROR\r\n)");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.
  const boost::regex reg ("Z\r\n");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.

  //sets read handler
  //  boost::asio::async_read_until((*mSocket),
  //                                (*sBuff),
  //                                reg,
  //                                boost::bind(&AsyncConnectionSet::generic_read_stored_not_found_error_handler,this,_1,_2,sBuff));
  //

  boost::asio::async_read_until((*mSocket),
                                (*sBuff),
                                reg,
                                mStrand->wrap(boost::bind(&AsyncConnectionSet::generic_read_stored_not_found_error_handler,this,_1,_2,sBuff)));
  
}



void AsyncConnectionSet::generic_read_stored_not_found_error_handler ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff)
{
  //  set_generic_stored_not_found_error_handler();
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
    mReady = READY;
  }

  set_generic_stored_not_found_error_handler();
}




// Looks for and removes all instances of complete stored messages
// Processes them as well
//
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
      //      std::cout<<"\n\nDebug: value stored.\n";
      //      std::cout<<"\tdataKey: "<<dataKey<<"\n\n";
      returner = true;      
      processStoredValue(dataKey);
    }
    else
    {
      response = response + storedPhrase;
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


}//end namespace
