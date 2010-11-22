/*  Sirikata Graphical Object Host
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
#ifndef SIRIKATA_GRAPHICS_CAMERA_HPP__
#define SIRIKATA_GRAPHICS_CAMERA_HPP__

#include "Entity.hpp"
#include "OgreHeaders.hpp"
#include <OgreMovableObject.h>
#include <OgreRenderable.h>
#include <OgreRenderTarget.h>


namespace Sirikata {
namespace Graphics {

class Camera {
    OgreSystem *const mScene;
    Ogre::Camera* mOgreCamera;
    Ogre::SceneNode *mSceneNode;

    Ogre::RenderTarget *mRenderTarget;
    Ogre::Viewport *mViewport;

    Entity* mFollowing;
public:
    Camera(OgreSystem *scene, Entity* follow);
    ~Camera();

    Entity* following() const;

    void attach (const String&renderTargetName,
                         uint32 width,
                         uint32 height);
    void detach();

    void tick();

    Ogre::Viewport* getViewport() {
        return mViewport;
    }
    Ogre::Camera* getOgreCamera() {
        return mOgreCamera;
    }

private:

    static String ogreCameraName(const SpaceObjectReference&ref);

};

}
}

#endif
