/*  Sirikata Object Host
 *  ProxyObject.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <oh/Platform.hpp>
#include <oh/ProxyObject.hpp>
#include <util/Extrapolation.hpp>
#include <oh/PositionListener.hpp>
#include <oh/ProxyManager.hpp>
#include "ObjectHost_Sirikata.pbj.hpp"

#include "util/RoutableMessageBody.hpp"
#include "util/RoutableMessageHeader.hpp"
#include "util/KnownServices.hpp"
#include "oh/SpaceTimeOffsetManager.hpp"
namespace Sirikata {

ProxyObject::ProxyObject(ProxyManager *man, const SpaceObjectReference&id)
        : mID(id),
        mManager(man),
        mLocation(Duration::seconds(.1),
                  TemporalValue<Location>::Time::null(),
                  Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                           Vector3f(0,0,0),Vector3f(0,1,0),0),
                  UpdateNeeded()),
        mParentId(SpaceObjectReference::null()),
        mLocationAuthority(0) {
    mLocal = true;
}

ProxyObject::~ProxyObject() {}

void ProxyObject::setLocal(bool loc) {
    mLocal = loc;
}

void ProxyObject::destroy() {
    ProxyObjectProvider::notify(&ProxyObjectListener::destroyed);
    //FIXME mManager->notify(&ProxyCreationListener::onDestroyProxy);
}

bool ProxyObject::UpdateNeeded::operator() (
    const Location&updatedValue,
    const Location&predictedValue) const {
    Vector3f ux,uy,uz,px,py,pz;
    updatedValue.getOrientation().toAxes(ux,uy,uz);
    predictedValue.getOrientation().toAxes(px,py,pz);
    return (updatedValue.getPosition()-predictedValue.getPosition()).lengthSquared()>1.0 ||
           ux.dot(px)<.9||uy.dot(py)<.9||uz.dot(pz)<.9;
}

class IsLocationStatic {
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
    unsetParent(SpaceTimeOffsetManager::getSingleton().now(mID.space()));
}

QueryTracker *ProxyObject::getQueryTracker() const {
    return mManager->getQueryTracker(getObjectReference());
}

void ProxyObject::addressMessage(RoutableMessageHeader &hdr) const {
    hdr.set_destination(getObjectReference());
}
bool ProxyObject::sendMessage(MemoryReference message) const {
    QueryTracker *qt = getQueryTracker();
    if (qt) {
        RoutableMessageHeader hdr;
        addressMessage(hdr);
        qt->sendMessage(hdr, message);
        return true;
    }
    else {
        return false;
    }
}

void ProxyObject::setLocation(TemporalValue<Location>::Time timeStamp,
                              const Location&location) {
    mLocation.updateValue(timeStamp,
                          location);
    PositionProvider::notify(&PositionListener::updateLocation, timeStamp, location);
}

void ProxyObject::updateLocationWithObjLoc(
        Location & loc,
        const Protocol::ObjLoc& reqLoc)
{
    if (reqLoc.has_position()) {
        loc.setPosition(reqLoc.position());
    }
    if (reqLoc.has_orientation()) {
        loc.setOrientation(reqLoc.orientation());
    }
    if (reqLoc.has_velocity()) {
        loc.setVelocity(reqLoc.velocity());
    }
    if (reqLoc.has_rotational_axis()) {
        loc.setAxisOfRotation(reqLoc.rotational_axis());
    }
    if (reqLoc.has_angular_speed()) {
        loc.setAngularSpeed(reqLoc.angular_speed());
    }
}

void ProxyObject::requestLocation(TemporalValue<Location>::Time timeStamp, const Protocol::ObjLoc& reqLoc) {
    if (mLocationAuthority) {
        mLocationAuthority->requestLocation(timeStamp, reqLoc);
    }
    else {
        Location loc;
        loc = extrapolateLocation(timeStamp);
        updateLocationWithObjLoc(loc, reqLoc);
        setLocation(timeStamp, loc);
    }
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
