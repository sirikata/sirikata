#include <oh/Platform.hpp>
#include <oh/ProxyObject.hpp>
#include <util/Extrapolation.hpp>
#include <oh/PositionListener.hpp>
#include <oh/ProxyManager.hpp>

namespace Sirikata {

ProxyObject::ProxyObject(ProxyManager *man, const SpaceObjectReference&id)
      : mID(id),
        mManager(man),
        mLocation(Duration::seconds(.1),
                  TemporalValue<Location>::Time::null(),
                  Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                           Vector3f(0,0,0),Vector3f(0,1,0),0),
                  UpdateNeeded()),
        mParentId(SpaceObjectReference::null()) {}

ProxyObject::~ProxyObject(){}

void ProxyObject::destroy() {
    ProxyObjectProvider::notify(&ProxyObjectListener::destroyed);
    //FIXME mManager->notify(&ProxyCreationListener::destroyProxy);
}

bool ProxyObject::UpdateNeeded::operator() (
    const Location&updatedValue,
    const Location&predictedValue) const
{
    Vector3f ux,uy,uz,px,py,pz;
    updatedValue.getOrientation().toAxes(ux,uy,uz);
    predictedValue.getOrientation().toAxes(px,py,pz);
    return (updatedValue.getPosition()-predictedValue.getPosition()).lengthSquared()>1.0 ||
        ux.dot(px)<.9||uy.dot(py)<.9||uz.dot(pz)<.9;
}

class IsLocationStatic{
public:
    bool operator() (const Location&l) const {
        return l.getVelocity()==Vector3f(0.0,0.0,0.0)
            &&(l.getAxisOfRotation()==Vector3f(0.0,0.0,0.0)
               || l.getAngularSpeed()==0.0);
    }
};
bool ProxyObject::isStatic(const TemporalValue<Location>::Time& when) const {
    return mLocation.templatedPropertyHolds(when,IsLocationStatic());
}

// protected:
// Notification that the Parent has been destroyed
void ProxyObject::destroyed() {
    unsetParent(TemporalValue<Location>::Time::now());
}



void ProxyObject::setLocation(TemporalValue<Location>::Time timeStamp,
                             const Location&location) {
    mLocation.updateValue(timeStamp,
                          location);
    PositionProvider::notify(&PositionListener::updateLocation, timeStamp, location);
}
void ProxyObject::resetLocation(TemporalValue<Location>::Time timeStamp,
                             const Location&location) {
    mLocation.resetValue(timeStamp,
                         location);
    PositionProvider::notify(&PositionListener::resetLocation, timeStamp, location);
}
void ProxyObject::setParent(const ProxyObjectPtr &parent,
               TemporalValue<Location>::Time timeStamp) {
    if (!parent) {
        unsetParent(timeStamp);
        return;
    }
    Location globalParent (parent->globalLocation(timeStamp));
    Location globalLoc (globalLocation(timeStamp));

//    std::cout << "Extrapolated = "<<extrapolateLocation(timeStamp)<<std::endl;

    Location localLoc (globalLoc.toLocal(globalParent));
/*
    std::cout<<" Setting parent "<<std::endl<<globalParent<<std::endl<<
        "global loc = "<<std::endl<<globalLoc<<
        std::endl<<"local loc = "<<std::endl<<localLoc<<std::endl;
*/
    setParent(parent, timeStamp, globalLoc, localLoc);
}

void ProxyObject::setParent(const ProxyObjectPtr &parent,
               TemporalValue<Location>::Time timeStamp,
               const Location &absLocation,
               const Location &relLocation) {
    if (!parent) {
        unsetParent(timeStamp, absLocation);
        return;
    }
    ProxyObjectPtr oldParent (getParentProxy());
    if (oldParent) {
        oldParent->ProxyObjectProvider::removeListener(this);
    }
    parent->ProxyObjectProvider::addListener(this);

    // Using now() should best allow a linear extrapolation to work.
    Location lastPosition(globalLocation(timeStamp));

    mParentId = parent->getObjectReference();
    Location newparentLastGlobal(parent->globalLocation(timeStamp));
/*
    std::cout<<" Last parent global "<<std::endl<<newparentLastGlobal<<std::endl<<
        "global loc = "<<std::endl<<lastPosition<<
        std::endl<<"local loc = "<<std::endl<<lastPosition.toLocal(newparentLastGlobal)<<std::endl;
*/
    mLocation.resetValue(timeStamp, lastPosition.toLocal(newparentLastGlobal));
    mLocation.updateValue(timeStamp, relLocation);

    PositionProvider::notify(&PositionListener::setParent,
                             parent,
                             timeStamp,
                             absLocation,
                             relLocation);
}

void ProxyObject::unsetParent(TemporalValue<Location>::Time timeStamp) {
    unsetParent(timeStamp, globalLocation(timeStamp));
}

void ProxyObject::unsetParent(TemporalValue<Location>::Time timeStamp,
               const Location &absLocation) {

    ProxyObjectPtr oldParent (getParentProxy());
    if (oldParent) {
        oldParent->ProxyObjectProvider::removeListener(this);
    }

    Location lastPosition(globalLocation(timeStamp));
    mParentId = SpaceObjectReference::null();

    mLocation.resetValue(timeStamp, lastPosition);
    mLocation.updateValue(timeStamp, absLocation);

    PositionProvider::notify(&PositionListener::unsetParent,
                             timeStamp,
                             absLocation);
}

ProxyObjectPtr ProxyObject::getParentProxy() const {
    if (getParent() == SpaceObjectReference::null()) {
        return ProxyObjectPtr();
    }
    ProxyObjectPtr parentProxy(getProxyManager()->getProxyObject(getParent()));
    return parentProxy;
}



}
