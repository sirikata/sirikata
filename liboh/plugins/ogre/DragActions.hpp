/*  Sirikata liboh -- Ogre Graphics Plugin
 *  DragActions.hpp
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

#ifndef _SIRIKATA_DRAG_ACTIONS_
#define _SIRIKATA_DRAG_ACTIONS_
#include <util/Platform.hpp>
#include <util/Time.hpp>
#include <util/ListenerProvider.hpp>
#include <oh/TimeSteppedSimulation.hpp>
#include <oh/ProxyObject.hpp>
#include "input/InputEvents.hpp"

namespace Sirikata {
namespace Input {
class SDLInputManager;
class MouseDragEvent;
typedef std::tr1::shared_ptr<MouseDragEvent> MouseDragEventPtr;
}
namespace Graphics {
class Entity;
using Input::SDLInputManager;
class CameraEntity;
class OgreSystem;

struct DragStartInfo {
    OgreSystem *sys;
    CameraEntity *camera;
    typedef std::set<ProxyPositionObjectPtr> EntitySet;
    const EntitySet &objects;
    const Input::MouseDragEventPtr &ev;
};

class ActiveDrag {
public:
    virtual void mouseMoved(Input::MouseDragEventPtr ev) = 0;
    virtual ~ActiveDrag() {}

    template <class ActiveDragSubclass>
    static ActiveDrag *factory(const DragStartInfo &info) {
        return new ActiveDragSubclass(info);
    }

};

/** Factory for DragData, to be called when a drag action is started. */
typedef std::tr1::function<ActiveDrag * (const DragStartInfo &info)> DragAction;

/** Simple global registry to look up drag handlers. */
class DragActionRegistry {
    static DragActionRegistry &getSingleton();

    std::tr1::unordered_map<std::string, DragAction> mRegistry;
public:
    static void set(const std::string &name, const DragAction &object);
    static void unset(const std::string &name);
    class Register {
    public:
        Register(const std::string &name, const DragAction &object) {
            set(name, object);
        }
    };
    template <class T>
    class RegisterClass {
    public:
        RegisterClass(const std::string &name) {
            set(name, &ActiveDrag::factory<T>);
        }
    };
    static const DragAction &get(const std::string &name);
};

class CameraEntity;
class Entity;
class OgreSystem;
Vector3f pixelToDirection(CameraEntity *cam, Quaternion orient, float xPixel, float yPixel);
void zoomInOut(Input::AxisValue value, const Input::InputDevicePtr &dev, CameraEntity *camera, std::set<ProxyPositionObjectPtr> objects, OgreSystem *parent);
void pixelToRadians(CameraEntity *cam, float deltaXPct, float deltaYPct, float &xRadians, float &yRadians);

template <class Iterator>
inline Vector3d averageSelectedPosition(Task::AbsTime now, Iterator iter, Iterator end) {
    Vector3d totalPosition(0,0,0);
    if (iter == end) {
        return totalPosition;
    }
    size_t count = 0;
    for (;iter != end; ++iter) {
        const ProxyPositionObjectPtr &ent = *iter;
        if (!ent) continue;
        totalPosition += (ent->globalLocation(now).getPosition());
        ++count;
    }
    return totalPosition / count;
}

inline Vector3f direction(Quaternion cameraAngle) {
    return -cameraAngle.zAxis();
}



} }
#endif
