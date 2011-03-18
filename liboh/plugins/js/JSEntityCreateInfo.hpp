#ifndef __SIRIKATA_ENTITY_CREATE_INFO_HPP__
#define __SIRIKATA_ENTITY_CREATE_INFO_HPP__

#include <sirikata/oh/HostedObject.hpp>

namespace Sirikata{
namespace JS{

struct EntityCreateInfo
{
    String scriptType;
    String scriptOpts;
    SpaceID spaceID;
    Location loc;
    float  scale;
    String mesh;
    SolidAngle solid_angle;
};

} //close js
} //close sirikata

#endif
