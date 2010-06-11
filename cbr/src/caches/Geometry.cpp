/*  Sirikata
 *  Geometry.cpp
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
