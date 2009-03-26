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
class ProxyManager;

class SIRIKATA_OH_EXPORT ProxyObject
  : public ProxyObjectProvider
{
    const SpaceObjectReference mID;
    ProxyManager *const mManager;
public:
    ProxyObject(ProxyManager *manager, const SpaceObjectReference&ref)
      : mID(ref),
        mManager(manager) {
    }
    virtual ~ProxyObject(){}
    void destroy() {
        ProxyObjectProvider::notify(&ProxyObjectListener::destroyed);
    }
    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    const SpaceObjectReference&getObjectReference() const{
        return mID;
    }
    ProxyManager *getProxyManager() const {
        return mManager;
    }
};
}
#endif
