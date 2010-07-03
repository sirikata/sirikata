/*  Sirikata
 *  asyncUtil.cpp
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

#include "asyncUtil.hpp"
#include <string.h>

namespace Sirikata
{
  CraqOperationResult::CraqOperationResult(const CraqEntry& sID,const CraqDataKey& obj_id, const int& tm, const bool& suc, const GetOrSet& gos,const  bool& track_or_not):servID(sID)
  {
    memcpy(objID,obj_id,CRAQ_DATA_KEY_SIZE);
    trackedMessage = tm;
    succeeded = suc;
    whichOperation = gos;
    tracking = track_or_not;
    traceToken = NULL;
  }

  CraqOperationResult::CraqOperationResult(const CraqEntry& sID,const CraqDataKey& obj_id, const int& tm, const bool& suc, const GetOrSet& gos,const  bool& track_or_not, OSegLookupTraceToken* ttoken):servID(sID)
  {
    memcpy(objID,obj_id,CRAQ_DATA_KEY_SIZE);
    trackedMessage = tm;
    succeeded = suc;
    whichOperation = gos;
    tracking = track_or_not;
    traceToken = ttoken;
  }


  std::string CraqOperationResult::idToString()
  {
    objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
    return (std::string) objID;
  }


  CraqDataSetGet::CraqDataSetGet(const std::string& query, const CraqEntry& dKeyValue, const bool& tMessage, const TypeMessage& message_type):dataKeyValue(dKeyValue)
  {

    trackMessage   =       tMessage;
    messageType    =   message_type;
    strncpy(dataKey,query.c_str(),CRAQ_DATA_KEY_SIZE);
  }


  CraqDataSetGet::CraqDataSetGet(const CraqDataKey& dKey, const CraqEntry& dKeyValue, const bool& tMessage, const TypeMessage& message_type):dataKeyValue(dKeyValue)
  {
    memcpy (dataKey,dKey,CRAQ_DATA_KEY_SIZE);
    //  dataKey         =           dKey;
    trackMessage    =       tMessage;
    messageType     =   message_type;
  }

}//namespace
