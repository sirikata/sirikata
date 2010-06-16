/*  Sirikata
 *  OSegLookupTraceToken.cpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/cbrcore/OSegLookupTraceToken.hpp>
#include <sirikata/cbrcore/Utility.hpp>

#include <iostream>
#include <iomanip>

namespace Sirikata
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

    osegQLenPostReturn                  = 1000;
    osegQLenPostQuery                   = 1000;
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

    osegQLenPostReturn                  = 1000;
    osegQLenPostQuery                   = 1000;

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
    std::cout<<"osegQLenPostReturn:\t\t"<<osegQLenPostReturn<<"\n";
    std::cout<<"osegQLenPostQuery:\t\t"<<osegQLenPostQuery<<"\n";
    std::cout<<"\n\n";
  }

}
