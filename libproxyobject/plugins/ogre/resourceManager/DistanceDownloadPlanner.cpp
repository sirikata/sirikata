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


#include "DistanceDownloadPlanner.hpp"
#include <sirikata/ogre/Entity.hpp>
#include "SAngleDownloadPlanner.hpp"
#include <stdlib.h>
#include <algorithm>
#include <math.h>

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;
using namespace Sirikata::Graphics;

namespace Sirikata {

DistanceDownloadPlanner::DistanceDownloadPlanner(Context* c)
 : ResourceDownloadPlanner(c)
{
}

DistanceDownloadPlanner::~DistanceDownloadPlanner()
{

}

vector<DistanceDownloadPlanner::Resource>::iterator DistanceDownloadPlanner::findResource(ProxyObjectPtr p)
{
    vector<Resource>::iterator it;
    for (it = resources.begin(); it != resources.end(); it++) {
        if (it->proxy == p) return it;
    }
    return resources.end();
}

void DistanceDownloadPlanner::addNewObject(ProxyObjectPtr p, Entity *mesh)
{
    p->MeshProvider::addListener(this);
    Resource r(mesh, p);
    resources.push_back(r);
}

void DistanceDownloadPlanner::removeObject(ProxyObjectPtr p) {
    p->MeshProvider::removeListener(this);
    vector<Resource>::iterator it = findResource(p);
    if (it != resources.end()) resources.erase(it);
}

void DistanceDownloadPlanner::onSetMesh(ProxyObjectPtr proxy, URI const &meshFile,const SpaceObjectReference& sporef)
{
    vector<Resource>::iterator it = findResource(proxy);
    URI last_file = it->file;
    it->file = meshFile;
    it->ready = true;
    proxy->priority = calculatePriority(proxy);
    if (it->file != last_file)
        it->mesh->processMesh(it->file);
}

double DistanceDownloadPlanner::calculatePriority(ProxyObjectPtr proxy)
{
    if (camera == NULL) return 0;

    Vector3d cameraLoc = camera->getPosition();
    Vector3d objLoc = proxy->getPosition();
    Vector3d diff = cameraLoc - objLoc;

    /*double diff2d = sqrt(pow(diff.x, 2) + pow(diff.y, 2));
      double diff3d = sqrt(pow(diff2d, 2) + pow(diff.x, 2));*/

    if (diff.length() <= 0) return 1.0;

    double priority = 1/(diff.length());

    return priority;
}

void DistanceDownloadPlanner::poll()
{
    if (camera == NULL) return;
    vector<Resource>::iterator it;

    for (it = resources.begin(); it != resources.end(); it++) {
        it->proxy->priority = calculatePriority(it->proxy);
    }
}

void DistanceDownloadPlanner::stop()
{

}
}
