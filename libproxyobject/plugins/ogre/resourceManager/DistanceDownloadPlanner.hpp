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

#ifndef _DISTANCE_DOWNLOAD_PLANNER_HPP
#define _DISTANCE_DOWNLOAD_PLANNER_HPP

#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/mesh/ModelsSystem.hpp>
#include <sirikata/proxyobject/MeshListener.hpp>
#include "ResourceDownloadPlanner.hpp"
#include <vector>

namespace Sirikata {
namespace Graphics{
class Entity;
}

class DistanceDownloadPlanner : public ResourceDownloadPlanner
{
public:
    DistanceDownloadPlanner(Context* c);
    ~DistanceDownloadPlanner();

    virtual void addNewObject(ProxyObjectPtr p, Graphics::Entity *mesh);
    virtual void removeObject(ProxyObjectPtr p);

    //MeshListener interface
    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef);

    //PollingService interface
    virtual void poll();
    virtual void stop();

    struct Resource {
        Resource(Graphics::Entity *m, ProxyObjectPtr p)
         : mesh(m),
           proxy(p),
           loaded(false)
        {}
        virtual ~Resource(){}

        Transfer::URI file;
        Graphics::Entity *mesh;
        ProxyObjectPtr proxy;
        bool loaded;


        class Hasher {
        public:
            size_t operator() (const Resource& r) const {
                return r.proxy->hash();
            }
        };

        struct MaxHeapComparator {
            bool operator()(Resource* lhs, Resource* rhs) {
                return lhs->proxy->priority < rhs->proxy->priority;
            }
        };
        struct MinHeapComparator {
            bool operator()(Resource* lhs, Resource* rhs) {
                return lhs->proxy->priority > rhs->proxy->priority;
            }
        };

    };

protected:
    void addResource(Resource* r);
    Resource* findResource(const SpaceObjectReference& sporef);
    void removeResource(const SpaceObjectReference& sporef);

    virtual double calculatePriority(ProxyObjectPtr proxy);

    void checkShouldLoadNewResource(Resource* r);

    // Checks if changes just due to budgets are possible,
    // e.g. regardless of priorities, we have waiting objects and free
    // spots for them.
    bool budgetRequiresChange() const;

    void loadResource(Resource* r);
    void unloadResource(Resource* r);


    typedef std::tr1::unordered_map<SpaceObjectReference, Resource*, SpaceObjectReference::Hasher> ResourceSet;
    // The full list
    ResourceSet mResources;
    // Loading has started for these
    ResourceSet mLoadedResources;
    // Waiting to be important enough to load
    ResourceSet mWaitingResources;

    // Heap storage for Resources. Choice between min/max heap is at call time.
    typedef std::vector<Resource*> ResourceHeap;


};
}

#endif
