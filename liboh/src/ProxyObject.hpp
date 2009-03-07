#ifndef _SIRIKATA_PROXY_OBJECT_HPP_
#define _SIRIKATA_PROXY_OBJECT_HPP_
#include "LightListener.hpp"
namespace Sirikata {
class ProxyObject : MarkovianProvider1<LightListenerPtr,LightInfo>{
public:
    ProxyObject():MarkovianProvider1<LightListenerPtr,LightInfo>(LightInfo()){}
    
};
}
#endif
