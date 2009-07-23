/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  ProxyObject.hpp
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

#ifndef _SIRIKATA_PROXY_OBJECT_HPP_
#define _SIRIKATA_PROXY_OBJECT_HPP_
#include <util/Extrapolation.hpp>
#include <util/SpaceObjectReference.hpp>
#include "ProxyObjectListener.hpp"
#include "ProxyObject.hpp"
#include <util/ListenerProvider.hpp>
#include "PositionListener.hpp"

namespace Sirikata {

class ProxyObject;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;
typedef std::tr1::weak_ptr<ProxyObject> ProxyObjectWPtr;

typedef Provider<ProxyObjectListener*> ProxyObjectProvider;
class ProxyManager;

typedef double AbsTime;

typedef Provider<PositionListener*> PositionProvider;

/**
 * This class represents a generic object on a remote server
 * Every object has a SpaceObjectReference that allows one to communicate
 * with it. Subclasses implement several Providers for concerned listeners
 * This class should be casted to the various subclasses (ProxyLightObject,etc)
 * and appropriate listeners be registered.
 */
class SIRIKATA_OH_EXPORT ProxyObject
  : public ProxyObjectProvider,
    public PositionProvider,
    protected ProxyObjectListener // Parent death notification. FIXME: or should we leave the parent here, but ignore it in globalLocation()???
{

    class UpdateNeeded {
    public:
        bool operator()(
            const Location&updatedValue,
            const Location&predictedValue) const;
    };

private:
    const SpaceObjectReference mID;
    ProxyManager *const mManager;

    TimedWeightedExtrapolator<Location,UpdateNeeded> mLocation;
    SpaceObjectReference mParentId;
protected:
    // Notification that the Parent has been destroyed.
    virtual void destroyed();

public:
    ProxyObject(ProxyManager *man, const SpaceObjectReference&id);
    virtual ~ProxyObject();

    /// Subclasses can do any necessary cleanup first.
    virtual void destroy();


    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    inline const SpaceObjectReference&getObjectReference() const{
        return mID;
    }
    inline ProxyManager *getProxyManager() const {
        return mManager;
    }

    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    inline const Vector3d& getPosition() const{
        return mLocation.lastValue().getPosition();
    }
    inline const Quaternion& getOrientation() const{
        return mLocation.lastValue().getOrientation();
    }
    inline const SpaceObjectReference& getParent() const{
        return mParentId;
    }

    ProxyObjectPtr getParentProxy() const;

    inline TemporalValue<Location>::Time getLastUpdated() const {
        return mLocation.lastUpdateTime();
    }

    bool isStatic(const TemporalValue<Location>::Time& when) const;

    void setLocation(TemporalValue<Location>::Time timeStamp,
                             const Location&location);
    void resetLocation(TemporalValue<Location>::Time timeStamp,
                               const Location&location);

    void setParent(const ProxyObjectPtr &parent,
               TemporalValue<Location>::Time timeStamp,
               const Location &absLocation,
               const Location &relLocation);
    void unsetParent(TemporalValue<Location>::Time timeStamp,
               const Location &absLocation);

    void setParent(const ProxyObjectPtr &parent,
               TemporalValue<Location>::Time timeStamp);
    void unsetParent(TemporalValue<Location>::Time timeStamp);

    Location globalLocation(TemporalValue<Location>::Time timeStamp) const {
        ProxyObjectPtr ppop = getParentProxy();
        if (ppop) {
            return extrapolateLocation(timeStamp).
                toWorld(ppop->globalLocation(timeStamp));
        } else {
            return extrapolateLocation(timeStamp);
        }
    }

    Location extrapolateLocation(TemporalValue<Location>::Time current) const {
        return mLocation.extrapolate(current);
    }
};
}
#endif
