
#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "asyncUtil.hpp"
#include "asyncConnection.hpp"


#ifndef __ASYNC_CRAQ_CLASS_H__
#define __ASYNC_CRAQ_CLASS_H__


namespace CBR
{


class AsyncCraq
{
public:
  AsyncCraq();
  ~AsyncCraq();

  enum AsyncCraqReqStatus{REQUEST_PROCESSED, REQUEST_NOT_PROCESSED};

  void initialize(std::vector<CraqInitializeArgs>);

  boost::asio::io_service io_service;  //creates an io service

  int set(CraqDataSetGet cdSet);
  int get(CraqDataSetGet cdGet);

  void runTestOfConnection();
  void runTestOfAllConnections();
  void tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);

  int queueSize();
  
  
private:
  
  void processGetResults       (std::vector <CraqOperationResult*> & getRes);
  void processErrorResults     (std::vector <CraqOperationResult*> & errorRes);
  void processTrackedSetResults(std::vector <CraqOperationResult*> & trackedSetRes);
  
  
  std::vector<CraqInitializeArgs> mIpAddPort;
  AsyncConnection mConnection;
  std::vector<AsyncConnection> mConnections;
  int mCurrentTrackNum;

  
  bool connected;
  CraqDataResponseBuffer mReadData;  
  CraqDataGetResp mReadSomeData;

  std::queue<CraqDataSetGet> mQueue;
  

  void reInitializeNode(int s);
  void checkConnections(int s);
  
};

}//namespece
  

#endif


