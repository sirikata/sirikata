#ifndef __SET_ASYNC_THREE_UTIL_HPP__
#define __SET_ASYNC_THREE_UTIL_HPP__


#include <string>

namespace CBR
{


  //good
  //  const int CRAQ_DATA_KEY_SIZE = 34;
  //  typedef char CraqDataKey [CRAQ_DATA_KEY_SIZE];


  //good
  //  const char STREAM_CRAQ_DATA_KEY_QUERY_PREFIX[]  = "get ";
  //  const char STREAM_CRAQ_DATA_KEY_QUERY_SUFFIX[] = "\r\n";
  //  const int  STREAM_CRAQ_DATA_PREFIX_SIZE          = 4;
  //  const int  STREAM_CRAQ_DATA_SUFFIX_SIZE          = 2;
  const int  STREAM_CRAQ_DATA_KEY_QUERY_SIZE       = CRAQ_DATA_KEY_SIZE + CRAQ_DATA_PREFIX_SIZE + CRAQ_DATA_SUFFIX_SIZE + 1;//have to add +1 for Z suffix
  typedef char StreamCraqDataKeyQuery[STREAM_CRAQ_DATA_KEY_QUERY_SIZE];  //this is what actually goes out to the router.


  //good
  //  const char STREAM_CRAQ_DATA_SET_PREFIX[]       = "set ";
  //  const char STREAM_CRAQ_DATA_SET_END_LINE[]     = "\r\n";
  //  const int  STREAM_CRAQ_DATA_SET_PREFIX_SIZE    =      4;
  //  const int  STREAM_CRAQ_DATA_SET_END_LINE_SIZE  =      2;


  //good
  //  const char STREAM_CRAQ_DATA_TO_SET_SIZE[]      = " 12";
  //  const int  STREAM_CRAQ_DATA_TO_SET_SIZE_SIZE   =     3; //There are 3 digits in the above.
  //  const int  STREAM_CRAQ_SERVER_SIZE             =    10;
  //  const int  STREAM_CRAQ_TO_SET_SUFFIX_SIZE      =     2;
  const char STREAM_CRAQ_TO_SET_SUFFIX[]         =  "YZ";
  //  const int  STREAM_CRAQ_DATA_VALUE_SIZE         =    12;
  //  typedef char CraqDataValue [CRAQ_DATA_VALUE_SIZE];

  
  //good
  const int STREAM_CRAQ_DATA_SET_SIZE       = CRAQ_DATA_SET_PREFIX_SIZE + CRAQ_DATA_KEY_SIZE + CRAQ_DATA_TO_SET_SIZE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE + CRAQ_DATA_VALUE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE +1 + 1; //another + 1 for adding Z suffixx

  typedef char StreamCraqDataSetQuery[STREAM_CRAQ_DATA_SET_SIZE];  //this is what actually goes out to the router.


  //good
  //  const char STREAM_CRAQ_NOT_FOUND_RESP[]    = "NOT_FOUND";
  const char STREAM_CRAQ_STORED_RESP[]       =    "STORED";
  const char STREAM_CRAQ_VALUE_RESP[]        =     "VALUE";
  const char STREAM_CRAQ_ERROR_RESP[]        =     "ERROR";

  //good
  const int STREAM_CRAQ_NOT_FOUND_RESP_SIZE  =           9;
  const int STREAM_CRAQ_STORED_RESP_SIZE     =           6;
  const int STREAM_CRAQ_VALUE_RESP_SIZE      =           5;
  const int STREAM_CRAQ_ERROR_RESP_SIZE      =           5;

  //good
  const char STREAM_DATA_KEY_SUFFIX = 'Z';
  
  const int STREAM_SIZE_SIZE_TAG_GET_RESPONSE =2;
  
  const int STREAM_CRAQ_NUM_CONNECTIONS_SET        = 30;
  const int STREAM_CRAQ_NUM_CONNECTIONS_GET        = 1;


  const char STREAM_CRAQ_READ_RESULT_UNTIL_REGEX[] = "YY\r\n";
  const char STREAM_CRAQ_READ_ERROR_RESULT_UNTIL_REGEX[] = "ERROR\r\n";
  
  
  
  struct StreamCraqObjectID
  {
    StreamCraqDataKey cdk;
  };

  struct StreamCraqOperationResult
  {
    int servID;
    StreamCraqDataKey objID;
    int trackedMessage; //id assigned to track
    bool tracking;  //are we tracking this message
    bool succeeded;
    enum GetOrSet {GET, SET};
    GetOrSet whichOperation;
    CraqOperationResult(int sID,StreamCraqDataKey obj_id, int tm, bool hasSucceeded, GetOrSet, bool track_or_not);
    std::string idToString();
  };

  //  struct CraqInitializeArgs
  //  {
  //    std::string ipAdd;
  //    std::string port;
  //  };

  struct StreamCraqDataSetGet
  {
    StreamCraqDataKey dataKey;
    int dataKeyValue;
    bool trackMessage;
    int trackingID;
    enum TypeMessage {GET,SET};
    TypeMessage messageType;
    CraqDataSetGet(std::string dKey,int dKeyValue,bool tMessage,TypeMessage message_type);
    CraqDataSetGet(StreamCraqDataKey dKey,int dKeyValue,bool tMessage,TypeMessage message_type);
  };

}//namespace

#endif

