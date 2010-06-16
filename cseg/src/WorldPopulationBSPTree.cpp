/*  Sirikata
 *  WorldPopulationBSPTree.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include "DistributedCoordinateSegmentation.hpp"
#include "WorldPopulationBSPTree.hpp"
#include <sirikata/cbrcore/Options.hpp>
#include <boost/tokenizer.hpp>

namespace Sirikata {


BoundingBox3f intersect(const BoundingBox3f& b1, const BoundingBox3f& b2) {
  float x1 = (b1.min().x > b2.min().x) ? b1.min().x : b2.min().x;
  float y1 = (b1.min().y > b2.min().y) ? b1.min().y : b2.min().y;
  float z1 = (b1.min().z > b2.min().z) ? b1.min().z : b2.min().z;

  float x2 = (b1.max().x < b2.max().x) ? b1.max().x : b2.max().x;
  float y2 = (b1.max().y < b2.max().y) ? b1.max().y : b2.max().y;
  float z2 = (b1.max().z < b2.max().z) ? b1.max().z : b2.max().z;


  return BoundingBox3f( Vector3f(x1,y1,z1), Vector3f(x2,y2,z2) );
}

bool intersects(const BoundingBox3f& bbox1, const BoundingBox3f& bbox2) {
  bool b1=false, b2=false;

  b1 =   (bbox2.min().x <= bbox1.min().x && bbox1.min().x <= bbox2.max().x)
      && (bbox2.min().y <= bbox1.min().y && bbox1.min().y <= bbox2.max().y);

  if (b1) return b1;

  b2 =   (bbox2.min().x <= bbox1.max().x && bbox1.max().x <= bbox2.max().x)
      && (bbox2.min().y <= bbox1.max().y && bbox1.max().y <= bbox2.max().y);

  if (b2) return b2;

  b1 =   (bbox1.min().x <= bbox2.min().x && bbox2.min().x <= bbox1.max().x)
      && (bbox1.min().y <= bbox2.min().y && bbox2.min().y <= bbox1.max().y);

  if (b1) return b1;

  b2 =   (bbox1.min().x <= bbox2.max().x && bbox2.max().x <= bbox1.max().x)
      && (bbox1.min().y <= bbox2.max().y && bbox2.max().y <= bbox1.max().y);

  if (b2) return b2;

  return false;
}

void WorldPopulationBSPTree::setupRegionBoundaries(WorldRegion* regionList) {
  for (int i=0; i<mWorldHeight; i++) {
    for (int j=0; j<mWorldWidth; j++) {
      regionList[i*mWorldWidth+j].mBoundingBox =
	BoundingBox3f( Vector3f(j*mCellEdgeWidth, i*mCellEdgeWidth,0),
		       Vector3f(j*mCellEdgeWidth + mCellEdgeWidth, i*mCellEdgeWidth + mCellEdgeWidth,0) );
      }
    }
}


void WorldPopulationBSPTree::constructBSPTree(SegmentedRegion& bspTree, WorldRegion* regionList, int listLength, bool makeHorizontalCut, int depth)
{
  SegmentedRegion* bspTree1 = new SegmentedRegion();
  SegmentedRegion* bspTree2 = new SegmentedRegion();

  if (depth > mBiggestDepth) {
    mBiggestDepth = depth;
    std::cout << "mBiggestDepth=" << mBiggestDepth << "\n";
  }

  if (makeHorizontalCut) {
    bspTree1->mBoundingBox =
                  BoundingBox3f(Vector3f(bspTree.mBoundingBox.min().x, bspTree.mBoundingBox.min().y, 0),
		  Vector3f(bspTree.mBoundingBox.max().x, (bspTree.mBoundingBox.min().y+bspTree.mBoundingBox.max().y)/2.0, 0));

    bspTree2->mBoundingBox =
      BoundingBox3f(Vector3f(bspTree.mBoundingBox.min().x, (bspTree.mBoundingBox.min().y+bspTree.mBoundingBox.max().y)/2.0, 0),
		    Vector3f(bspTree.mBoundingBox.max().x, bspTree.mBoundingBox.max().y, 0));
  }
  else {
    bspTree1->mBoundingBox =
      BoundingBox3f(Vector3f(bspTree.mBoundingBox.min().x, bspTree.mBoundingBox.min().y, 0),
		    Vector3f((bspTree.mBoundingBox.min().x+bspTree.mBoundingBox.max().x)/2.0, bspTree.mBoundingBox.max().y, 0));

    bspTree2->mBoundingBox =
      BoundingBox3f(Vector3f((bspTree.mBoundingBox.min().x+bspTree.mBoundingBox.max().x)/2.0, bspTree.mBoundingBox.min().y, 0),
		    Vector3f(bspTree.mBoundingBox.max().x, bspTree.mBoundingBox.max().y, 0));
  }

  bool split1 =false, split2 = false;
  double sum1 = 0;
  double sum2 = 0;

  int  i =0;
  int idx1=0, idx2=0;

  WorldRegion* tempRegionList1 = new WorldRegion[listLength];
  WorldRegion* tempRegionList2 = new WorldRegion[listLength];

  for (i=0; i<  listLength; i++) {
    if (regionList[i].density == 0) continue;

    if (intersects(bspTree1->mBoundingBox, regionList[i].mBoundingBox) ) {

      mIntersect1 = intersect(bspTree1->mBoundingBox, regionList[i].mBoundingBox);
      sum1 += regionList[i].density * mIntersect1.across().x * mIntersect1.across().y;
      tempRegionList1[idx1++] = regionList[i];
    }
    if (intersects(bspTree2->mBoundingBox, regionList[i].mBoundingBox) ) {

      mIntersect2 = intersect(bspTree2->mBoundingBox, regionList[i].mBoundingBox);
      sum2 += regionList[i].density * mIntersect2.across().x * mIntersect2.across().y;
      tempRegionList2[idx2++] = regionList[i];
    }

    if (sum1 > mMaxPeopleInLeaf) {
      split1 = true;
    }
    if (sum2 > mMaxPeopleInLeaf) {
      split2 = true;
    }
  }

  WorldRegion* regionList1 = new WorldRegion[idx1];
  WorldRegion* regionList2 = new WorldRegion[idx2];

  for (i=0; i<idx1; i++) {
    regionList1[i] = tempRegionList1[i];
  }
  for (i=0; i<idx2; i++) {
    regionList2[i] = tempRegionList2[i];
  }

  delete tempRegionList1;
  delete tempRegionList2;

  if (split1 || split2) {
    bspTree.mLeftChild = bspTree1;
    constructBSPTree(*bspTree1, regionList1, idx1, !makeHorizontalCut, depth+1);

    bspTree.mRightChild = bspTree2;
    constructBSPTree(*bspTree2, regionList2, idx2, !makeHorizontalCut, depth+1);
  }

  if (!split1 && !split2) {
    bspTree.mServer = (mTotalLeaves % mInitialSpaceServerCount) + 1;
    ++mTotalLeaves;
    mHistogram[depth]++;
  }



  delete regionList1;
  delete regionList2;
}

WorldPopulationBSPTree::WorldPopulationBSPTree()
  : mFileName(GetOption("cseg-population-density-file")->as<String>()),
    mMaxPeopleInLeaf(GetOption("cseg-max-leaf-population")->as<uint32>())
{
  mWorldWidth = GetOption("cseg-world-width")->as<uint32>();
  mWorldHeight = GetOption("cseg-world-height")->as<uint32>();
  mBiggestDepth = 0;
  mNumRegions = mWorldWidth * mWorldHeight;

  Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();
  mInitialSpaceServerCount = layout.x * layout.y * layout.z;

  mCellEdgeWidth = sqrt(WORLD_SURFACE_AREA/(mWorldWidth*mWorldHeight));
}

void WorldPopulationBSPTree::constructBSPTree(SegmentedRegion& topLevelRegion) {
  mBiggestDepth = 0;
  mTotalLeaves = 0;

  double i =0, sum=0, largestdensity = 0;
  int j=0, k=0;

  WorldRegion* regionList = new WorldRegion[mNumRegions];

  std::ifstream filestream(mFileName.c_str());

  while(!filestream.bad() && !filestream.fail() && !filestream.eof()) {
    char line[1048576];
    filestream.getline(line,1048576);

    boost::char_separator<char> sep(",: ");
    boost::tokenizer<boost::char_separator<char> > tokens(String(line), sep);
    for(boost::tokenizer<boost::char_separator<char> >::iterator beg=tokens.begin();
	beg!=tokens.end();++beg)
      {
	String token = *beg;

	if (token != "-9999") {
	  double val = atof(token.c_str());
	  regionList[j].density = val;
	  if (val > largestdensity) {
	    largestdensity = val;
	  }
	}
	else {
	  regionList[j].density = 0;
	}

	j++;
      }
  }

  mHistogram = new int[NUM_HISTOGRAM_BINS];
  memset(mHistogram, 0 , NUM_HISTOGRAM_BINS * sizeof(int));

  setupRegionBoundaries(regionList);

  int mindensity = MIN_REGION_DENSITY_CUTOFF;
  j=0;
  std::vector<WorldRegion> regionVector;
  for (k=0; k<mNumRegions; k++) {
    if (regionList[k].density > mindensity) {
      regionVector.push_back(regionList[k]);
      j++;
    }
  }

  WorldRegion* regionList2 = new WorldRegion[regionVector.size()];
  for (k=0; k < (int)regionVector.size(); k++) {
    regionList2[k] = regionVector[k];
  }

  regionVector.clear();

  delete regionList;

  topLevelRegion.mBoundingBox = BoundingBox3f(
				    Vector3f(0,0,0),
 			            Vector3f(mWorldWidth*mCellEdgeWidth, mWorldHeight*mCellEdgeWidth, 0));

  constructBSPTree(topLevelRegion, regionList2, j, false, 1);

  printf("total_leaves=%d\n", mTotalLeaves);
  for (j = 0 ; j < NUM_HISTOGRAM_BINS; j++) {
    std::cout << j << "," << mHistogram[j] << "\n";
  }

  delete regionList2;

  delete mHistogram;

  fflush(stdout);
}

}
