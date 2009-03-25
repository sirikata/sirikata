#ifndef _SIRIKATA_PROXY_OBJECT_HPP_
#define _SIRIKATA_PROXY_OBJECT_HPP_
#include <util/UUID.hpp>
#include <util/ListenerProvider.hpp>
#include <util/SpaceObjectReference.hpp>
#include "ProxyObjectListener.hpp"
namespace Sirikata {
/**
 * This class represents a generic object on a remote server
 * Every object has a SpaceObjectReference that allows one to communicate
 * with it. Subclasses implement several Providers for concerned listeners
 * This class should be casted to the various subclasses (ProxyLightObject,etc)
 * and appropriate listeners be registered.
 */
class ProxyObject;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;

typedef Provider<ProxyObjectListener*> ProxyObjectProvider;

class SIRIKATA_OH_EXPORT ProxyObject
  : public ProxyObjectProvider
{
    SpaceObjectReference mID;
public:
    ProxyObject(const SpaceObjectReference&ref):mID(ref){}
    virtual ~ProxyObject(){}
    void destroy() {
        ProxyObjectProvider::notify(&ProxyObjectListener::destroyed);
    }
    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    const SpaceObjectReference&getObjectReference() const{
        return mID;
    }
};
}
#endif
