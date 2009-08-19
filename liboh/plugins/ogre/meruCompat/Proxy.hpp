/*  Meru
 *  Proxy.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _MERU_PROXY_HPP_
#define _MERU_PROXY_HPP_

#include "MeruDefs.hpp"
#include "Event.hpp"
#include "../OgreSystem.hpp"
#include "../Entity.hpp"
#include "oh/SpaceTimeOffsetManager.hpp"
namespace Meru {

class ProxyObject;
typedef ProxyObject *SharedProxyPtr;
typedef ProxyObject *WeakProxyPtr;

/** Maintains state for a proxy to a virtual world object stored
 *  on the server.  This includes both the Ruby proxy object and
 *  the Ogre state associated with it.
 */
class ProxyObject {
  Sirikata::Graphics::Entity *mEntity;
public:
    ProxyObject() : mEntity(NULL),mSpace(SpaceID::null()) {}

    /** Initializes the proxy.  This performs initalizations that
     *  call virtual methods that cannot be called during construction.
     *  This should *always* be called immediately after construction.
     */
    void initialize(Sirikata::Graphics::OgreSystem *sys, const SpaceObjectReference& ref) {
      mSpace=ref.space();
      mEntity = sys->getEntity(ref);
    }

    /** Initializes the proxy.  This performs initalizations that
     *  call virtual methods that cannot be called during construction.
     *  This should *always* be called immediately after construction.
     */
    void initialize(Sirikata::Graphics::Entity *ent) {
      mEntity = ent;
    }

    /** Get the short reference of this proxy.
     *  \returns a ObjectReference for this proxy
     */
    inline const SpaceObjectReference &reference() const {
        return mEntity->id();
    }

    /** Update this objects location. Might also, e.g. send updates to the server.
     *  \param t the local time at which these values were valid
     *  \param updated_loc the updated location
     */
    /*
    void updatePosition(const Time& t, const Location& updated_loc) {
      mEntity->updatePosition(updated_loc, t);
    }
    */
    /** Update the position of this object by replacing it with a predicted value at Time t.
     *  \param t the time at which to predict the location
     */
    /*
     * // already called extrapolateTime
     * void predictLocation(Time t);
     */

    /** Animate this proxy with the specified animation.
     *  \param animation_name the name of the animation to use
     */
    void setSelected(bool selected);

    void setLocalLocation(const Location &loc);
    const Location getLocation()const {
        return mEntity->getProxy().extrapolateLocation(Sirikata::SpaceTimeOffsetManager::getSingleton().now(mSpace));
    }
    void ignoreNetwork(bool ignore);
    const String& getName() const {
      return mName;
    }
    void setName(const String &name) {
      mName = name;
    }
    bool isSelected() const {
      return mSelected;
    }

  protected:

    /** Returns true if the object is visually complete, i.e. all the information needed
     *  to display it is available.
     */
    bool graphicallyComplete();

    /** Callback for initial position lookup. */
    void setInitialPosition(const EventPtr& evt, uint32 msgid);

    /*
    SpaceObjectReference mReference;
    Mono::Object mScriptingProxy;
    GraphicsEntity* mEntity;
    GraphicsLight* mLight;
    Location mLocation;
    LocationInterpolator* mLocationInterpolator;
    int mIgnoreNetwork;
    */
    SpaceID mSpace;
    String mName;
    bool mNamed;
    bool mSelected;
};

} // namespace Meru

#endif //_PROXY_HPP_
