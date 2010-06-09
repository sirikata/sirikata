/*  Sirikata
 *  Geometry.hpp
 *
 *  Copyright (c) 2010, Behram Mistree
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

#include <iostream>
#include <iomanip>
#include "Utility.hpp"
#include <map>

#ifndef __CACHE_GEOMETRY_HPP__
#define __CACHE_GEOMETRY_HPP__

namespace Sirikata
{

  struct Point
  {
    float x,y,z;
    void printPoint() const;
    Point(float a, float b,float c);
    Point(const Point& pt);
    Point();
  };


  struct WorldBoundary
  {
    Point bottomLeft, bottomRight, topLeft, topRight;
  };

  struct WorldDimensions
  {
    std::map<ServerID,Point*> mBidToPoint;
    WorldBoundary wb;
    int blocksWide, blocksTall;
    float blockSideLen;

    BlockID getBlockNum(const Point& pt, bool secondTime=false, const Point* originalPoint = NULL) const;

    bool inWorldX(float positionX) const;
    bool inWorldY(float positionY) const;
    Point randomPoint() const;
    Point* returnCenterOfBlock(BlockID bid);
    ServerID getCenterBlockID() const;
    const Point& getWraparoundPoint(const Point& pt);
    const Point& getWraparoundPoint(Point* pt);
    float getWidth()  const;
    float getHeight() const;

    Point returningPoint;

    ~WorldDimensions(); //need to write this.
  };


  float findDistance(Point* a,Point* b);
  float findDistance(const Point& a,Point* b);
  float findDistance(const Point& a,const Point& b);
  float findDistance(const Point& a,const Point& b, WorldDimensions* wd,bool secondTime =false);
  float findDistance(const Point& a,Point* b, WorldDimensions* wd,bool secondTime =false);

  float findDistanceSquared(const Point& a,const Point& b, WorldDimensions* wd,bool secondTime =false);
  float findDistanceSquared(const Point& a,Point* b, WorldDimensions* wd,bool secondTime =false);
  float findDistanceSquared(Point* a,Point* b);
  float findDistanceSquared(const Point& a,Point* b);

  const static float ONE_KM_IN_METERS  =  1000;
  const static float FIVE_KM_IN_METERS =  5000;
  const static float TEN_KM_IN_METERS  = 10000;
  const static float FIFTEEN_KM_IN_METERS  = 15000;
}

#endif
