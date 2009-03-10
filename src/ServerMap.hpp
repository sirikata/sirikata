/*  cbr
 *  ServerMap.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_SERVER_MAP_HPP_
#define _CBR_SERVER_MAP_HPP_

#include "Utility.hpp"
#include "LocationService.hpp"
#include "Server.hpp"

namespace CBR {

/* Represents the layout of servers.  Allows you to map objects or positions
 * to the server that handles them.
 */
class ServerMap {
public:
    ////map from xyxmin,  xyxmax,  uvwmin,  uvwmax  to  bandwidth
    typedef std::tr1::function<double(const Vector3d&,const Vector3d&,const Vector3d&, const Vector3d&)> BandwidthFunction;
    ServerMap(LocationService* loc_service, const BandwidthFunction&bandwidthIntegral)
     : mLocationService(loc_service)
      ,mBandwidthCap(bandwidthIntegral)
    {}
    virtual ~ServerMap() {}

    virtual ServerID lookup(const Vector3f& pos) = 0;
    virtual ServerID lookup(const UUID& obj_id) = 0;
    virtual double serverBandwidthRate(ServerID source, ServerID destination)const=0;
protected:

    LocationService* mLocationService;
    BandwidthFunction mBandwidthCap;
};

} // namespace CBR

#endif //_CBR_SERVER_MAP_HPP_
