
#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "../asyncCraqUtil.hpp"
#include "asyncConnectionSet.hpp"
#include "../../SpaceContext.hpp"
#include <sirikata/core/network/IOStrandImpl.hpp>
#include "../asyncCraqScheduler.hpp"
#include <sirikata/core/network/Asio.hpp>

#ifndef __ASYNC_CRAQ_SET_CLASS_H__
#define __ASYNC_CRAQ_SET_CLASS_H__


namespace CBR
{

class AsyncCraqSet : public AsyncCraqScheduler
  {
  public:

    AsyncCraqSet(SpaceContext* con, IOStrand* strand_this_runs_on, IOStrand* strand_to_post_results_to, ObjectSegmentation* parent_oseg_called);
    ~AsyncCraqSet();


    void initialize(std::vector<CraqInitializeArgs>);

    void set(CraqDataSetGet cdSet, uint64 tracking_number = 0);


    int queueSize();
    int numStillProcessing();

    virtual void erroredGetValue(CraqOperationResult* cor);
    virtual void erroredSetValue(CraqOperationResult* cor);
    virtual void stop();

  private:

    std::vector<CraqInitializeArgs> mIpAddPort;
    std::vector<AsyncConnectionSet*> mConnections;
    std::vector<IOStrand*>mConnectionsStrands;


    bool connected;

    std::queue<CraqDataSetGet> mQueue;

    void reInitializeNode(int s);
    ///return false if the connection is not indeed ready
    bool checkConnections(int s);
    void readyStateChanged(int s);
    void pushQueue(const CraqDataSetGet&dataToSet);
    std::vector<int> mReadyConnections;
    SpaceContext*                    ctx;
    IOStrand*                    mStrand;
    IOStrand*             mResultsStrand;
    ObjectSegmentation*            mOSeg;

  };

}//end namespace


#endif
