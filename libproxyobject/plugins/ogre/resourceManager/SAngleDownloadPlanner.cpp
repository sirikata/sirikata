/*  Meru
 *  ResourceDownloadTask.cpp
 *
 *  Copyright (c) 2009, Stanford University
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

#include "SAngleDownloadPlanner.hpp"
#include <sirikata/core/util/SolidAngle.hpp>

namespace Sirikata {

SAngleDownloadPlanner::SAngleDownloadPlanner(Context* c)
 : DistanceDownloadPlanner(c)
{

}

SAngleDownloadPlanner::~SAngleDownloadPlanner()
{

}

bool withinBound(float radius, Vector3d objLoc, Vector3d cameraLoc)
{
    if (cameraLoc.x < objLoc.x - radius || cameraLoc.x > objLoc.x + radius) return false;
    if (cameraLoc.y < objLoc.y - radius || cameraLoc.y > objLoc.y + radius) return false;
    if (cameraLoc.z < objLoc.z - radius || cameraLoc.z > objLoc.z + radius) return false;
    return true;
}


double SAngleDownloadPlanner::calculatePriority(ProxyObjectPtr proxy)
{
    if (camera == NULL) return 0;

    float radius = proxy->getBounds().radius();
    Vector3d objLoc = proxy->getPosition();
    Vector3d cameraLoc = camera->following()->getOgrePosition();

    if (withinBound(radius, objLoc, cameraLoc)) return 0.99;

    Vector3d diff = cameraLoc - objLoc;
    SolidAngle sa = SolidAngle::fromCenterRadius((Vector3f)diff, radius);
    float priority = (sa.asFloat())/(SolidAngle::Max.asFloat());
    return (double)priority;
}
}
