#include <iostream>
#include <iomanip>
#include "Geometry.hpp"
#include <stdlib.h>
#include "Utility.hpp"
#include <cassert>
#include <cmath>
#include <map>



Point::Point(const Point& pt)
  : x(pt.x),
    y(pt.y),
    z(pt.z)
{
}

Point::Point(float a, float b,float c) 
    : x(a),
      y(b),
      z(c)
{
}
Point::Point()
  : x (-1),
    y (-1),
    z (-1)
{
  
}

void Point::printPoint() const
{
  std::cout<<"\n\tX: "<<x<<"   Y: "<<y<<"    Z: "<<z<<"\n";
}



float findDistance(Point* a,Point* b)
{
  float xer = a->x - b->x;
  float yer = a->y - b->y;
  float zer = a->z - b->z;

  float returner = sqrt( xer*xer + yer*yer + zer*zer);

  return returner;
  
}

float findDistance(const Point& a,const Point& b)
{
  float xer = a.x - b.x;
  float yer = a.y - b.y;
  float zer = a.z - b.z;

  float returner = sqrt( xer*xer + yer*yer + zer*zer);

  return returner;
}


float findDistance(const Point& a,Point* b)
{
  float xer = a.x - b->x;
  float yer = a.y - b->y;
  float zer = a.z - b->z;

  float returner = sqrt( xer*xer + yer*yer + zer*zer );

  return returner;

}


float findDistanceSquared(Point* a,Point* b)
{
  float diffX = a->x - b->x;
  float diffY = a->y - b->y;
  float diffZ = a->z - b->z;

  return diffX*diffX + diffY*diffY + diffZ*diffZ;
}


float findDistanceSquared(const Point& a,Point* b)
{
  float diffX = a.x - b->x;
  float diffY = a.y - b->y;
  float diffZ = a.z - b->z;

  return diffX*diffX + diffY*diffY + diffZ*diffZ;
}
