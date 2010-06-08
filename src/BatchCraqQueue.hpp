#ifndef __CBR_BATCH_CRAQ_QUEUE_HPP__
#define __CBR_BATCH_CRAQ_QUEUE_HPP__

#include "ObjectSegmentation.hpp"
#include "Utility.hpp"
#include "Statistics.hpp"
#include "Message.hpp"
#include "Statistics.hpp"
#include "OSegLookupTraceToken.hpp"
#include <queue>

#include <sirikata/core/util/ThreadSafeQueueWithNotification.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>


namespace CBR
{
  class BatchCraqQueue
  {
  private:
    const static int MAX_LENGTH_QUEUE = 15;

    QueryQueue* mQueryQueue;
    ObjectSegmentation* mOSeg;
    IOStrand* mStrand;
    SpaceContext* ctx;
    void internalAddToQueue(Query*);
    void flushQueue();  //push current mQueue to oseg function, and then create a new mQueue.

  public:
    BatchCraqQueue(IOStrand* strand,ObjectSegmentation* oseg, SpaceContext* spc);
    ~BatchCraqQueue();
    void addToQueue(Query*);  //just a wrapper for posting to internalAddToQueue through mStrand

  };
}



#endif
