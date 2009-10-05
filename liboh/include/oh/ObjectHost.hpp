/*  Sirikata liboh -- Object Host
 *  ObjectHost.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_OBJECT_HOST_HPP_
#define _SIRIKATA_OBJECT_HOST_HPP_

#include <oh/Platform.hpp>
#include <util/MessageService.hpp>
#include <util/SpaceObjectReference.hpp>
#include <network/Address.hpp>
namespace Sirikata {
class ProxyManager;
class PluginManager;
class SpaceIDMap;
class TopLevelSpaceConnection;
class SpaceConnection;
class ObjectScriptManager;
namespace Task {
class WorkQueue;
}
class HostedObject;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;

class SIRIKATA_OH_EXPORT ObjectHost :public MessageService{
    SpaceIDMap *mSpaceIDMap;
    typedef std::tr1::unordered_multimap<SpaceID,std::tr1::weak_ptr<TopLevelSpaceConnection>,SpaceID::Hasher> SpaceConnectionMap;
    typedef std::tr1::unordered_map<Network::Address,std::tr1::weak_ptr<TopLevelSpaceConnection>,Network::Address::Hasher> AddressConnectionMap;

    typedef std::tr1::unordered_map<UUID, HostedObjectPtr, UUID::Hasher> HostedObjectMap;
    typedef std::map<MessagePort, MessageService *> ServicesMap;
    
    SpaceConnectionMap mSpaceConnections;
    AddressConnectionMap mAddressConnections;
    friend class TopLevelSpaceConnection;
    void insertAddressMapping(const Network::Address&, const std::tr1::weak_ptr<TopLevelSpaceConnection>&);
    void removeTopLevelSpaceConnection(const SpaceID&, const Network::Address&, const TopLevelSpaceConnection*);
    Network::IOService *mSpaceConnectionIO;

    Task::WorkQueue *volatile mMessageQueue;
    struct AtomicInt;
    AtomicInt *mEnqueuers;

    HostedObjectMap mHostedObjects;
    ServicesMap mServices;
    PluginManager *mScriptPlugins;
public:

    /** Caller is responsible for starting a thread
     *
     * @param ioServ = Network::IOServiceFactory::makeIOService();
     *
     * Destroy it with Network::IOServiceFactory::destroyIOService(ioServ);
     */
    ObjectHost(SpaceIDMap *spaceIDMap, Task::WorkQueue *messageQueue, Network::IOService*ioServ);
    /// The ObjectHost must be destroyed after all HostedObject instances.
    ~ObjectHost();
    ///ObjectHost does not forward messages to other services, only to objects it owns
    bool forwardMessagesTo(MessageService*){return false;}
    ///ObjectHost does not forward messages to other services, only to objects it owns
    bool endForwardingMessagesTo(MessageService*){return false;}

    /** Register a global space-like service for null Space, null Object.
        @param port  The service port number (i.e. Services::PERSISTENCE)
        @param serv  MessageService* -- make sure to unregister before deleting.
    */
    void registerService(MessagePort port, MessageService *serv) {
        mServices.insert(ServicesMap::value_type(port, serv));
        serv->forwardMessagesTo(this);
    }
    /// Unregister a global service. Unnecessary if you delete the ObjectHost first.
    void unregisterService(MessagePort port) {
        ServicesMap::iterator iter = mServices.find(port);
        if (iter != mServices.end()) {
            iter->second->endForwardingMessagesTo(this);
            mServices.erase(iter);
        }
    }
    /// Lookup a global service by port number.
    MessageService *getService(MessagePort port) const {
        ServicesMap::const_iterator iter = mServices.find(port);
        if (iter != mServices.end()) {
            return iter->second;
        }
        return NULL;
    }
    const Duration&getSpaceTimeOffset(const SpaceID&)const;
    const Duration&getSpaceTimeOffset(const Network::Address&)const;
    /** Register object by private UUID, so that it is possible to
        talk to objects/services which are not part of any space.
        Done automatically by HostedObject::initialize* functions.
    */
    void registerHostedObject(const HostedObjectPtr &obj);
    /// Unregister a private UUID. Done automatically by ~HostedObject.
    void unregisterHostedObject(const UUID &objID);

    /** Lookup HostedObject by private UUID. */
    HostedObjectPtr getHostedObject(const UUID &id) const;

    /// Returns the SpaceID -> Network::Address lookup map.
    SpaceIDMap*spaceIDMap(){return mSpaceIDMap;}

    class MessageProcessor;
    ///This method checks if the message is destined for any named mServices. If not, it gives it to mRouter
    void processMessage(const RoutableMessageHeader&header,
                        MemoryReference message_body);
    /// @see connectToSpaceAddress. Looks up the space in the spaceIDMap().
    std::tr1::shared_ptr<TopLevelSpaceConnection> connectToSpace(const SpaceID& space);
    ///immediately returns a usable stream for the spaceID. The stream may or may not connect successfully, but will allow queueing messages. The stream will be deallocated if the return value is discarded. In most cases, this should not be called directly.
    std::tr1::shared_ptr<TopLevelSpaceConnection> connectToSpaceAddress(const SpaceID&, const Network::Address&);

    void tick();
    PluginManager *getScriptPluginManager(){return mScriptPlugins;}
    /** Gets an IO service corresponding to this object host.
        This can be used to schedule timeouts that are guaranteed
        to be in the correct thread. */
    Network::IOService *getSpaceIO() const {
        return mSpaceConnectionIO;
    }
    Task::WorkQueue *getWorkQueue() const {
        return mMessageQueue;
    }
    /// Looks up a TopLevelSpaceConnection corresponding to a certain space.
    ProxyManager *getProxyManager(const SpaceID&space) const;
}; // class ObjectHost

} // namespace Sirikata

#endif //_SIRIKATA_OBJECT_HOST_HPP
