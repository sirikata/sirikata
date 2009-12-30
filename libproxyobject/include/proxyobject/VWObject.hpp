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


class SIRIKATA_PROXYOBJECT_EXPORT VWObject : public SelfWeakPtr<VWObject>{
protected:
//------- Members
    QueryTracker mTracker;
    /// Call if you know that a position for some other ProxyObject has changed. FIXME: should this be made private?
    void applyPositionUpdate(const ProxyObjectPtr &proxy, const Protocol::ObjLoc &objLoc, bool force_reset);
    static void parsePhysicalParameters(PhysicalParameters &out, const Protocol::PhysicalParameters &parsedProperty);
public:
    ///determine if this object is an object hosted by this computer
    virtual bool isLocal(const SpaceObjectReference&)const=0;
    virtual ProxyManager*getProxyManager(const SpaceID&space)=0;
    virtual QueryTracker*getTracker()=0;
    virtual void addQueryInterest(uint32 query_id, const SpaceObjectReference&ref)=0;
    virtual void removeQueryInterest(uint32 query_id, const ProxyObjectPtr&obj, const SpaceObjectReference&)=0;
    VWObject(Network::IOService*);
    virtual ~VWObject();
    static void receivedProxObjectLocation(
        const VWObjectWPtr &weakThis,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData,
        int32 queryId);
    static void receivedProxObjectProperties( 
            const VWObjectWPtr &weakThis,
            SentMessage* sentMessageBase,
            const RoutableMessageHeader &hdr,
            uint64 returnStatus,//type Persistence::Protocol::Response::ReturnStatus
            int32 queryId,
            const Protocol::ObjLoc &objLoc);
    static void receivedPositionUpdateResponse(
        const VWObjectWPtr &weakThis,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData);
    void receivedPropertyUpdate(
        const ProxyObjectPtr &proxy,
        const std::string &propertyName,
        const std::string &arguments);

    void processRPC(const RoutableMessageHeader&msg, const std::string &name, MemoryReference args, String *response);
    
};

}

#endif
