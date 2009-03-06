/*  cbr
 *  UniformServerMap.cpp
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
#include "Network.hpp"
#include "UniformServerMap.hpp"

namespace CBR {

template<typename T>
T clamp(T val, T minval, T maxval) {
    if (val < minval) return minval;
    if (val > maxval) return maxval;
    return val;
}

UniformServerMap::UniformServerMap(LocationService* loc_service, const BoundingBox3f& region, const Vector3ui32& perside)
 : ServerMap(loc_service),
   mRegion(region),
   mServersPerDim(perside)
{
}

UniformServerMap::~UniformServerMap() {
}

ServerID UniformServerMap::lookup(const Vector3f& pos) {
    Vector3f region_extents = mRegion.extents();
    Vector3f to_point = pos - mRegion.min();

    Vector3i32 server_dim_indices( (int32)((to_point.x/region_extents.x)*mServersPerDim.x),
                                   (int32)((to_point.y/region_extents.y)*mServersPerDim.y),
                                   (int32)((to_point.z/region_extents.z)*mServersPerDim.z) );

    server_dim_indices.x = clamp(server_dim_indices.x, 0, (int32)mServersPerDim.x-1);
    server_dim_indices.y = clamp(server_dim_indices.y, 0, (int32)mServersPerDim.y-1);
    server_dim_indices.z = clamp(server_dim_indices.z, 0, (int32)mServersPerDim.z-1);

    uint32 server_index = (uint32) server_dim_indices.z*mServersPerDim.x*mServersPerDim.y + server_dim_indices.y*mServersPerDim.x + server_dim_indices.x + 1;
    return ServerID(server_index);
}

ServerID UniformServerMap::lookup(const UUID& obj_id) {
    Vector3f pos = mLocationService->currentPosition(obj_id);
    return lookup(pos);
}

} // namespace CBR
