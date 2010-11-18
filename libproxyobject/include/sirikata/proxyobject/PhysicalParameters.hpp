#ifndef _SIRIKATA_PHYSICAL_PARAMETERS_HPP_
#define _SIRIKATA_PHYSICAL_PARAMETERS_HPP_

#include <sirikata/core/transfer/URI.hpp>

namespace Sirikata {


class PhysicalParameters {
public:
    enum PhysicalMode {
        Disabled = 0,               /// non-active, remove from physics
        Static,                 /// collisions, no dynamic movement (bullet mass==0)
        DynamicBox,                 /// fully physical -- collision & dynamics
        DynamicSphere,
        DynamicCylinder,
        Character
    };

    std::string name;
    PhysicalMode mode;
    float density;
    float friction;
    float bounce;
    float gravity;
    int colMask;
    int colMsg;
    Vector3f hull;
    PhysicalParameters() :
        mode(Disabled),
        density(0),
        friction(0),
        bounce(0),
        gravity(0),
        colMask(0),
        colMsg(0),
        hull() {
    }
};


}

#endif
