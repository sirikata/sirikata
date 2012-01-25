/*  Sirikata liboh -- Object Host
 *  HostedObject.cpp
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
#include <sirikata/proxyobject/SessionEventListener.hpp>
#include <sirikata/proxyobject/Defs.hpp>
#include <sirikata/core/util/PresenceProperties.hpp>

namespace Sirikata {

/** VWObject is the basic interface that must be provided by virtual world
 *  objects. This interface gives ProxyObjects, related classes, and
 *  ProxyObject-based simulations (such as graphical display) the most basic
 *  access to the functionality of their parent virtual world object, such as
 *  movement and messaging.
 */
class SIRIKATA_PROXYOBJECT_EXPORT VWObject :
        public SelfWeakPtr<VWObject>,
        public ODP::Service,
        public SessionEventProvider
{
public:
    VWObject();
    virtual ~VWObject();

    /** Get the ProxyManager for the given presence. */
    virtual ProxyManagerPtr presence(const SpaceObjectReference& sor) { return ProxyManagerPtr(); };

    // TODO(ewencp) presence() should really be the only method and should
    // return something like PerPresenceData (or just an interface version of
    // it). That would avoid all these separate methods and just use one,
    // standard interface.
    /** Get requested location information for a presence. This should be used
     * carefully -- generally you should use a proxy object to resolve possibly
     * rejected requests rather than using this directly.
     */
    virtual SequencedPresencePropertiesPtr presenceRequestedLocation(const SpaceObjectReference& sor) { return SequencedPresencePropertiesPtr(); };
    /** Get the last seen epoch for the given presence (reported from space,
     *  there may be higher epochs for outstanding requests).
     */
    virtual uint64 presenceLatestEpoch(const SpaceObjectReference& sor) { return 0; };

}; // class VWObject

} // namespace Sirikata

#endif
