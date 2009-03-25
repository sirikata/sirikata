/*  Sirikata Graphical Object Host
 *  Entity.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#include <oh/Platform.hpp>
#include "Entity.hpp"
#include <options/Options.hpp>

namespace Sirikata {
namespace Graphics {

Entity::Entity(OgreSystem *scene,
               const std::tr1::shared_ptr<ProxyPositionObject> &ppo,
               const UUID &id,
               Ogre::MovableObject *obj)
  : mScene(scene),
    mProxy(ppo),
    mOgreObject(NULL),
    mSceneNode(scene->getSceneManager()->createSceneNode(id.readableHexData())),
    mId(id),
    mSceneIter(scene->mSceneEntities.end()),
    mMovingIter(scene->mMovingEntities.end())
{
    if (obj) {
        init(obj);
    }
    mSceneIter = scene->mSceneEntities.insert(mSceneIter, this);
        
    ppo->ProxyObjectProvider::addListener(this);
    ppo->PositionProvider::addListener(this);
}

Entity::~Entity() {
    mScene->mSceneEntities.erase(mSceneIter);
    if (mMovingIter != mScene->mMovingEntities.end()) {
        mScene->mMovingEntities.erase(mMovingIter);
    }
    getProxy().ProxyObjectProvider::removeListener(this);
    getProxy().PositionProvider::removeListener(this);
    init(NULL);
    mSceneNode->detachAllObjects();
    mScene->getSceneManager()->destroySceneNode(mId.readableHexData());
}

void Entity::init(Ogre::MovableObject *obj) {
    if (mOgreObject) {
        mSceneNode->detachObject(mOgreObject);
        if (!obj) {
            removeFromScene();
        }
    } else if (obj) {
        addToScene(NULL);
    }
    mOgreObject = obj;
    if (obj) {
        mSceneNode->attachObject(obj);
    }
}

void Entity::setStatic(bool isStatic) {
    const std::list<Entity*>::iterator end = mScene->mMovingEntities.end();
    if (isStatic) {
        if (mMovingIter != end) {
            SILOG(ogre,debug,"Removed "<<this<<" from moving entities queue.");
            mScene->mMovingEntities.erase(mMovingIter);
            mMovingIter = end;
        }
    } else {
        if (mMovingIter == end) {
            SILOG(ogre,debug,"Added "<<this<<" to moving entities queue.");
            mMovingIter = mScene->mMovingEntities.insert(end, this);
        }
    }
}

void Entity::removeFromScene() {
    Ogre::SceneNode *oldParent = mSceneNode->getParentSceneNode();
    if (oldParent) {
        oldParent->removeChild(mSceneNode);
    }
    setStatic(true);
}
void Entity::addToScene(Ogre::SceneNode *newParent) {
    if (newParent == NULL) {
        newParent = mScene->getSceneManager()->getRootSceneNode();
    }
    removeFromScene();
    newParent->addChild(mSceneNode);
    setStatic(false); // May get set to true after the next frame has drawn.
}

void Entity::updateLocation(Time ti, const Location &newLocation) {
    SILOG(ogre,debug,"UpdateLocation "<<this<<" to "<<newLocation.getPosition());
    if (!getProxy().isStatic(ti)) {
        setStatic(false);
    }
}

void Entity::resetLocation(Time ti, const Location &newLocation) {
    SILOG(ogre,debug,"ResetLocation "<<this<<" to "<<newLocation.getPosition());
    if (!getProxy().isStatic(ti)) {
        setStatic(false);
    }
}

void Entity::destroyed() {
    delete this;
}

void Entity::extrapolateLocation(TemporalValue<Location>::Time current) {
    setOgrePosition(getProxy().extrapolatePosition(current));
    setOgreOrientation(getProxy().extrapolateOrientation(current));
    setStatic(getProxy().isStatic(current));
}

}
}
