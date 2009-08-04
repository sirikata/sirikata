
#include <boost/asio.hpp>
#include <map>
#include <vector>

#ifndef __ASYNC_CRAQ_CLASS_H__
#define __ASYNC_CRAQ_CLASS_H__



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
const char CRAQ_DATA_TO_SET_SIZE[]      = " 42";
const int  CRAQ_DATA_TO_SET_SIZE_SIZE   =     3; //There are 3 digits in the above.
const int  CRAQ_SERVER_SIZE             =    10;
const int  CRAQ_DATA_VALUE_SIZE         =    42;
typedef char CraqDataValue [CRAQ_DATA_VALUE_SIZE];


const char CRAQ_GET_RESP[]              = "VALUE 42\r\n";
const int  CRAQ_GET_RESP_SIZE           =             10;



const int CRAQ_DATA_SET_SIZE       = CRAQ_DATA_SET_PREFIX_SIZE + CRAQ_DATA_KEY_SIZE + CRAQ_DATA_TO_SET_SIZE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE + CRAQ_DATA_VALUE_SIZE + CRAQ_DATA_SET_END_LINE_SIZE +1;

typedef char CraqDataSetQuery[CRAQ_DATA_SET_SIZE];  //this is what actually goes out to the router.


class AsyncCraq
{
public:
  AsyncCraq();
  //  void run();

  ~AsyncCraq();

  enum AsyncCraqReqStatus{REQUEST_PROCESSED, REQUEST_NOT_PROCESSED};
  

  //  void initialize(); //initializes the
  void initialize(char* ipAdd, char* port); //initializes the
  //  void tick(std::vector<int> &serverIDs,  std::vector<CraqDataKey> &objectIds,std::vector<int>&trackedMessages);  //runs through one iteration of io_service.run_once.
  void tick(std::vector<int> &serverIDs,  std::vector<char*> &objectIds,std::vector<int>&trackedMessages);  //runs through one iteration of io_service.run_once.
  AsyncCraqReqStatus get(CraqDataKey dataToGet);

  boost::asio::io_service io_service;  //creates an io service
  int set(CraqDataKey dataToSet, int  dataToSetTo,bool trackMesasge = false); //returns a transaction id.
  
private:

  int transId;
  std::vector<int> serverLoc;
  //  std::vector<CraqDataKey> objectId;
  std::vector<char*> objectId;
  std::vector<int> transIds;
  
  boost::asio::ip::tcp::socket* mSocket;

  bool connected;

  CraqDataResponseBuffer mReadData;  

  void connect_handler2(const boost::system::error_code& error);
  void read_handler2( const boost::system::error_code& error, std::size_t bytes_transferred);
  void write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred,int trans_id=0);
  void write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred);  
};


#endif


