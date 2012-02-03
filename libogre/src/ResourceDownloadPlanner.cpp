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

#include <sirikata/ogre/ResourceDownloadPlanner.hpp>
#include <stdlib.h>
#include <algorithm>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/MeshListener.hpp>

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;
using namespace Sirikata::Graphics;

#define frequency 1.0

namespace Sirikata {
namespace Graphics {

ResourceDownloadPlanner::ResourceDownloadPlanner(Context *c, OgreRenderer* renderer)
 : PollingService(c->mainStrand, "Resource Download Planner Poll", Duration::seconds(frequency), c, "Resource Download Planner Poll"),
   mContext(c),
   mScene(renderer),
   mMaxLoaded(2500)
{
    c->add(this);
    camera = NULL;
}

ResourceDownloadPlanner::~ResourceDownloadPlanner()
{

}

void ResourceDownloadPlanner::addNewObject(Graphics::Entity *ent, const Transfer::URI& mesh) {
  RMutex::scoped_lock lock(mDlPlannerMutex);
}

void ResourceDownloadPlanner::addNewObject(ProxyObjectPtr p, Entity *mesh)
{
  RMutex::scoped_lock lock(mDlPlannerMutex);
}

void ResourceDownloadPlanner::updateObject(ProxyObjectPtr p)
{
  RMutex::scoped_lock lock(mDlPlannerMutex);
}

void ResourceDownloadPlanner::removeObject(ProxyObjectPtr p) {
  RMutex::scoped_lock lock(mDlPlannerMutex);
}

void ResourceDownloadPlanner::removeObject(Graphics::Entity* mesh) {
  RMutex::scoped_lock lock(mDlPlannerMutex);
}


void ResourceDownloadPlanner::setCamera(Camera *entity)
{
    RMutex::scoped_lock lock(mDlPlannerMutex);
    camera = entity;
}

void ResourceDownloadPlanner::poll()
{
  RMutex::scoped_lock lock(mDlPlannerMutex);
}

void ResourceDownloadPlanner::stop()
{
  RMutex::scoped_lock lock(mDlPlannerMutex);
}

void ResourceDownloadPlanner::setMaxObjects(int32 new_max) {
    RMutex::scoped_lock lock(mDlPlannerMutex);
    mMaxLoaded = new_max;
}

} // namespace Graphics
} // namespace Sirikata
