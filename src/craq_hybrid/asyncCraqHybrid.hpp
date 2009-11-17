#include "craq_gets/asyncCraqGet.hpp"
#include "craq_sets/asyncCraqSet.hpp"
#include "asyncCraqUtil.hpp"
#include "../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>

#ifndef  __ASYNC_CRAQ_HYBRID_HPP__
#define  __ASYNC_CRAQ_HYBRID_HPP__

namespace CBR
{

class AsyncCraqHybrid
{
public:
  AsyncCraqHybrid(SpaceContext* con, IOStrand* str);
  ~AsyncCraqHybrid();
  
  
  void initialize(std::vector<CraqInitializeArgs>);

  int set(CraqDataSetGet cdSet);
  int get(CraqDataSetGet cdGet);

  void tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);

  int queueSize();
  int numStillProcessing();
  
private:

  SpaceContext*  ctx;
  IOStrand*  mStrand;
  
  AsyncCraqGet aCraqGet;
  AsyncCraqSet aCraqSet;


  
};

}//end namespace

#endif
