/*  Sirikata Graphical Object Host
 *  Entity.cpp
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

#include <sirikata/proxyobject/Platform.hpp>
#include "Entity.hpp"
#include <sirikata/core/options/Options.hpp>
#include "OgreSystem.hpp"

namespace Sirikata {
namespace Graphics {

Entity::Entity(OgreSystem *scene,
               const ProxyObjectPtr &ppo,
               const std::string &ogreId,
               Ogre::MovableObject *obj)
  : mScene(scene),
    mProxy(ppo),
    mOgreObject(NULL),
    mSceneNode(scene->getSceneManager()->createSceneNode(ogreId)),
    mMovingIter(scene->mMovingEntities.end())
{
    mSceneNode->setInheritScale(false);
    addToScene(NULL);
    if (obj) {
        init(obj);
    }
    bool successful = scene->mSceneEntities.insert(
        OgreSystem::SceneEntitiesMap::value_type(mProxy->getObjectReference(), this)).second;

    assert (successful == true);

    ppo->ProxyObjectProvider::addListener(this);
    ppo->PositionProvider::addListener(this);
}

Entity::~Entity() {
    OgreSystem::SceneEntitiesMap::iterator iter =
        mScene->mSceneEntities.find(mProxy->getObjectReference());
    if (iter != mScene->mSceneEntities.end()) {
        // will fail while in the OgreSystem destructor.
        mScene->mSceneEntities.erase(iter);
    }
    if (mMovingIter != mScene->mMovingEntities.end()) {
        mScene->mMovingEntities.erase(mMovingIter);
        mMovingIter = mScene->mMovingEntities.end();
    }
    getProxy().ProxyObjectProvider::removeListener(this);
    getProxy().PositionProvider::removeListener(this);
    removeFromScene();
    init(NULL);
    mSceneNode->detachAllObjects();
    /* detaches all children from the scene.
       There should be none, as the server should have adjusted their parents.
     */
    mSceneNode->removeAllChildren();
    mScene->getSceneManager()->destroySceneNode(mSceneNode);
}

Entity *Entity::fromMovableObject(Ogre::MovableObject *movable) {
    return Ogre::any_cast<Entity*>(movable->getUserAny());
}

void Entity::init(Ogre::MovableObject *obj) {
    if (mOgreObject) {
        mSceneNode->detachObject(mOgreObject);
    }
    mOgreObject = obj;
    if (obj) {
        mOgreObject->setUserAny(Ogre::Any(this));
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

void Entity::setOgrePosition(const Vector3d &pos) {
    SILOG(ogre,debug,"setOgrePosition "<<this<<" to "<<pos);
    Ogre::Vector3 ogrepos = toOgre(pos, getScene()->getOffset());
    const Ogre::Vector3 &scale = mSceneNode->getScale();
    mSceneNode->setPosition(ogrepos);
}
void Entity::setOgreOrientation(const Quaternion &orient) {
    SILOG(ogre,debug,"setOgreOrientation "<<this<<" to "<<orient);
    mSceneNode->setOrientation(toOgre(orient));
}


void Entity::updateLocation(const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient) {
    SILOG(ogre,debug,"UpdateLocation "<<this<<" to "<<newLocation.position()<<"; "<<newOrient.position());
    if (!getProxy().isStatic()) {
        setStatic(false);
    } else {
        setOgrePosition(Vector3d(newLocation.position()));
        setOgreOrientation(newOrient.position());
    }
}

void Entity::destroyed(const Time&) {
    delete this;
}
void Entity::extrapolateLocation(TemporalValue<Location>::Time current) {
    Location loc (getProxy().extrapolateLocation(current));
    setOgrePosition(loc.getPosition());
    setOgreOrientation(loc.getOrientation());
    setStatic(getProxy().isStatic());
}

Vector3d Entity::getOgrePosition() {
    if (mScene == NULL) assert(false);
    return fromOgre(mSceneNode->getPosition(), mScene->getOffset());
}

Quaternion Entity::getOgreOrientation() {
    return fromOgre(mSceneNode->getOrientation());
}

}
}
