
#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "../asyncCraqUtil.hpp"
#include "asyncConnectionGet.hpp"



#ifndef __ASYNC_CRAQ_GET_CLASS_H__
#define __ASYNC_CRAQ_GET_CLASS_H__

namespace CBR
{


class AsyncCraqGet
{
public:
  AsyncCraqGet();
  ~AsyncCraqGet();
  
  int runReQuery();
  
  enum AsyncCraqReqStatus{REQUEST_PROCESSED, REQUEST_NOT_PROCESSED};

  void initialize(std::vector<CraqInitializeArgs>);

  boost::asio::io_service io_service;  //creates an io service

  int set(StreamCraqDataSetGet cdSet);
  int get(StreamCraqDataSetGet cdGet);

  void runTestOfConnection();
  void runTestOfAllConnections();
  void tick(std::vector<StreamCraqOperationResult*>&getResults, std::vector<StreamCraqOperationResult*>&trackedSetResults);

  int queueSize();
  int numStillProcessing();
  
private:
  
  void processGetResults       (std::vector <StreamCraqOperationResult*> & getRes);
  void processErrorResults     (std::vector <StreamCraqOperationResult*> & errorRes);
  void processTrackedSetResults(std::vector <StreamCraqOperationResult*> & trackedSetRes);

  void straightPoll();
  std::vector<CraqInitializeArgs> mIpAddPort;
  std::vector<AsyncConnectionGet*> mConnections;
  int mCurrentTrackNum;
  bool connected;

  std::queue<StreamCraqDataSetGet> mQueue;
  

  void reInitializeNode(int s);
  void checkConnections(int s);
  
};


}//end namespace

#endif


