/*  Sirikata Graphical Object Host
 *  Camera.cpp
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

#include "ProxyCamera.hpp"
#include "ProxyEntity.hpp"
#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/core/options/Options.hpp>

namespace Sirikata {
namespace Graphics {

ProxyCamera::ProxyCamera(OgreRenderer *scene, Ogre::SceneManager* scenemgr, ProxyEntity* follow)
 : Camera(scene, scenemgr, ogreCameraName(follow->id()))
{
    reparent(follow);
}

ProxyCamera::~ProxyCamera() {
    if (mFollowing != NULL)
        ((Provider<ProxyEntityListener*>*)mFollowing)->removeListener(this);
}

void ProxyCamera::proxyEntityDestroyed(ProxyEntity*) {
    mFollowing = NULL;
}

void ProxyCamera::reparent(ProxyEntity* follow) {
    mFollowing = follow;
    ((Provider<ProxyEntityListener*>*)mFollowing)->addListener(this);

    // We don't do anything with these, but they force caching of the current state
    getGoalPosition();
    getGoalOrientation();
    getGoalBounds();

    // Reset the mode to the existing setting
    setMode(mMode);
}

Vector3d ProxyCamera::getGoalPosition() {
    if (mFollowing) return (mLastGoalPosition = mFollowing->getOgrePosition());
    else return mLastGoalPosition;
}

Quaternion ProxyCamera::getGoalOrientation() {
    if (mFollowing) return (mLastGoalOrientation = mFollowing->getOgreOrientation());
    else return mLastGoalOrientation;
}

BoundingSphere3f ProxyCamera::getGoalBounds() {
    if (mFollowing) return (mLastGoalBounds = mFollowing->getProxyPtr()->getBounds());
    else return mLastGoalBounds;
}

void ProxyCamera::setMode(Mode m) {
    Camera::setMode(m);
    if (mFollowing) mFollowing->setVisible( mMode == FirstPerson ? false : true );
}

ProxyEntity* ProxyCamera::following() const {
    return mFollowing;
}

std::string ProxyCamera::ogreCameraName(const String& ref) {
    return "Camera:"+ref;
}

}
}
