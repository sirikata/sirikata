#include "craq_gets/asyncCraqGet.hpp"
#include "craq_sets/asyncCraqSet.hpp"
#include "asyncCraqUtil.hpp"
#include "../SpaceContext.hpp"
#include <sirikata/core/network/IOStrandImpl.hpp>
#include "asyncCraqScheduler.hpp"
#include "../OSegLookupTraceToken.hpp"

#ifndef  __ASYNC_CRAQ_HYBRID_HPP__
#define  __ASYNC_CRAQ_HYBRID_HPP__

namespace Sirikata
{

  class AsyncCraqHybrid
{
public:
  AsyncCraqHybrid(SpaceContext* con, IOStrand* strand_to_post_results_to, ObjectSegmentation* oseg);
  ~AsyncCraqHybrid();

  void initialize(std::vector<CraqInitializeArgs>);

  void set(CraqDataSetGet cdSet, uint64 tracking_number = 0);
  void get(CraqDataSetGet cdGet, OSegLookupTraceToken* traceToken);

  int queueSize();
  int numStillProcessing();

  void stop();

private:

  SpaceContext*  ctx;

  IOStrand* mGetStrand;
  IOStrand* mSetStrand;

  AsyncCraqGet aCraqGet;
  AsyncCraqSet aCraqSet;



};

}//end namespace

#endif
