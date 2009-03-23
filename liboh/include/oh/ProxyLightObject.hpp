#ifndef _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#define _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#include "LightListener.hpp"
#include "ProxyPositionObject.hpp"
namespace Sirikata {
/**
 * This class represents a ProxyObject that holds a LightInfo
 */
class SIRIKATA_OH_EXPORT ProxyLightObject
  : public MarkovianProvider1<LightListenerPtr,LightInfo>,
    public ProxyPositionObject {
public:
    ProxyLightObject():MarkovianProvider1<LightListenerPtr,LightInfo>(LightInfo()){}
    void update(const LightInfo &li) {
        notify(li);
    }
};
}
#endif
