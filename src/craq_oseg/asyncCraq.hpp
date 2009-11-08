
#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "asyncUtil.hpp"
#include "asyncConnection.hpp"
#include "../Timer.hpp"
#include "../SpaceContext.hpp"


#ifndef __ASYNC_CRAQ_CLASS_H__
#define __ASYNC_CRAQ_CLASS_H__


namespace CBR
{


class AsyncCraq
{
public:
  AsyncCraq(SpaceContext* spc);
  ~AsyncCraq();

  enum AsyncCraqReqStatus{REQUEST_PROCESSED, REQUEST_NOT_PROCESSED};

  void initialize(std::vector<CraqInitializeArgs>);

  boost::asio::io_service io_service;  //creates an io service

  int set(const CraqDataSetGet& cdSet);
  int get(const CraqDataSetGet& cdGet);

  void runTestOfConnection();
  void runTestOfAllConnections();
  void tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);

  int queueSize();

  Timer mTimer;
  
private:
  
  void processGetResults       (std::vector <CraqOperationResult*> & getRes);
  void processErrorResults     (std::vector <CraqOperationResult*> & errorRes);
  void processTrackedSetResults(std::vector <CraqOperationResult*> & trackedSetRes);
  
  
  std::vector<CraqInitializeArgs> mIpAddPort;
  std::vector<AsyncConnection*> mConnections;
  int mCurrentTrackNum;


  SpaceContext* ctx;
  bool connected;
  CraqDataResponseBuffer mReadData;  
  CraqDataGetResp mReadSomeData;

  std::queue<CraqDataSetGet*> mQueue;
  

  void reInitializeNode(int s);
  void checkConnections(int s);
  
};

}//namespece
  

#endif


