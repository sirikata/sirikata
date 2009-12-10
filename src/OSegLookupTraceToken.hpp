
#ifndef __OSEG_LOOKUP_TRACE_TOKEN__
#define __OSEG_LOOKUP_TRACE_TOKEN__

#include "Utility.hpp"

namespace CBR
{


  struct OSegLookupTraceToken
  {
    OSegLookupTraceToken(const UUID& uID);
    OSegLookupTraceToken();
    
    /*****Additional***/
    UUID mID;
    ServerID lookerUpper;
    ServerID locatedOn;

    
    /********FLAGS********/
    //AsyncConnectionGet::get
    bool notReady;				
    //Lots of places
    bool shuttingDown;			
    //AsyncConnectionGet::queryTimeout
    bool deadlineExpired;			
    //AsyncConnectionGet::processNotFound
    bool notFound;				


    /***TIMES****/
    //CraqObjectSegmentation::get
    uint64 initialLookupTime;		      

    //CraqObjectSegmentation::get
    uint64 checkCacheLocalBegin;			      

    //CraqObjectSegmentation::get
    uint64 checkCacheLocalEnd;

    //CraqObjectSegmentation::beginning of beginCraqLookup //anything that gets to beginCraqLookupGets to here
    uint64 craqLookupBegin;

    //CraqObjectSegmentation::end of beginCraqLookup
    uint64 craqLookupEnd;

    //CraqObjectSegmentation::beginCraqLookup//wasn't already being looked up begin
    uint64 craqLookupNotAlreadyLookingUpBegin;

    //CraqObjectSegmentation::beginCraqLookup
    uint64 craqLookupNotAlreadyLookingUpEnd;		  

    //AsyncCraqGet::get
    uint64 getManagerEnqueueBegin;

    //AsyncCraqGet::get
    uint64 getManagerEnqueueEnd;	

    //AsyncCraqGet::checkConnections
    uint64 getManagerDequeued;

    //AsyncConnectionGet::get
    uint64 getConnectionNetworkGetBegin;			      

    //AsyncConnectionGet::get
    uint64 getConnectionNetworkGetEnd;			   

    //AsyncConnectionGet::processValueFound
    uint64 getConnectionNetworkReceived;

    //CraqObjectSegmentation:craqGetResult
    uint64 lookupReturnBegin;
    
    //CraqObjectSegmentation:craqGetResult
    uint64 lookupReturnEnd;			    
  };
}

#endif
