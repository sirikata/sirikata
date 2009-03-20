#ifndef _SIRIKATA_PROXY_OBJECT_HPP_
#define _SIRIKATA_PROXY_OBJECT_HPP_
#include <util/UUID.hpp>
#include "ProxyObjectListener.hpp"
namespace Sirikata {

typedef UUID SpaceObjectReference;//FIXME: this is too short
class SIRIKATA_OH_EXPORT ProxyObject{
    SpaceObjectReference mID;
public:
    ProxyObject(){}
    virtual ~ProxyObject(){}
    const SpaceObjectReference&getObjectReference() const{
        return mID;
    }
};
}
#endif
