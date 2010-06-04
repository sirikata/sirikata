/*  Sirikata Proximity Management -- Introduction Services
 *  ProximitySystem.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include <sirikata/core/util/MessageService.hpp>
#ifndef _PROXIMITY_PROXIMITYSYSTEM_HPP_
#define _PROXIMITY_PROXIMITYSYSTEM_HPP_
namespace Sirikata { namespace Proximity {
class SIRIKATA_PROXIMITY_EXPORT ProximitySystem :public MessageService{
public:
    static void defaultProximityCallback(Network::Stream*, const RoutableMessageHeader&,const Sirikata::RoutableMessageBody&);
    static void defaultNoDestinationAddressProximityCallback(Network::Stream*, const RoutableMessageHeader&,const Sirikata::RoutableMessageBody&);
    static void defaultNoAddressProximityCallback(Network::Stream*, const RoutableMessageHeader&,const Sirikata::RoutableMessageBody&);
    typedef std::tr1::function<void(Network::Stream*, const RoutableMessageHeader&,const Sirikata::RoutableMessageBody&)> Callback;
    virtual ~ProximitySystem();

    enum OpaqueMessageReturnValue{
        OBJECT_NOT_DESTROYED,
        OBJECT_DELETED
    };

    /**
     * Pass the ReturnedObjectConnection info,
     * containing an Object UUID to the proximity manager,
     * so the proximity system knows about a new object
     */
    virtual ObjectReference newObj(const Sirikata::Protocol::IRetObj&,
                                   const void *optionalSerializedReturnObjectConnection=NULL,
                                   size_t optionalSerializedReturnObjectConnectionSize=0)=0;
    /**
     * Register a new proximity query.
     * The callback may come from an ASIO response thread
     */
    virtual void newProxQuery(const ObjectReference&source,
                              Sirikata::uint32  source_port,
                              const Sirikata::Protocol::INewProxQuery&,
                              const void *optionalSerializedProximityQuery=NULL,
                              size_t optionalSerializedProximitySize=0)=0;
    /**
     * Since certain setups have proximity responses coming from another message stream
     * Those messages should be shunted to this function and processed
     */
    virtual void processProxCallback(const ObjectReference&destination,
                                     const Sirikata::Protocol::IProxCall&,
                                     const void *optionalSerializedProxCall=NULL,
                                     size_t optionalSerializedProxCallSize=0)=0;
    /**
     * The proximity management system must be informed of all position updates
     * Pass an objects position updates to this function
     */
    virtual void objLoc(const ObjectReference&source, const Sirikata::Protocol::IObjLoc&, const void *optionalSerializedObjLoc=NULL,size_t optionalSerializedObjLocSize=0)=0;

    /**
     * Objects may lose interest in a particular query
     * when this function returns, no more responses will be given
     */
    virtual void delProxQuery(const ObjectReference&source, Sirikata::uint32  source_port,const Sirikata::Protocol::IDelProxQuery&cb,  const void *optionalSerializedDelProxQuery=NULL,size_t optionalSerializedDelProxQuerySize=0)=0;
    /**
     * Objects may be destroyed: indicate loss of interest here
     */
    virtual void delObj(const ObjectReference&source, const Sirikata::Protocol::IDelObj&, const void *optionalSerializedDelObj=NULL,size_t optionalSerializedDelObjSize=0)=0;

};

} }
#endif
