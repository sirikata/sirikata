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


namespace Sirikata {
class PhysicalParameters;
namespace Protocol {
class ObjLoc;
class PhysicalParameters;
}
class VWObject;
/// shared_ptr, keeps a reference to the VWObject. Do not store one of these.
typedef std::tr1::shared_ptr<VWObject> VWObjectPtr;
/// weak_ptr, to be used to hold onto a VWObject reference. @see SelfWeakPtr.
typedef std::tr1::weak_ptr<VWObject> VWObjectWPtr;

/**
 * An interface that all virtual world objects that wish to relate to ProxyObjects should provide
 * These objects must be able to keep data on outstanding queries using the QueryTracker
 * They must be able to get a ProxyManager per space to which they are connected:
 *   In general any camera-like VWObject will be a ProxyManager for ProxyObjects it makes in a space
 * The HostedObject must be able to track reported interest in a given object based on that object's query id
 *   The addQueryInterest function will be called when 
 *   the processRPC function determines that a ProxCall method has returned a particular SpaceObjectReference
 *   The remoteQueryInterest function will be called when
 *   the processRPC function determines that a ProxCall method has invalidated a paricular SpaceObjectReference
 */
class SIRIKATA_PROXYOBJECT_EXPORT VWObject : public SelfWeakPtr<VWObject>{
private:
    static void receivedProxObjectProperties( 
            const VWObjectWPtr &weakThis,
            SentMessage* sentMessageBase,
            const RoutableMessageHeader &hdr,
            uint64 returnStatus,
            int32 queryId,
            const Protocol::ObjLoc &objLoc);
    static void receivedPositionUpdateResponse(
        const VWObjectWPtr &weakThis,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData);
    static void receivedProxObjectLocation(
        const VWObjectWPtr &weakThis,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData,
        int32 queryId);
protected:
    ///This convenience function takes a recently deserialized property argument and applies it to a ProxyObject, notifying the ProxyObject's listeners about relevent changes 
    void receivedPropertyUpdate(
        const ProxyObjectPtr &proxy,
        const std::string &propertyName,
        const std::string &arguments);
    ///This convenience function takes a recently deserialized objLoc struct and applies it to a ProxyObject notifying the ProxyObject's listeners of changes
    void applyPositionUpdate(const ProxyObjectPtr &proxy, const Protocol::ObjLoc &objLoc, bool force_reset);
    ///This function translates Protobuf parsedProperties into a structure of them for making a ProxyObject that can be physical
    static void parsePhysicalParameters(PhysicalParameters &out, const Protocol::PhysicalParameters &parsedProperty);
    ///This must be called when a HostedObject has received an RPC message and pulled it apart into its respective args
    void processRPC(const RoutableMessageHeader&msg, const std::string &name, MemoryReference args, String *response);

public:
    VWObject();
    virtual ~VWObject();
    ///The tracker managing state for outstanding requests this object has made
    virtual QueryTracker*getTracker()=0;
    ///The ProxyManager this VWObject is responsible for (or partially responsible as in the current OH design) given the space ID
    virtual ProxyManager*getProxyManager(const SpaceID&space)=0;
    ///determine if objectId is an object hosted by this computer so messages to it may directly reach it
    virtual bool isLocal(const SpaceObjectReference&objectId)const=0;

    ///a callback to this object telling it that an object has entered its region of interest for query query_id
    virtual void addQueryInterest(uint32 query_id, const SpaceObjectReference&ref)=0;
    ///a callback to this object telling it that an object with an instantiated ProxyObject has exited its region of interest for query query_id
    virtual void removeQueryInterest(uint32 query_id, const ProxyObjectPtr&obj, const SpaceObjectReference&)=0;

    
};

}

#endif
