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
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include "../MeshEntity.hpp"
#include <stdlib.h>
#include <algorithm>

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;
using namespace Sirikata::Graphics;

#define frequency 0.1

namespace Sirikata {

DistanceDownloadPlanner::DistanceDownloadPlanner(Provider<ProxyCreationListener*> *proxyManager, Context *c)
 : ResourceDownloadPlanner(proxyManager, c)
{
}

DistanceDownloadPlanner::~DistanceDownloadPlanner()
{

}

void DistanceDownloadPlanner::onCreateProxy(ProxyObjectPtr p)
{

}

vector<DistanceDownloadPlanner::Resource>::iterator DistanceDownloadPlanner::findResource(ProxyObjectPtr p)
{
    vector<Resource>::iterator it;
    for (it = resources.begin(); it != resources.end(); it++) {
        if (it->proxy == p) return it;
    }
}

void DistanceDownloadPlanner::onDestroyProxy(ProxyObjectPtr p)
{
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    if (meshptr) {
        meshptr->MeshProvider::removeListener(this);
        vector<Resource>::iterator it = findResource(p);
        resources.erase(it);
    }
}

void DistanceDownloadPlanner::addNewObject(ProxyObjectPtr p, MeshEntity *mesh)
{
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    if (meshptr) {
        meshptr->MeshProvider::addListener(this);

        Resource r(mesh, p, false);
        resources.push_back(r);
    }
}

void DistanceDownloadPlanner::onSetMesh(ProxyObjectPtr proxy, URI const &meshFile)
{
   ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(proxy));
   if (meshptr) {
       Resource sample = Resource(NULL, meshptr, false);
       vector<Resource>::iterator it = findResource(proxy);
       it->mesh->processMesh(meshFile);
   }
}

void DistanceDownloadPlanner::poll()
{

}

void DistanceDownloadPlanner::stop()
{

}
}
