#include "OSegLookupTraceToken.hpp"
#include "Utility.hpp"
#include <iostream>
#include <iomanip>

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

  
  void OSegLookupTraceToken::printCumulativeTraceToken()
  {
    std::cout<<"\n\n";
    std::cout<<"ID: \t\t"<<mID.toString()<<"\n";
    std::cout<<"not ready: \t\t"<<notReady<<"\n";
    std::cout<<"shuttingDown: \t\t"<<shuttingDown<<"\n";
    std::cout<<"notFound:\t\t"<<notFound<<"\n";
    std::cout<<"initialLookupTime:\t\t"<<initialLookupTime<<"\n";
    std::cout<<"checkCacheLocalBeing:\t\t"<<checkCacheLocalBegin<<"\n";
    std::cout<<"checkCacheLocalEnd:\t\t"<<checkCacheLocalEnd<<"\n";
    std::cout<<"craqLookupBegin:\t\t"<<craqLookupBegin<<"\n";
    std::cout<<"craqLookupEnd:\t\t"<<craqLookupEnd<<"\n";
    std::cout<<"craqLookupNotAlreadyLookingUpBegin:\t\t"<<craqLookupNotAlreadyLookingUpBegin<<"\n";
    std::cout<<"craqLookupNotAlreadyLookingUpEnd:\t\t"<<craqLookupNotAlreadyLookingUpEnd<<"\n";
    std::cout<<"getManagerEnqueueBegin:\t\t"<<getManagerEnqueueBegin<<"\n";
    std::cout<<"getManagerEnqueueEnd:\t\t"<<getManagerEnqueueEnd<<"\n";
    std::cout<<"getManagerDequeued:\t\t"<<getManagerDequeued<<"\n";
    std::cout<<"getConnectionNetworkGetBegin:\t\t"<<getConnectionNetworkGetBegin<<"\n";
    std::cout<<"getConnectionNetworkGetEnd:\t\t"<<getConnectionNetworkGetEnd<<"\n";
    std::cout<<"getConnectionNetworkReceived:\t\t"<<getConnectionNetworkReceived<<"\n";
    std::cout<<"lookupReturnBegin:\t\t"<<lookupReturnBegin<<"\n";
    std::cout<<"lookupReturnEnd:\t\t"<<lookupReturnEnd<<"\n";
    std::cout<<"\n\n";
  }

}
