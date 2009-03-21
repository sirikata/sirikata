#ifndef _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#define _SIRIKATA_PROXY_LIGHT_OBJECT_HPP_
#include "LightListener.hpp"
#includE "ProxyObject.hpp"
namespace Sirikata {
/**
 * This class represents a ProxyObject that holds a LightInfo
 */
class SIRIKATA_OH_EXPORT ProxyLightObject : MarkovianProvider1<LightListenerPtr,LightInfo>, ProxyObject{
public:
    ProxyLightObject():MarkovianProvider1<LightListenerPtr,LightInfo>(LightInfo()){}
    
};
}
#endif
