
#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "../asyncCraqUtil.hpp"
#include "asyncConnectionGet.hpp"

#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>

#ifndef __ASYNC_CRAQ_GET_CLASS_H__
#define __ASYNC_CRAQ_GET_CLASS_H__

namespace CBR
{


class AsyncCraqGet
{
public:
  AsyncCraqGet(SpaceContext* ctx, IOStrand* str);
  ~AsyncCraqGet();
  
  int runReQuery();
  
  enum AsyncCraqReqStatus{REQUEST_PROCESSED, REQUEST_NOT_PROCESSED};

  void initialize(std::vector<CraqInitializeArgs>);

  boost::asio::io_service io_service;  //creates an io service

  int set(const CraqDataSetGet& cdSet);
  int get(const CraqDataSetGet& cdGet);

  void runTestOfConnection();
  void runTestOfAllConnections();
  void tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);

  int queueSize();
  int numStillProcessing();

  int getMulti(CraqDataSetGet& dataToGet);
  void tickMulti(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);
  int getRespCount();
  
private:
  
  void processGetResults       (std::vector <CraqOperationResult*> & getRes);
  void processErrorResults     (std::vector <CraqOperationResult*> & errorRes);
  void processTrackedSetResults(std::vector <CraqOperationResult*> & trackedSetRes);

  void straightPoll();
  std::vector<CraqInitializeArgs> mIpAddPort;
  std::vector<AsyncConnectionGet*> mConnections;
  int mCurrentTrackNum;
  bool connected;

  std::queue<CraqDataSetGet*> mQueue;
  

  void reInitializeNode(int s);
  void checkConnections(int s);
  void checkConnectionsMulti(int s);


  SpaceContext* ctx;
  IOStrand* mStrand;
  
};


}//end namespace

#endif


