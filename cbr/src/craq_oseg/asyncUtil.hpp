/*  Sirikata
 *  asyncUtil.hpp
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

#ifndef __ASYNC_UTIL_HPP__
#define __ASYNC_UTIL_HPP__

#include <string>
#include <sirikata/cbrcore/OSegLookupTraceToken.hpp>
#include "CraqEntry.hpp"
namespace Sirikata
{

const int CRAQ_DATA_RESPONSE_SIZE = 70;
typedef char CraqDataResponseBuffer [CRAQ_DATA_RESPONSE_SIZE];


  //multi-usef change
  //const int CRAQ_DATA_KEY_SIZE = 33;
  //typedef char CraqDataKey [CRAQ_DATA_KEY_SIZE];
  const int CRAQ_DATA_KEY_SIZE = 34;
  typedef char CraqDataKey [CRAQ_DATA_KEY_SIZE];

const char CRAQ_DATA_KEY_QUERY_PREFIX[]  = "get ";
const char CRAQ_DATA_KEY_QUERY_SUFFIX[] = "\r\n";
const int  CRAQ_DATA_PREFIX_SIZE          = 4;
const int  CRAQ_DATA_SUFFIX_SIZE          = 2;
const int CRAQ_DATA_KEY_QUERY_SIZE       = CRAQ_DATA_KEY_SIZE + CRAQ_DATA_PREFIX_SIZE + CRAQ_DATA_SUFFIX_SIZE;
typedef char CraqDataKeyQuery[CRAQ_DATA_KEY_QUERY_SIZE];  //this is what actually goes out to the router.



const char CRAQ_DATA_SET_PREFIX[]       = "set ";
const char CRAQ_DATA_SET_END_LINE[]     = "\r\n";
const int  CRAQ_DATA_SET_PREFIX_SIZE    =      4;
const int  CRAQ_DATA_SET_END_LINE_SIZE  =      2;



const char CRAQ_DATA_TO_SET_SIZE[]      = " 12";
const int  CRAQ_DATA_TO_SET_SIZE_SIZE   =     3; //There are 3 digits in the above.

const int  CRAQ_TO_SET_SUFFIX_SIZE      =     2;
const char CRAQ_TO_SET_SUFFIX[]         =  "ND";
const int  CRAQ_DATA_VALUE_SIZE         =    12;
typedef char CraqDataValue [CRAQ_DATA_VALUE_SIZE];


const char CRAQ_GET_RESP[]              =      "VALUE12";
const int  CRAQ_GET_RESP_SIZE           =              7;

  //const int CRAQ_THREAD_POOL_CONNECTIONS  =             10;


const int CRAQ_DATA_SET_SIZE       = CRAQ_DATA_SET_PREFIX_SIZE + CRAQ_DATA_KEY_SIZE + CRAQ_DATA_TO_SET_SIZE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE + CRAQ_DATA_VALUE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE +1;

typedef char CraqDataSetQuery[CRAQ_DATA_SET_SIZE];  //this is what actually goes out to the router.


const char CRAQ_NOT_FOUND_RESP[] = "NOT_FOUND";


const int CRAQ_DATA_GET_RESP_SIZE         = 52;
typedef char CraqDataGetResp[CRAQ_DATA_GET_RESP_SIZE];



  //const int CRAQ_NUM_CONNECTIONS            = 10;
  const int CRAQ_NUM_CONNECTIONS            = 40;

struct CraqObjectID
{
  CraqDataKey cdk;
};


struct CraqOperationResult
{
  CraqEntry servID;
  CraqDataKey objID;
  int trackedMessage; //id assigned to track
  bool tracking;  //are we tracking this message
  bool succeeded;
  enum GetOrSet {GET, SET};
  GetOrSet whichOperation;
  CraqOperationResult(const CraqEntry& sID,const CraqDataKey& obj_id, const int& tm, const bool& suc, const GetOrSet& gos,const  bool& track_or_not);
  CraqOperationResult(const CraqEntry& sID,const CraqDataKey& obj_id, const int& tm, const bool& suc, const GetOrSet& gos,const  bool& track_or_not, OSegLookupTraceToken* ttoken);
  std::string idToString();
  OSegLookupTraceToken* traceToken;
};

struct CraqInitializeArgs
{
  std::string ipAdd;
  std::string port;
};
struct CraqDataSetGet
{
  CraqDataKey dataKey;
  CraqEntry dataKeyValue;
  bool trackMessage;
  int trackingID;
  enum TypeMessage {GET,SET};
  TypeMessage messageType;
  CraqDataSetGet(const std::string& dKey, const CraqEntry& dKeyValue, const bool& tMessage, const TypeMessage& message_type);
  CraqDataSetGet(const CraqDataKey& dKey, const CraqEntry& dKeyValue, const bool& tMessage, const TypeMessage& message_type);
};


}//end namespace

#endif
