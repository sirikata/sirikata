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

void DistanceDownloadPlanner::addResource(Resource* r) {
    mResources[r->proxy->getObjectReference()] = r;
    mWaitingResources[r->proxy->getObjectReference()] = r;
    checkShouldLoadNewResource(r);
}

DistanceDownloadPlanner::Resource* DistanceDownloadPlanner::findResource(const SpaceObjectReference& sporef) {
    ResourceSet::iterator it = mResources.find(sporef);
    return (it != mResources.end() ? it->second : NULL);
}

void DistanceDownloadPlanner::removeResource(const SpaceObjectReference& sporef) {
    ResourceSet::iterator it = mResources.find(sporef);
    if (it != mResources.end()) {
        ResourceSet::iterator loaded_it = mLoadedResources.find(sporef);
        if (loaded_it != mLoadedResources.end()) mLoadedResources.erase(loaded_it);

        ResourceSet::iterator waiting_it = mWaitingResources.find(sporef);
        if (waiting_it != mWaitingResources.end()) mWaitingResources.erase(waiting_it);

        delete it->second;
        mResources.erase(it);
    }
}


void DistanceDownloadPlanner::addNewObject(ProxyObjectPtr p, Entity *mesh) {
    p->MeshProvider::addListener(this);
    addResource(new Resource(mesh, p));
}

void DistanceDownloadPlanner::removeObject(ProxyObjectPtr p) {
    p->MeshProvider::removeListener(this);
    removeResource(p->getObjectReference());
}

void DistanceDownloadPlanner::onSetMesh(ProxyObjectPtr proxy, URI const &meshFile,const SpaceObjectReference& sporef)
{
    Resource* r = findResource(proxy->getObjectReference());
    URI last_file = r->file;
    r->file = meshFile;
    proxy->priority = calculatePriority(proxy);
    if (r->file != last_file && r->loaded)
        r->mesh->display(r->file);
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

void DistanceDownloadPlanner::checkShouldLoadNewResource(Resource* r) {
    if ((int32)mLoadedResources.size() < mMaxLoaded)
        loadResource(r);
}

bool DistanceDownloadPlanner::budgetRequiresChange() const {
    return
        ((int32)mLoadedResources.size() < mMaxLoaded && !mWaitingResources.empty()) ||
        ((int32)mLoadedResources.size() > mMaxLoaded && !mLoadedResources.empty());

}

void DistanceDownloadPlanner::loadResource(Resource* r) {
    mWaitingResources.erase(r->proxy->getObjectReference());
    mLoadedResources[r->proxy->getObjectReference()] = r;

    r->loaded = true;
    r->mesh->display(r->proxy->getMesh());
}

void DistanceDownloadPlanner::unloadResource(Resource* r) {
    mLoadedResources.erase(r->proxy->getObjectReference());
    mWaitingResources[r->proxy->getObjectReference()] = r;

    r->loaded = false;
    r->mesh->undisplay();
}

void DistanceDownloadPlanner::poll()
{
    if (camera == NULL) return;

    // Update priorities, tracking the largest undisplayed priority and the
    // smallest displayed priority to decide if we're going to have to swap.
    float32 mMinLoadedPriority = 1000000, mMaxWaitingPriority = 0;
    for (ResourceSet::iterator it = mResources.begin(); it != mResources.end(); it++) {
        Resource* r = it->second;
        r->proxy->priority = calculatePriority(r->proxy);

        if (r->loaded)
            mMinLoadedPriority = std::min(mMinLoadedPriority, (float32)r->proxy->priority);
        else
            mMaxWaitingPriority = std::max(mMaxWaitingPriority, (float32)r->proxy->priority);
    }

    // If the min and max computed above are on the wrong sides, then, we need
    // to do more work to figure out which things to swap between loaded &
    // waiting.
    //
    // Or, if we've just gone under budget (e.g. the max number
    // allowed increased, objects left the scene, etc) then we also
    // run this, which will safely add if we're under budget.
    if (mMinLoadedPriority < mMaxWaitingPriority || budgetRequiresChange()) {
        std::vector<Resource*> loaded_resource_heap;
        std::vector<Resource*> waiting_resource_heap;

        for (ResourceSet::iterator it = mResources.begin(); it != mResources.end(); it++) {
            Resource* r = it->second;
            if (r->loaded)
                loaded_resource_heap.push_back(r);
            else
                waiting_resource_heap.push_back(r);
        }
        std::make_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Resource::MinHeapComparator());
        std::make_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Resource::MaxHeapComparator());

        while(true) {
            if ((int32)mLoadedResources.size() < mMaxLoaded && !waiting_resource_heap.empty()) {
                // If we're under budget, just add to top waiting items
                Resource* max_waiting = waiting_resource_heap.front();
                std::pop_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Resource::MaxHeapComparator());
                waiting_resource_heap.pop_back();

                loadResource(max_waiting);
            }
            else if ((int32)mLoadedResources.size() > mMaxLoaded && !loaded_resource_heap.empty()) {
                // Otherwise, extract the min and check if we can continue
                Resource* min_loaded = loaded_resource_heap.front();
                std::pop_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Resource::MinHeapComparator());
                loaded_resource_heap.pop_back();

                unloadResource(min_loaded);
            }
            else if (!waiting_resource_heap.empty() && !loaded_resource_heap.empty()) {
                // They're equal, we're (potentially) exchanging
                Resource* max_waiting = waiting_resource_heap.front();
                std::pop_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Resource::MaxHeapComparator());
                waiting_resource_heap.pop_back();

                Resource* min_loaded = loaded_resource_heap.front();
                std::pop_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Resource::MinHeapComparator());
                loaded_resource_heap.pop_back();

                if (min_loaded->proxy->priority < max_waiting->proxy->priority) {
                    unloadResource(min_loaded);
                    loadResource(max_waiting);
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }
    }
}

void DistanceDownloadPlanner::stop()
{

}
}
