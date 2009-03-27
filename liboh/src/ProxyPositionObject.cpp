/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  ProxyPositionObject.cpp
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
#include <util/Extrapolation.hpp>
#include <oh/ProxyPositionObject.hpp>
#include <oh/PositionListener.hpp>
#include <oh/ProxyManager.hpp>

namespace Sirikata {

void ProxyPositionObject::setPosition(TemporalValue<Location>::Time timeStamp,
                     const Vector3d &newPos,
                     const Quaternion &newOri) {
    Location soon=mLocation.extrapolate(timeStamp);
    setPositionVelocity(timeStamp,
                        Location(newPos,
                                 newOri,
                                 soon.getVelocity(),
                                 soon.getAxisOfRotation(),
                                 soon.getAngularSpeed()));
}
void ProxyPositionObject::setPositionVelocity(TemporalValue<Location>::Time timeStamp,
                             const Location&location) {
    mLocation.updateValue(timeStamp,
                          location);
    PositionProvider::notify(&PositionListener::updateLocation, timeStamp, location);
}
void ProxyPositionObject::resetPositionVelocity(TemporalValue<Location>::Time timeStamp,
                             const Location&location) {
    mLocation.resetValue(timeStamp,
                         location);
    PositionProvider::notify(&PositionListener::resetLocation, timeStamp, location);
}
void ProxyPositionObject::setParent(const ProxyPositionObjectPtr &parent,
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

void ProxyPositionObject::setParent(const ProxyPositionObjectPtr &parent,
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

void ProxyPositionObject::unsetParent(TemporalValue<Location>::Time timeStamp) {
    unsetParent(timeStamp, globalLocation(timeStamp));
}

void ProxyPositionObject::unsetParent(TemporalValue<Location>::Time timeStamp,
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

ProxyPositionObjectPtr ProxyPositionObject::getParentProxy() const {
    if (getParent() == SpaceObjectReference::null()) {
        return ProxyPositionObjectPtr();
    }
    ProxyObjectPtr parentProxy(getProxyManager()->getProxyObject(getParent()));
    return std::tr1::dynamic_pointer_cast<ProxyPositionObject>(parentProxy);
}

}
