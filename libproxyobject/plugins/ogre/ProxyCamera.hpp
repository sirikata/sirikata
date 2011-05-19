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

#ifndef SIRIKATA_OGRE_PROXY_CAMERA_HPP__
#define SIRIKATA_OGRE_PROXY_CAMERA_HPP__

#include <sirikata/ogre/Camera.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>

namespace Sirikata {
namespace Graphics {

class ProxyEntity;

// This class just overrides some virtual methods to make the camera follow a ProxyObject.
class ProxyCamera : public Camera{
private:
    ProxyEntity* mFollowing;
public:
    ProxyCamera(OgreRenderer *scene, ProxyEntity* follow);
    ~ProxyCamera();

    ProxyEntity* following() const;

    virtual Vector3d getGoalPosition();
    virtual Quaternion getGoalOrientation();
    virtual BoundingSphere3f getGoalBounds();

    virtual void setMode(Mode m);

private:
    static String ogreCameraName(const String& ref);
};

}
}

#endif // SIRIKATA_OGRE_PROXY_CAMERA_HPP__
