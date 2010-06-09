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

