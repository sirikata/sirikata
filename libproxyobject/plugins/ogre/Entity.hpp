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
#ifndef SIRIKATA_GRAPHICS_ENTITY_HPP__
#define SIRIKATA_GRAPHICS_ENTITY_HPP__

#include <sirikata/core/util/UUID.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyObjectListener.hpp>
#include <OgreMovableObject.h>
#include <OgreRenderable.h>
#include <OgreSceneManager.h>
#include "OgreConversions.hpp"
namespace Sirikata {
namespace Graphics {
class OgreSystem;

/** Base class for any ProxyObject that has a representation in Ogre. */
class Entity
  : public PositionListener,
    public ProxyObjectListener
{
protected:
    OgreSystem *const mScene;
    const ProxyObjectPtr mProxy;

    Ogre::MovableObject *mOgreObject;
    Ogre::SceneNode *mSceneNode;

    std::list<Entity*>::iterator mMovingIter;

    void init(Ogre::MovableObject *obj);

    void setStatic(bool isStatic);

protected:
    void setOgrePosition(const Vector3d &pos);

    void setOgreOrientation(const Quaternion &orient);
public:
    ProxyObject &getProxy() const {
        return *mProxy;
    }
    const ProxyObjectPtr &getProxyPtr() const {
        return mProxy;
    }
    Entity(OgreSystem *scene,
           const ProxyObjectPtr &ppo,
           const std::string &ogreId,
           Ogre::MovableObject *obj=NULL);

    virtual ~Entity();

    static Entity *fromMovableObject(Ogre::MovableObject *obj);

    void removeFromScene();
    void addToScene(Ogre::SceneNode *newParent=NULL);

    OgreSystem *getScene() const{
        return mScene;
    }

    virtual void updateLocation(const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient);

    virtual void destroyed();

    Ogre::SceneNode *getSceneNode() {
        return mSceneNode;
    }
    Vector3d getOgrePosition();

    Quaternion getOgreOrientation();

    void extrapolateLocation(TemporalValue<Location>::Time current);

    virtual void setSelected(bool selected) {
      mSceneNode->showBoundingBox(selected);
    }
    virtual std::string ogreMovableName() const{
        return id().toString();
    }
    const SpaceObjectReference&id()const{
        return mProxy->getObjectReference();
    }
};
typedef std::tr1::shared_ptr<Entity> EntityPtr;

}
}

#endif
