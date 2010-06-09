/*  cbr
 *  WorldPopulationBSPTree.hpp
 *
 *  Copyright (c) 2009, Tahir Azim
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

#ifndef _SIRIKATA_WORLD_POPULATION_BSPTREE_HPP_
#define _SIRIKATA_WORLD_POPULATION_BSPTREE_HPP_

#include "Utility.hpp"

#define NUM_HISTOGRAM_BINS 50
#define WORLD_SURFACE_AREA 510072000.0
#define MIN_REGION_DENSITY_CUTOFF 1

namespace Sirikata {

typedef struct WorldRegion {
  BoundingBox3f mBoundingBox;
  double density;

  WorldRegion() {
    density = 0;
    mBoundingBox = BoundingBox3f();
  }

  WorldRegion(const WorldRegion& wr) {
    density = wr.density;
    mBoundingBox = wr.mBoundingBox;
  }

} WorldRegion;

/** Read in world population data from a file and create a BSP tree. */
class WorldPopulationBSPTree {
public:
  WorldPopulationBSPTree();

  
  void setupRegionBoundaries(WorldRegion* regionList);
  

  void constructBSPTree(SegmentedRegion& topLevelRegion);

private:
  void constructBSPTree(SegmentedRegion& bspTree, WorldRegion* regionList, int listLength,
			bool makeHorizontalCut, int depth);

  String mFileName;

  int mMaxPeopleInLeaf;
  int mWorldWidth;
  int mWorldHeight;
  int mNumRegions;
  int mTotalLeaves;

  int mBiggestDepth;
  BoundingBox3f mRectangle1;
  BoundingBox3f mRectangle2;

  BoundingBox3f mIntersect1;
  BoundingBox3f mIntersect2;

  

  int* mHistogram;

  uint32 mInitialSpaceServerCount;

  float32 mCellEdgeWidth;
};

}


#endif
