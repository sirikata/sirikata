/*  Meru
 *  GraphicsResourceEntity.cpp
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
#include "MeruDefs.hpp"
#include "GraphicsResourceEntity.hpp"
#include "GraphicsResourceName.hpp"
#include "GraphicsResourceManager.hpp"
#include "../OgreSystem.hpp"
#include "../CameraEntity.hpp"
#include "../MeshEntity.hpp"

using std::set;

namespace Meru {

GraphicsResourceEntity::GraphicsResourceEntity(const SpaceObjectReference &id, GraphicsEntity *graphicsEntity)
: GraphicsResource(id.toString(), ENTITY), mGraphicsEntity(graphicsEntity), mLoadTime(Time::now())
{

}

#define ALL_MESHES_ARE_CREATED_EQUAL 0
#define MESH_DISTANCE_IS_KING 0
#define STANDARD_COST_BENEFIT 1

GraphicsResourceEntity::~GraphicsResourceEntity()
{
}

float GraphicsResourceEntity::calcBenefit()
{
  if (!mGraphicsEntity) {
    return 0.0f;
  }
  if (MESH_DISTANCE_IS_KING || STANDARD_COST_BENEFIT) {
    const Location& curLoc = mGraphicsEntity->getProxy().extrapolateLocation(Time::now());

    /*****************
     *  Loop through all OgreSystem's, and loop through the list of attached cameras
     *  (will need to change CameraEntity to keep track of this!)
     *****************/

    std::list<OgreSystem*>::const_iterator systemIter = OgreSystem::sActiveOgreScenes.begin(),
      systemend = OgreSystem::sActiveOgreScenes.end();
    float best = 0.0f;
    for (; systemIter != systemend; ++systemIter) {
      OgreSystem *system = *systemIter;
      std::list<CameraEntity*>::const_iterator cameraIter = system->mAttachedCameras.begin(),
        cameraEnd = system->mAttachedCameras.end();
      for (; cameraIter != cameraEnd; ++cameraIter) {
        CameraEntity *camera = *cameraIter;
        const Location& avatarLoc = camera->getProxy().extrapolateLocation(Time::now());
        float dist = (curLoc.getPosition() - avatarLoc.getPosition()).length();
        float radius = mGraphicsEntity->getBoundingInfo().radius();

        if (dist == 0) {
          return std::numeric_limits<float>::max();
        }
        else if (STANDARD_COST_BENEFIT) {
          if (dist > radius) {
            float scaled = radius / (dist * dist);
          //int64 elapsed = (CURRENT_TIME - mLoadTime).asMilliseconds();
          //return (1.0f + exp(200.0f - elapsed)) *  mGraphicsEntity->getBoundingInfo().radius() / distSq;
            best = best > scaled ? best : scaled;
          } else {
            best = best > radius ? best : radius;
          }
        }
        else if (MESH_DISTANCE_IS_KING) {
          return 1.0f / dist;
        }
      }
    }
    return best;
  }
  else if (ALL_MESHES_ARE_CREATED_EQUAL){
    return 1.0f;
  }
  else {
    return 0.0f;
  }
}

void GraphicsResourceEntity::doParse()
{
  if (mDependencies.size() > 0)
    parsed(true);
}

void GraphicsResourceEntity::doLoad()
{
  assert(mDependencies.size() > 0);

  mLoadTime = CURRENT_TIME;
  if (mGraphicsEntity) {
    mGraphicsEntity->loadMesh(mMeshID.toString());
  }
  mCurMesh = SharedResourcePtr();
  loaded(true, mLoadEpoch);
}

void GraphicsResourceEntity::doUnload()
{
  if (mGraphicsEntity) {
    mGraphicsEntity->unloadMesh();
  }
  mLoadTime = Time::now();
  unloaded(true, mLoadEpoch);
}

void GraphicsResourceEntity::setMeshResource(SharedResourcePtr newMeshPtr)
{
  if(mDependencies.size() != 0) {
    SILOG(resource,warn,"GraphicsResourceEntity::setMeshResource called again before finished loading.");
  }
  if (mDependencies.size() > 0
   && (mLoadState == LOAD_LOADING || mLoadState == LOAD_LOADED)) {
    mCurMesh = *mDependencies.begin();
  }

  clearDependencies();
  addDependency(newMeshPtr);
  if (mCurMesh) { // this is a hack, need to properly split different parse/load
    sCostPropEpoch++;
    mDepCost = mCurMesh->getDepCost(sCostPropEpoch);
  }
}

void GraphicsResourceEntity::fullyParsed()
{
  if (mCurMesh)
    mLoadState = LOAD_SWAPPABLE;
  else
    mLoadState = LOAD_UNLOADED;

  GraphicsResource::fullyParsed();
}

void GraphicsResourceEntity::resolveName(const URI& id, const URI& hash)
{
//FIXME!!!!!!!!!!!
//  assert((*mDependencies.begin())->getID() == getID());
  mMeshID = hash;
}

}
