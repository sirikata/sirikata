#include "craq_gets/asyncCraqGet.hpp"
#include "craq_sets/asyncCraqSet.hpp"
#include "asyncCraqUtil.hpp"
#include "asyncCraqHybrid.hpp"
#include "../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include "../ObjectSegmentation.hpp"

namespace CBR
{

  AsyncCraqHybrid::AsyncCraqHybrid(SpaceContext* con, IOStrand* strand_to_post_results_to, ObjectSegmentation* oseg)
  : ctx(con),
    mGetStrand(con->ioService->createStrand()),
    mSetStrand(con->ioService->createStrand()),
    aCraqGet(con,mGetStrand,strand_to_post_results_to,oseg),
    aCraqSet(con,mSetStrand,strand_to_post_results_to,oseg)
  {
  }

  void AsyncCraqHybrid::initialize(std::vector<CraqInitializeArgs> initArgs)
  {
    mGetStrand->post(std::tr1::bind(&AsyncCraqGet::initialize,&aCraqGet,initArgs));
    mSetStrand->post(std::tr1::bind(&AsyncCraqSet::initialize,&aCraqSet,initArgs));
  }

  void AsyncCraqHybrid::stop()
  {
    //    mGetStrand->post(std::tr1::bind(&AsyncCraqGet::stop,&aCraqGet));
    //    mSetStrand->post(std::tr1::bind(&AsyncCraqSet::stop,&aCraqSet));
  }

  AsyncCraqHybrid::~AsyncCraqHybrid()
  {
    delete mGetStrand;
    delete mSetStrand;
  }


  void AsyncCraqHybrid::set(CraqDataSetGet cdSet, uint64 trackingNumber)
  {
    mSetStrand->post(std::tr1::bind(&AsyncCraqSet::set,&aCraqSet,cdSet, trackingNumber));
  }

  void AsyncCraqHybrid::get(CraqDataSetGet cdGet, OSegLookupTraceToken* traceToken)
  {
    mGetStrand->post(std::tr1::bind(&AsyncCraqGet::get,&aCraqGet, cdGet, traceToken));
  }

  int AsyncCraqHybrid::queueSize()
  {
    return aCraqSet.queueSize() + aCraqGet.queueSize();
  }

  int AsyncCraqHybrid::numStillProcessing()
  {
    return aCraqSet.numStillProcessing() + aCraqGet.numStillProcessing();
  }

  std::vector <PollingService*> AsyncCraqHybrid::getPollingServices()
  {
    std::vector<PollingService*> returner;
    returner.push_back(&aCraqSet);
    returner.push_back(&aCraqGet);
    return returner;
  }

}//end namespace
