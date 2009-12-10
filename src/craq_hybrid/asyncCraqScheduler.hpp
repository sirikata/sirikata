
#ifndef __ASYNC_CRAQ_SCHEDULER__
#define __ASYNC_CRAQ_SCHEDULER__

#include "asyncCraqUtil.hpp"


namespace CBR
{

  class AsyncCraqScheduler
  {

  public:

    virtual void erroredGetValue(CraqOperationResult* cor) = 0; //responsible for freeing as well.
    virtual void erroredSetValue(CraqOperationResult* cor) = 0; //responsible for freeing as well.  

    virtual ~AsyncCraqScheduler()
    {
    }
    
  };

} //end namespace
#endif
