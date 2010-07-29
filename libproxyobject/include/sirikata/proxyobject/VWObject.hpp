/*  Sirikata liboh -- Object Host
 *  VWObject.hpp
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

#ifndef _SIRIKATA_PROXYOBJECT_VWOBJECT_HPP_
#define _SIRIKATA_PROXYOBJECT_VWOBJECT_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/odp/Service.hpp>
#include <sirikata/core/util/MotionVector.hpp>

namespace Sirikata {

class QueryTracker;

/** VWObject is the basic interface that must be provided by virtual world
 *  objects. This interface gives ProxyObjects, related classes, and
 *  ProxyObject-based simulations (such as graphical display) the most basic
 *  access to the functionality of their parent virtual world object, such as
 *  movement and messaging.
 */
class SIRIKATA_PROXYOBJECT_EXPORT VWObject : public SelfWeakPtr<VWObject>, public ODP::Service {
public:
    VWObject();
    virtual ~VWObject();

    ///The tracker managing state for outstanding requests this object has made
    virtual QueryTracker*getTracker(const SpaceID& space)=0;

    // Identification
    virtual SpaceObjectReference id(const SpaceID& space) const = 0;

    // ODP::Service Interface
    virtual ODP::Port* bindODPPort(SpaceID space, ODP::PortID port) = 0;
    virtual ODP::Port* bindODPPort(SpaceID space) = 0;
    virtual void registerDefaultODPHandler(const ODP::MessageHandler& cb) = 0;
    virtual void registerDefaultODPHandler(const ODP::OldMessageHandler& cb) = 0;

    // Movement Interface
    virtual void requestLocationUpdate(const SpaceID& space, const TimedMotionVector3f& loc) = 0;
    virtual void requestBoundsUpdate(const SpaceID& space, const BoundingSphere3f& bounds) = 0;
    virtual void requestMeshUpdate(const SpaceID& space, const String& mesh) = 0;
}; // class VWObject

typedef std::tr1::shared_ptr<VWObject> VWObjectPtr;
typedef std::tr1::weak_ptr<VWObject> VWObjectWPtr;

} // namespace Sirikata

#endif
