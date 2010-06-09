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

