
#include "OSegLookupTraceToken.hpp"
#include "Utility.hpp"

namespace CBR
{

  OSegLookupTraceToken::OSegLookupTraceToken()
  {
    lookerUpper = NullServerID;
    locatedOn   = NullServerID;
    
    
    notReady          = false;
    shuttingDown      = false;
    deadlineExpired   = false;
    notFound          = false;

    initialLookupTime                   =  0;
    checkCacheLocalBegin                =  0;
    checkCacheLocalEnd                  =  0;
    craqLookupBegin                     =  0;
    craqLookupEnd                       =  0;
    craqLookupNotAlreadyLookingUpBegin  =  0;
    craqLookupNotAlreadyLookingUpEnd    =  0;
    getManagerEnqueueBegin              =  0;
    getManagerEnqueueEnd                =  0;
    getManagerDequeued                  =  0;
    getConnectionNetworkGetBegin        =  0;
    getConnectionNetworkGetEnd          =  0;
    getConnectionNetworkReceived        =  0;
    lookupReturnBegin                   =  0;
    lookupReturnEnd                     =  0;
  }
  OSegLookupTraceToken::OSegLookupTraceToken(const UUID& uID)
  {
    mID         =          uID;
    lookerUpper = NullServerID;
    locatedOn   = NullServerID;
    
    
    notReady          = false;
    shuttingDown      = false;
    deadlineExpired   = false;
    notFound          = false;

    initialLookupTime                   =  0;
    checkCacheLocalBegin                =  0;
    checkCacheLocalEnd                  =  0;
    craqLookupBegin                     =  0;
    craqLookupEnd                       =  0;
    craqLookupNotAlreadyLookingUpBegin  =  0;
    craqLookupNotAlreadyLookingUpEnd    =  0;
    getManagerEnqueueBegin              =  0;
    getManagerEnqueueEnd                =  0;
    getManagerDequeued                  =  0;
    getConnectionNetworkGetBegin        =  0;
    getConnectionNetworkGetEnd          =  0;
    getConnectionNetworkReceived        =  0;
    lookupReturnBegin                   =  0;
    lookupReturnEnd                     =  0;
  }
  

}
