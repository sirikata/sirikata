/*  cbr
 *  UniformCoordinateSegmentation.cpp
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

#include "UniformCoordinateSegmentation.hpp"

#include <algorithm>
#include <boost/tokenizer.hpp>
#include "Options.hpp"



namespace CBR {

template<typename T>
T clamp(T val, T minval, T maxval) {
    if (val < minval) return minval;
    if (val > maxval) return maxval;
    return val;
}

UniformCoordinateSegmentation::UniformCoordinateSegmentation(SpaceContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim)
 : CoordinateSegmentation(ctx),
   mRegion(region),
   mServersPerDim(perdim)
{
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_CSEG_CHANGE, this);

  /* Read in the file which maintains how the layout of the region
     changes at different times. */
  std::ifstream infile("layoutChangeFile.txt");

  lastLayoutChangeIdx = 0;



  int  i = 0;
  std::string str;
  while (getline(infile, str)) {


    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(" ");
    tokenizer tokens(str, sep);


    tokenizer::iterator tok_iter = tokens.begin();


    uint32 time_val = atoi((*tok_iter).c_str());


    ++tok_iter;


    std::string layoutAsStr = *tok_iter;

    Vector3ui32 vec;

    std::istringstream is(layoutAsStr);
    is >> vec;

    LayoutChangeEntry lce;
    lce.time = time_val;
    lce.layout = vec;

    mLayoutChangeEntries.push_back(lce);
  }


  infile.close();

}

UniformCoordinateSegmentation::~UniformCoordinateSegmentation() {
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_CSEG_CHANGE, this);
}

ServerID UniformCoordinateSegmentation::lookup(const Vector3f& pos)  {
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

BoundingBoxList UniformCoordinateSegmentation::serverRegion(const ServerID& server)  {
    BoundingBoxList boundingBoxList;
    //assert( server > 0 && server <= mServersPerDim.x * mServersPerDim.y * mServersPerDim.z );
    if ( server > mServersPerDim.x * mServersPerDim.y * mServersPerDim.z ) {
      boundingBoxList.push_back( BoundingBox3f( Vector3f(0,0,0),Vector3f(0,0,0)) );
      return boundingBoxList;
    }

    ServerID sid = server - 1;
    Vector3i32 server_dim_indices(sid%mServersPerDim.x,
                                  (sid/mServersPerDim.x)%mServersPerDim.y,
                                  (sid/mServersPerDim.x/mServersPerDim.y)%mServersPerDim.z);
    double xsize=mRegion.extents().x/mServersPerDim.x;
    double ysize=mRegion.extents().y/mServersPerDim.y;
    double zsize=mRegion.extents().z/mServersPerDim.z;
    Vector3f region_min(server_dim_indices.x*xsize+mRegion.min().x,
                        server_dim_indices.y*ysize+mRegion.min().y,
                        server_dim_indices.z*zsize+mRegion.min().z);
    Vector3f region_max((1+server_dim_indices.x)*xsize+mRegion.min().x,
                        (1+server_dim_indices.y)*ysize+mRegion.min().y,
                        (1+server_dim_indices.z)*zsize+mRegion.min().z);

    boundingBoxList.push_back(BoundingBox3f(region_min, region_max));

    return boundingBoxList;
}

BoundingBox3f UniformCoordinateSegmentation::region()  {
    return mRegion;
}

uint32 UniformCoordinateSegmentation::numServers()  {
    return mServersPerDim.x * mServersPerDim.y * mServersPerDim.z;
}

void UniformCoordinateSegmentation::service() {
  /* Short-circuited the code for changing the layout at run-time for now
     but its been tested and it works.
   */
  return;

  Time t = mContext->simTime();

  /* The following code changes the segmentation/layout of the region.*/

  for (uint32 i=lastLayoutChangeIdx; i< mLayoutChangeEntries.size(); i++) {
    LayoutChangeEntry lce = mLayoutChangeEntries[i];

    if (  t.raw() >= lce.time ) {

      printf("Changing layout to %s\n", lce.layout.toString().c_str());
      printf("lce.time=%ld, t.raw()=%ld\n", lce.time, t.raw() );

      lastLayoutChangeIdx = i+1;

      mServersPerDim = lce.layout;

      uint32 countServers = numServers();

      std::vector<Listener::SegmentationInfo> segInfoVector;

      for (uint32 j = 1; j <= countServers; j++) {
        Listener::SegmentationInfo segInfo;
        segInfo.server = j;
        segInfo.region = serverRegion(j);
        segInfoVector.push_back( segInfo );
      }

      notifyListeners(segInfoVector);

      break;
    }
  }
}

void UniformCoordinateSegmentation::receiveMessage(Message* msg) {
    // FIXME what are we doing with these
    assert(msg->dest_port() == SERVER_PORT_CSEG_CHANGE);
    delete msg;
}

} // namespace CBR
