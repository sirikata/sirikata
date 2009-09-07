#ifndef __ASYNC_UTIL_HPP__
#define __ASYNC_UTIL_HPP__

#include <string>


namespace CBR
{

const int CRAQ_DATA_RESPONSE_SIZE = 70;
typedef char CraqDataResponseBuffer [CRAQ_DATA_RESPONSE_SIZE];



const int CRAQ_DATA_KEY_SIZE = 33;
typedef char CraqDataKey [CRAQ_DATA_KEY_SIZE];


const char CRAQ_DATA_KEY_QUERY_PREFIX[]  = "get ";
const char CRAQ_DATA_KEY_QUERY_SUFFIX[] = "\r\n";
const int  CRAQ_DATA_PREFIX_SIZE          = 4;
const int  CRAQ_DATA_SUFFIX_SIZE          = 2;
const int CRAQ_DATA_KEY_QUERY_SIZE       = CRAQ_DATA_KEY_SIZE + CRAQ_DATA_PREFIX_SIZE + CRAQ_DATA_SUFFIX_SIZE;
typedef char CraqDataKeyQuery[CRAQ_DATA_KEY_QUERY_SIZE];  //this is what actually goes out to the router.

//the above is all okay.





const char CRAQ_DATA_SET_PREFIX[]       = "set ";
const char CRAQ_DATA_SET_END_LINE[]     = "\r\n";
const int  CRAQ_DATA_SET_PREFIX_SIZE    =      4;
const int  CRAQ_DATA_SET_END_LINE_SIZE  =      2;



/*
const char DATA_TO_SET_SIZE[]      = " 10";
const int  DATA_TO_SET_SIZE_SIZE   =     3; //There are 4 digits in the above.
const int DATA_VALUE_SIZE = 10;
typedef char DataValue [DATA_VALUE_SIZE];
*/
const char CRAQ_DATA_TO_SET_SIZE[]      = " 12";
const int  CRAQ_DATA_TO_SET_SIZE_SIZE   =     3; //There are 3 digits in the above.
const int  CRAQ_SERVER_SIZE             =    10;
const int  CRAQ_TO_SET_SUFFIX_SIZE      =     2;
const char CRAQ_TO_SET_SUFFIX[]         =  "YY";
const int  CRAQ_DATA_VALUE_SIZE         =    12;
typedef char CraqDataValue [CRAQ_DATA_VALUE_SIZE];


const char CRAQ_GET_RESP[]              =      "VALUE12";
const int  CRAQ_GET_RESP_SIZE           =              7;

const int CRAQ_THREAD_POOL_CONNECTIONS  =             10;


const int CRAQ_DATA_SET_SIZE       = CRAQ_DATA_SET_PREFIX_SIZE + CRAQ_DATA_KEY_SIZE + CRAQ_DATA_TO_SET_SIZE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE + CRAQ_DATA_VALUE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE +1;

typedef char CraqDataSetQuery[CRAQ_DATA_SET_SIZE];  //this is what actually goes out to the router.



const int CRAQ_DATA_GET_RESP_SIZE         = 52;
typedef char CraqDataGetResp[CRAQ_DATA_GET_RESP_SIZE];



//const int CRAQ_NUM_CONNECTIONS            = 10;
//const int CRAQ_NUM_CONNECTIONS            = 17;
  const int CRAQ_NUM_CONNECTIONS            = 30;

struct CraqObjectID
{
  CraqDataKey cdk;
};

struct CraqOperationResult
{
  int servID;
  CraqDataKey objID;
  int trackedMessage; //id assigned to track
  bool tracking;  //are we tracking this message
  bool succeeded;
  enum GetOrSet {GET, SET};
  GetOrSet whichOperation;
  CraqOperationResult(int sID,CraqDataKey obj_id, int tm, bool hasSucceeded, GetOrSet, bool track_or_not);
  std::string idToString();
};

struct CraqInitializeArgs
{
  std::string ipAdd;
  std::string port;
};

struct CraqDataSetGet
{
  CraqDataKey dataKey;
  int dataKeyValue;
  bool trackMessage;
  int trackingID;
  enum TypeMessage {GET,SET};
  TypeMessage messageType;
  CraqDataSetGet(std::string dKey,int dKeyValue,bool tMessage,TypeMessage message_type);
  CraqDataSetGet(CraqDataKey dKey,int dKeyValue,bool tMessage,TypeMessage message_type);
};

}

#endif

