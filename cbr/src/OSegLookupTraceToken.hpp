/*  Sirikata
 *  OSegLookupTraceToken.hpp
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


#ifndef __OSEG_LOOKUP_TRACE_TOKEN__
#define __OSEG_LOOKUP_TRACE_TOKEN__

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>

namespace Sirikata
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

    uint64 osegQLenPostQuery;

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

    uint64 osegQLenPostReturn;

    //CraqObjectSegmentation:craqGetResult
    uint64 lookupReturnBegin;

    //CraqObjectSegmentation:craqGetResult
    uint64 lookupReturnEnd;

    void printCumulativeTraceToken();
  };
}

#endif
