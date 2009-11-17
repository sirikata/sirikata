#include "craq_gets/asyncCraqGet.hpp"
#include "craq_sets/asyncCraqSet.hpp"
#include "asyncCraqUtil.hpp"
#include "asyncCraqHybrid.hpp"
#include "../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>


namespace CBR
{

AsyncCraqHybrid::AsyncCraqHybrid(SpaceContext* con, IOStrand* str)
  : ctx(con),
    mStrand(str),
    aCraqGet(con,str),
    aCraqSet(con,str)
{

}

void AsyncCraqHybrid::initialize(std::vector<CraqInitializeArgs> initArgs)
{
  aCraqGet.initialize(initArgs);
  aCraqSet.initialize(initArgs);
}

AsyncCraqHybrid::~AsyncCraqHybrid()
{

}


int AsyncCraqHybrid::set(CraqDataSetGet cdSet)
{
  return aCraqSet.set(cdSet);
}

int AsyncCraqHybrid::get(CraqDataSetGet cdGet)
{
  return aCraqGet.get(cdGet);
}


void AsyncCraqHybrid::tick(std::vector<CraqOperationResult*>&allGetResults, std::vector<CraqOperationResult*>&allTrackedResults)
{

  std::vector<CraqOperationResult*> getResults;
  std::vector<CraqOperationResult*> trackedSetResults;
  
  aCraqGet.tick(getResults,trackedSetResults);

  allGetResults.insert(allGetResults.end(), getResults.begin(), getResults.end());
  allTrackedResults.insert(allTrackedResults.end(), trackedSetResults.begin(), trackedSetResults.end());

  
  std::vector<CraqOperationResult*> getResults2;
  std::vector<CraqOperationResult*> trackedSetResults2;

  aCraqSet.tick(getResults2,trackedSetResults2);
  
  allGetResults.insert(allGetResults.end(), getResults2.begin(), getResults2.end());
  allTrackedResults.insert(allTrackedResults.end(),trackedSetResults2.begin(), trackedSetResults2.end());
  
}

int AsyncCraqHybrid::queueSize()
{
  return aCraqSet.queueSize() + aCraqGet.queueSize();
}

int AsyncCraqHybrid::numStillProcessing()
{
  return aCraqSet.numStillProcessing() + aCraqGet.numStillProcessing();
}

}//end namespace




