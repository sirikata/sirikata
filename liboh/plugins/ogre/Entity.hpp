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

#include "OgreSystem.hpp"
#undef nil
#include <util/UUID.hpp>
#include <oh/ProxyPositionObject.hpp>
#include <oh/ProxyObjectListener.hpp>
#include <OgreMovableObject.h>
#include <OgreRenderable.h>
#include <OgreSceneManager.h>

namespace Sirikata {
typedef Vector4f ColorAlpha;
typedef Vector3f Color;
namespace Graphics {
inline Ogre::Quaternion toOgre(const Sirikata::Quaternion &quat) {
    return quat.convert<Ogre::Quaternion>();
}

inline Ogre::Vector3 toOgre(const Sirikata::Vector3f &pos) {
    return pos.convert<Ogre::Vector3>();
}

// Ogre uses floating points internally. Base should be equal to the translation of the scene.
inline Ogre::Vector3 toOgre(const Sirikata::Vector3d &pos, const Sirikata::Vector3d &base) {
    return (pos - base).convert<Ogre::Vector3>();
}

inline Ogre::Vector4 toOgre(const Sirikata::Vector4f &pos) {
    return pos.convert<Ogre::Vector4>();
}

inline Ogre::ColourValue toOgreRGBA(const Sirikata::ColorAlpha &rgba) {
    return rgba.convert<Ogre::ColourValue>();
}

inline Ogre::ColourValue toOgreRGB(const Sirikata::Color &rgb) {
    return rgb.convert<Ogre::ColourValue>();
}

inline Ogre::ColourValue toOgreRGBA(const Sirikata::Color &rgb, float32 alpha) {
    return rgb.convert<Ogre::ColourValue>();
}

inline Sirikata::Quaternion fromOgre(const Ogre::Quaternion &quat) {
    return Sirikata::Quaternion(quat.x,quat.y,quat.z,quat.w,Quaternion::XYZW());
}

inline Sirikata::Vector3f fromOgre(const Ogre::Vector3 &pos) {
    return Sirikata::Vector3f(pos);
}

inline Sirikata::Vector3d fromOgre(const Ogre::Vector3 &pos, const Vector3d &base) {
    return Sirikata::Vector3d(pos) + base;
}

inline Sirikata::Vector4f fromOgre(const Ogre::Vector4 &pos) {
    return Sirikata::Vector4f(pos.x,pos.y,pos.z,pos.w);
}

inline Sirikata::ColorAlpha fromOgreRGBA(const Ogre::ColourValue &rgba) {
    return Sirikata::ColorAlpha(rgba.r,rgba.b,rgba.g,rgba.a);
}

inline Sirikata::Color fromOgreRGB(const Ogre::ColourValue &rgba) {
    return Sirikata::Color(rgba.r,rgba.g,rgba.b);
}
class OgreSystem;

class Entity 
  : public PositionListener,
    public ProxyObjectListener
{
protected:
    OgreSystem *const mScene;
    const std::tr1::shared_ptr<ProxyPositionObject> mProxy;

    Ogre::MovableObject *mOgreObject;
    Ogre::SceneNode *mSceneNode;

    const UUID mId;
    std::list<Entity*>::iterator mSceneIter, mMovingIter;

    void init(Ogre::MovableObject *obj) {
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

    void setOgrePosition(const Vector3d &pos) {
        mSceneNode->setPosition(toOgre(pos, getScene()->getOffset()));
    }
    void setOgreOrientation(const Quaternion &orient) {
        mSceneNode->setOrientation(toOgre(orient));
    }
    void setStatic(bool isStatic) {
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
public:
    ProxyPositionObject &getProxy() const {
        return *mProxy;
    }
    Entity(OgreSystem *scene,
           const std::tr1::shared_ptr<ProxyPositionObject> &ppo,
           const UUID &id,
           Ogre::MovableObject *obj=NULL)
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

    virtual ~Entity() {
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

    void removeFromScene() {
        Ogre::SceneNode *oldParent = mSceneNode->getParentSceneNode();
        if (oldParent) {
            oldParent->removeChild(mSceneNode);
        }
        setStatic(true);
    }
    void addToScene(Ogre::SceneNode *newParent=NULL) {
        if (newParent == NULL) {
            newParent = mScene->getSceneManager()->getRootSceneNode();
        }
        removeFromScene();
        newParent->addChild(mSceneNode);
        setStatic(false); // May get set to true after the next frame has drawn.
    }

    OgreSystem *getScene() {
        return mScene;
    }

    void updateLocation(Time ti, const Location &newLocation) {
        SILOG(ogre,debug,"UpdateLocation "<<this<<" to "<<newLocation.getPosition());
        if (!getProxy().isStatic(ti)) {
            setStatic(false);
        }
    }

    void resetLocation(Time ti, const Location &newLocation) {
        SILOG(ogre,debug,"ResetLocation "<<this<<" to "<<newLocation.getPosition());
        if (!getProxy().isStatic(ti)) {
            setStatic(false);
        }
    }

    void destroyed() {
        delete this;
    }

    Vector3d getOgrePosition() {
        return fromOgre(mSceneNode->getPosition(), getScene()->getOffset());
    }
    Quaternion getOgreOrientation() {
        return fromOgre(mSceneNode->getOrientation());
    }

    void extrapolateLocation(TemporalValue<Location>::Time current) {
        setOgrePosition(getProxy().extrapolatePosition(current));
        setOgreOrientation(getProxy().extrapolateOrientation(current));
        setStatic(getProxy().isStatic(current));
    }

    virtual bool setSelected(bool selected) {
      return false;
    }
    const UUID&id()const{
        return mId;
    }
};
typedef std::tr1::shared_ptr<Entity> EntityPtr;

}
}

#endif
