#include "asyncUtil.hpp"
#include <string.h>

namespace CBR
{
  CraqOperationResult::CraqOperationResult(int sID,CraqDataKey obj_id, int tm, bool suc, GetOrSet gos, bool track_or_not)
  {
    servID = sID;
    memcpy(objID,obj_id,CRAQ_DATA_KEY_SIZE);
    trackedMessage = tm;
    succeeded = suc;
    whichOperation = gos;
    tracking = track_or_not;
  }

  std::string CraqOperationResult::idToString()
  {
    objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
    return (std::string) objID;
  }

  
  CraqDataSetGet::CraqDataSetGet(std::string query,int dKeyValue,bool tMessage,TypeMessage message_type)
  {
    dataKeyValue   =      dKeyValue;
    trackMessage   =       tMessage;
    messageType    =   message_type;
    strncpy(dataKey,query.c_str(),CRAQ_DATA_KEY_SIZE);
  }


  CraqDataSetGet::CraqDataSetGet(CraqDataKey dKey,int dKeyValue,bool tMessage,TypeMessage message_type)
  {
    memcpy (dataKey,dKey,CRAQ_DATA_KEY_SIZE);
    //  dataKey         =           dKey;
    trackMessage    =       tMessage;
    dataKeyValue    =      dKeyValue;
    messageType     =   message_type;
  }

}//namespace

