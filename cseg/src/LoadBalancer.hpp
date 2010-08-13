/*  Sirikata
 *  LoadBalancer.hpp
 *
 *  Copyright (c) 2010, Tahir Azim
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

#ifndef _SIRIKATA_LOAD_BALANCER_HPP_
#define _SIRIKATA_LOAD_BALANCER_HPP_


#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/space/SegmentedRegion.hpp>
#include "CSegContext.hpp"

#include "Protocol_CSeg.pbj.hpp"


namespace Sirikata {

class DistributedCoordinateSegmentation;

typedef struct ServerAvailability  {
  ServerID mServer;
  bool mAvailable;

} ServerAvailability;


class LoadBalancer {

public:
  LoadBalancer(DistributedCoordinateSegmentation*, int nservers, const Vector3ui32& perdim);
  ~LoadBalancer();

  void reportRegionLoad(SegmentedRegion* region, ServerID sid, uint32 loadValue);
  void handleSegmentationChange(Sirikata::Protocol::CSeg::ChangeMessage segChangeMessage);

  void service();

  uint32 numAvailableServers() ;

  

private:

  uint32 getAvailableServerIndex();
   
  
  std::vector<SegmentedRegion*> mOverloadedRegionsList;
  std::vector<SegmentedRegion*> mUnderloadedRegionsList;    
  boost::mutex mOverloadedRegionsListMutex;
  boost::mutex mUnderloadedRegionsListMutex;

  std::vector<ServerAvailability> mAvailableServers;

  DistributedCoordinateSegmentation* mCSeg;

};

}
#endif
