
#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "../asyncCraqUtil.hpp"
#include "asyncConnectionGet.hpp"

#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include <sirikata/network/Asio.hpp>
#include "../../OSegLookupTraceToken.hpp"


#ifndef __ASYNC_CRAQ_GET_CLASS_H__
#define __ASYNC_CRAQ_GET_CLASS_H__

namespace CBR
{


  class AsyncCraqGet : public AsyncCraqScheduler,
                       public PollingService
  {
  private:

    struct QueueValue
    {
      CraqDataSetGet* cdQuery;
      OSegLookupTraceToken* traceToken;
    };

    
  public:
    AsyncCraqGet(SpaceContext* con, IOStrand* strand_this_runs_on, IOStrand* strand_to_post_results_to, ObjectSegmentation* parent_oseg_called);
    ~AsyncCraqGet();
    
    int runReQuery();
    void initialize(std::vector<CraqInitializeArgs>);

    virtual void erroredGetValue(CraqOperationResult* cor);
    virtual void erroredSetValue(CraqOperationResult* cor);

  
    void get(const CraqDataSetGet& cdGet, OSegLookupTraceToken* traceToken);

    int queueSize();
    int numStillProcessing();
    int getRespCount();
    virtual void stop();
    virtual void poll();
    
  private:

    void straightPoll();
    std::vector<CraqInitializeArgs> mIpAddPort;
    std::vector<AsyncConnectionGet*> mConnections;
    std::vector<IOStrand*> mConnectionsStrands;



    std::queue<QueueValue*>mQueue;

    void reInitializeNode(int s);
    void readyStateChanged(int s);
    bool checkConnections(int s);
    void pushQueue(QueueValue*qv);
    std::vector<int> mReadyConnections;

    SpaceContext* ctx;
    IOStrand* mStrand;        //strand that the asyncCraqGet is running on.
    IOStrand* mResultsStrand; //strand that we post our results to.
    ObjectSegmentation* mOSeg;
  
  };


}//end namespace

#endif


