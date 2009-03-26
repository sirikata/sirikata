#ifndef _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#define _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#include "LightListener.hpp"
#include "ProxyPositionObject.hpp"
namespace Sirikata {

typedef MarkovianProvider1<LightListener*,LightInfo> LightProvider;

/**
 * This class represents a ProxyObject that holds a LightInfo
 */
class SIRIKATA_OH_EXPORT ProxyLightObject
  : public LightProvider,
    public ProxyPositionObject {
public:
    ProxyLightObject(ProxyManager *man, const SpaceObjectReference&id);
    void update(const LightInfo &li);
};
}
#endif
