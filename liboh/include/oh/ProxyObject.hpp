#ifndef _SIRIKATA_PROXY_OBJECT_HPP_
#define _SIRIKATA_PROXY_OBJECT_HPP_
#include <util/UUID.hpp>
#include <util/SpaceObjectReference.hpp>
#include "ProxyObjectListener.hpp"
namespace Sirikata {

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
