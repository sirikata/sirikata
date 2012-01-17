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

#include <sirikata/proxyobject/Platform.hpp>

#include <sirikata/proxyobject/Defs.hpp>

#include <sirikata/core/util/Extrapolation.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include "PositionListener.hpp"

#include <sirikata/core/odp/Service.hpp>

#include "VWObject.hpp"

#include <sirikata/proxyobject/MeshListener.hpp>
#include "MeshListener.hpp"

#include <sirikata/proxyobject/PresenceProperties.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>

#include <sirikata/core/util/SerializationCheck.hpp>

namespace Sirikata {


//forward declares
class MeshListener;
class ProxyObjectListener;


//typedefs
typedef Provider<PositionListener*> PositionProvider;
typedef Provider<ProxyObjectListener*> ProxyObjectProvider;
typedef Provider< MeshListener* > MeshProvider;




/** Interface to listen for the destruction of a ProxyObject so one can discard any shared references to it. */
class SIRIKATA_PROXYOBJECT_EXPORT ProxyObjectListener {
public:
    virtual ~ProxyObjectListener() {}
    // Invoked when the object enters the result set.  Updates will start
    // flowing in for this object. Note that this could be called as opposed to
    // the actual creation of the ProxyObject, because the ProxyObject still
    // exists but had already been invalidated.
    virtual void validated(ProxyObjectPtr proxy) = 0;
    // Invoked when the object falls out of the result set. No further updates
    // will be received.
    virtual void invalidated(ProxyObjectPtr proxy, bool permanent) = 0;
    // Invoked when the ProxyObject is actually destroyed.
    virtual void destroyed(ProxyObjectPtr proxy) = 0;
};




/**
 * This class represents a generic object on a remote server. This
 * tracks the basic properties maintained by the space -- the object
 * identifier, location, orientation, bounds, mesh, and physics. Note
 * that this *always* represents the current reported status of the
 * object in the space, even if you own the presence.
 *
 * Note that this class is *not* thread safe. You need to protect it by locking
 * a mutex from the ProxyManager or HostedObject while accessing it.
 */
class SIRIKATA_PROXYOBJECT_EXPORT ProxyObject
    : public SelfWeakPtr<ProxyObject>,
      public SequencedPresenceProperties,
      public ProxyObjectProvider,
      public PositionProvider,
      public MeshProvider,
      SerializationCheck
{

public:
    static ProxyObjectPtr construct(ProxyManagerPtr man, const SpaceObjectReference& id);

    class SIRIKATA_PROXYOBJECT_EXPORT UpdateNeeded {
    public:
        bool operator()(
            const Location&updatedValue,
            const Location&predictedValue) const;
    };
    typedef TimedWeightedExtrapolator<Location,UpdateNeeded> Extrapolator;

private:
    const SpaceObjectReference mID;
    ProxyManagerPtr mParent;
public:
    /** Constructs a new ProxyObject. After constructing this object, it
        should be wrapped in a shared_ptr and sent to ProxyManager::createObject().
        @param man  The ProxyManager controlling this object.
        @param id  The SpaceID and ObjectReference assigned to this proxyObject.
        \param vwobj the owning VWObject, allowing the ProxyObject to interact
                    with the space
    */
    ProxyObject(ProxyManagerPtr man, const SpaceObjectReference& id);


    /// Subclasses can do any necessary cleanup first.
    virtual void destroy();

    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    inline const SpaceObjectReference& getObjectReference() const {
        return mID;
    }
    inline const SpaceObjectReference& getOwnerPresenceID() const {
        return getOwner()->id();
    }

    /// Gets the owning ProxyManager
    ProxyManagerPtr getOwner() const { return mParent; }

    /// Returns the last updated position for this object.
    inline Vector3d getPosition() const{
        return Vector3d(mLoc.position());
    }


    /// returns the last updated velocity for this object
    inline Vector3d getVelocity() const
    {
        return Vector3d(mLoc.velocity());
    }

    /// Returns the last updated Quaternion for this object.
    inline const Quaternion& getOrientation() const{
        return mOrientation.position();
    }

    /// Returns the Quaternion speed (I know that's not the right term; maybe
    /// angular velocity???) for this object.
    inline const Quaternion& getOrientationSpeed() const{
        return mOrientation.velocity();
    }



    ~ProxyObject();

    // Resets the state of this proxy as if it had been freshly created
    void reset();

    /** Marks this ProxyObject as validated, which indicates that updates about
     *  the object's properties will be provided by the system.
     */
    void validate();
    /** Marks this ProxyObject as invalidated, which indicates that although
     *  messages can still be sent and the last values of properties are
     *  available, no further updates will be received.
     */
    void invalidate(bool permanent);

    /// Returns if this object has a zero velocity and requires no extrapolation.
    bool isStatic() const;

    // PresenceProperties Overrides
    virtual const TimedMotionVector3f& location() const;
    virtual const TimedMotionQuaternion& orientation() const;
    virtual const BoundingSphere3f& bounds() const;
    virtual const Transfer::URI& mesh() const;
    virtual const String& physics() const;

    void setLocation(const TimedMotionVector3f& reqloc, uint64 seqno);
    void setOrientation(const TimedMotionQuaternion& reqorient, uint64 seqno);
    void setBounds(const BoundingSphere3f& bnds, uint64 seqno);
    void setMesh (Transfer::URI const& rhs, uint64 seqno);
    void setPhysics(const String& rhs, uint64 seqno);



    /** Retuns the local location of this object at the current timestamp. */
    Location extrapolateLocation(TemporalValue<Location>::Time current) const {
        Vector3f angaxis;
        float32 angvel;
        mOrientation.velocity().toAngleAxis(angvel, angaxis);

        return Location(Vector3d(mLoc.position(current)), mOrientation.position(current).normal(), mLoc.velocity(), angaxis, angvel);
    }


    unsigned int hash() const {
        return getOwnerPresenceID().hash() ^ getObjectReference().hash();
    }

    class Hasher{
    public:
        size_t operator() (const ProxyObject& p) const {
            return p.hash();
        }
        size_t operator() (ProxyObject* p) const {
            return p->hash();
        }
        size_t operator() (const ProxyObjectPtr& p) const {
            return p->hash();
        }
    };

};
}
#endif
