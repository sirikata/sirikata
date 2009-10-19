#include "craq_gets/asyncCraqGet.hpp"
#include "craq_sets/asyncCraqSet.hpp"
#include "asyncCraqUtil.hpp"
#include "asyncCraqHybrid.hpp"


namespace CBR
{

AsyncCraqHybrid::AsyncCraqHybrid()
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


int AsyncCraqHybrid::set(StreamCraqDataSetGet cdSet)
{
  return aCraqSet.set(cdSet);
}

int AsyncCraqHybrid::get(StreamCraqDataSetGet cdGet)
{
  return aCraqGet.get(cdGet);
}


void AsyncCraqHybrid::tick(std::vector<StreamCraqOperationResult*>&allGetResults, std::vector<StreamCraqOperationResult*>&allTrackedResults)
{

  std::vector<StreamCraqOperationResult*> getResults;
  std::vector<StreamCraqOperationResult*> trackedSetResults;
  
  aCraqGet.tick(getResults,trackedSetResults);

  allGetResults.insert(allGetResults.end(), getResults.begin(), getResults.end());
  allTrackedResults.insert(allTrackedResults.end(), trackedSetResults.begin(), trackedSetResults.end());

  
  std::vector<StreamCraqOperationResult*> getResults2;
  std::vector<StreamCraqOperationResult*> trackedSetResults2;

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




