#include "craq_gets/asyncCraqGet.hpp"
#include "craq_sets/asyncCraqSet.hpp"
#include "asyncCraqUtil.hpp"

#ifndef  __ASYNC_CRAQ_HYBRID_HPP__
#define  __ASYNC_CRAQ_HYBRID_HPP__

namespace CBR
{

class AsyncCraqHybrid
{
public:
  AsyncCraqHybrid();
  ~AsyncCraqHybrid();
  
  
  void initialize(std::vector<CraqInitializeArgs>);

  int set(CraqDataSetGet cdSet);
  int get(CraqDataSetGet cdGet);

  void tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);

  int queueSize();
  int numStillProcessing();
  
private:

  AsyncCraqGet aCraqGet;
  AsyncCraqSet aCraqSet;

};

}//end namespace

#endif
