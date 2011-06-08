/*  Sirikata
 *  Camera.hpp
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

#ifndef SIRIKATA_OGRE_CAMERA_HPP__
#define SIRIKATA_OGRE_CAMERA_HPP__

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include <OgreMovableObject.h>
#include <OgreRenderable.h>
#include <OgreRenderTarget.h>


namespace Sirikata {
namespace Graphics {

class OgreRenderer;

class SIRIKATA_OGRE_EXPORT Camera {
public:
    enum Mode {
        FirstPerson,
        ThirdPerson
    };

protected:
    OgreRenderer *const mScene;
    Ogre::SceneManager* mOgreSceneManager;
    Ogre::Camera* mOgreCamera;
    Ogre::SceneNode *mSceneNode;

    Ogre::RenderTarget *mRenderTarget;
    Ogre::Viewport *mViewport;

    Mode mMode;
    Vector3d mOffset;

    float32 mViewportLeft, mViewportTop, mViewportRight, mViewportBottom;
public:
    Camera(OgreRenderer *scene, Ogre::SceneManager* scenemgr, const String& name);
    virtual ~Camera();

    // Require an additional initialize call in order to use virtual functions
    // for position, orientation
    void initialize();

    virtual bool haveGoal() { return false; }
    virtual Vector3d getGoalPosition() { return Vector3d(); }
    virtual Quaternion getGoalOrientation() { return Quaternion(); }
    virtual BoundingSphere3f getGoalBounds() { return BoundingSphere3f(); }

    void attach (const String&renderTargetName,
        uint32 width,
        uint32 height,
        Vector4f back_color,
        int zorder);

    void detach();

    void windowResized();

    void tick(const Time& t, const Duration& dt);

    Ogre::Viewport* getViewport() {
        return mViewport;
    }
    Ogre::Camera* getOgreCamera() {
        return mOgreCamera;
    }

    void setViewportDimensions(int32 left, int32 top, int32 right, int32 bottom);
    void setViewportDimensions(float32 left, float32 top, float32 right, float32 bottom);

    virtual void setMode(Mode m);
    Mode getMode() const { return mMode; }

    void setOffset(Vector3d offset) {
        mOffset = offset;
    }

    Vector3d getPosition() const;
    Quaternion getOrientation() const;

};

}
}

#endif // SIRIKATA_OGRE_CAMERA_HPP_
