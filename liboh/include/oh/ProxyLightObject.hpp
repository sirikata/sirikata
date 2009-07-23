#ifndef _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#define _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#include "LightListener.hpp"
#include "ProxyObject.hpp"
namespace Sirikata {

typedef MarkovianProvider1<LightListener*,LightInfo> LightProvider;

/**
 * This class represents a ProxyObject that holds a LightInfo
 */
class SIRIKATA_OH_EXPORT ProxyLightObject
  : public LightProvider,
    public ProxyObject {
    LightInfo mLastInfo;
public:
    ProxyLightObject(ProxyManager *man, const SpaceObjectReference&id);
    void update(const LightInfo &li);
    const LightInfo &getLastLightInfo() const {
        return mLastInfo;
    }
};
}
#endif
