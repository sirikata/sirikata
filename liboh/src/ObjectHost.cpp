/*  Sirikata liboh -- Object Host
 *  ObjectHost.cpp
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/SpaceConnection.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <boost/thread.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/oh/TopLevelSpaceConnection.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/proxyobject/ConnectionEventListener.hpp>

#include <sirikata/core/service/Context.hpp>

namespace Sirikata {

struct ObjectHost::AtomicInt : public AtomicValue<int> {
    ObjectHost::AtomicInt*mNext;
    AtomicInt(int val, std::auto_ptr<AtomicInt>&genq) : AtomicValue<int>(val) {mNext=genq.release();}
    ~AtomicInt() {
        if (mNext)
            delete mNext;
    }
};


ObjectHost::ObjectHost(ObjectHostContext* ctx, SpaceIDMap *spaceMap, Task::WorkQueue *messageQueue, Network::IOService *ioServ, const String&options)
 : PollingService(ctx->mainStrand, Duration::seconds(1.f/30.f), ctx, "Object Host Poll"),
   mContext(ctx)
{
    mScriptPlugins=new PluginManager;
    mSpaceIDMap = spaceMap;
    mMessageQueue = messageQueue;
    mSpaceConnectionIO=ioServ;
    static std::auto_ptr<AtomicInt> gEnqueuers;
    mEnqueuers = new AtomicInt(0,gEnqueuers);
    std::auto_ptr<AtomicInt> tmp(mEnqueuers);
    gEnqueuers=tmp;
    OptionValue *protocolOptions;
    InitializeClassOptions ico("objecthost",this,
                           protocolOptions=new OptionValue("protocols","",OptionValueType<std::map<std::string,std::string> >(),"passes options into protocol specific libraries like \"tcpsst:{--send-buffer-size=1440 --parallel-sockets=1},udp:{--send-buffer-size=1500}\""),
                           NULL);
    {
        std::map<std::string,std::string> *options=&protocolOptions->as<std::map<std::string,std::string> > ();
        for (std::map<std::string,std::string>::iterator i=options->begin(),ie=options->end();i!=ie;++i) {
            mSpaceConnectionProtocolOptions[i->first]=Network::StreamFactory::getSingleton().getOptionParser(i->first)(i->second);
        }
    }

}

ObjectHost::~ObjectHost() {
    Task::WorkQueue *queue = mMessageQueue;
    mMessageQueue = NULL;
    int value=0;
    (*mEnqueuers)-=(1<<29);
    while ((value=mEnqueuers->read())!=-(1<<29)) {
        SILOG(objecthost,debug,"%d enqueuers"<<value);
        // silently wait for everyone to finish adding themselves.
    }
    queue->dequeueAll(); // filter through everything that might have an ObjectHost message in it.
    {
        HostedObjectMap objs;
        mHostedObjects.swap(objs);
        for (HostedObjectMap::iterator iter = mHostedObjects.begin();
                 iter != mHostedObjects.end();
                 ++iter) {
            iter->second->destroy();
        }
        objs.clear(); // The HostedObject destructor will attempt to delete from mHostedObjects
    }
    delete mScriptPlugins;
}

// Space API - Provide info for ObjectHost to communicate with spaces
void ObjectHost::addServerIDMap(const SpaceID& space_id, ServerIDMap* sidmap) {
    SessionManager* smgr = new SessionManager(
        mContext, space_id, sidmap,
        std::tr1::bind(&ObjectHost::handleObjectConnected, this, _1, _2),
        std::tr1::bind(&ObjectHost::handleObjectMigrated, this, _1, _2, _3),
        std::tr1::bind(&ObjectHost::handleObjectMessage, this, _1)
    );
    mSessionManagers[space_id] = smgr;
    smgr->start();
}

void ObjectHost::handleObjectConnected(const UUID& objid, ServerID server) {
}

void ObjectHost::handleObjectMigrated(const UUID& objid, ServerID from, ServerID to) {
}

void ObjectHost::handleObjectMessage(Sirikata::Protocol::Object::ObjectMessage* msg) {
}

// Primary HostedObject API

void ObjectHost::connect(
    HostedObjectPtr obj, const SpaceID& space,
    const TimedMotionVector3f& loc, const BoundingSphere3f& bnds,
    const SolidAngle& init_sa, ConnectedCallback connected_cb,
    MigratedCallback migrated_cb, StreamCreatedCallback stream_created_cb)
{
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    mSessionManagers[space]->connect(obj->getUUID(), loc, bnds, true, init_sa, connected_cb, migrated_cb, stream_created_cb);
}

void ObjectHost::connect(
    HostedObjectPtr obj, const SpaceID& space,
    const TimedMotionVector3f& loc, const BoundingSphere3f& bnds,
    ConnectedCallback connected_cb, MigratedCallback migrated_cb,
    StreamCreatedCallback stream_created_cb)
{
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    mSessionManagers[space]->connect(obj->getUUID(), loc, bnds, false, SolidAngle::Max, connected_cb, migrated_cb, stream_created_cb);
}

void ObjectHost::disconnect(HostedObjectPtr obj, const SpaceID& space) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    mSessionManagers[space]->disconnect(obj->getUUID());
}

bool ObjectHost::send(HostedObjectPtr obj, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, MemoryReference payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);

    std::string payload_str( (char*)payload.begin(), (char*)payload.end() );
    return send(obj, space, src_port, dest, dest_port, payload_str);
}

bool ObjectHost::send(HostedObjectPtr obj, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    return mSessionManagers[space]->send(obj->getUUID(), src_port, dest, dest_port, payload);
}


class ObjectHost::MessageProcessor : public Task::WorkItem {
    ObjectHost *parent;
    RoutableMessageHeader header;
    std::string body;
public:
    MessageProcessor(ObjectHost *parent,
                     const RoutableMessageHeader&header,
                     MemoryReference message_body)
        : parent(parent), header(header), body((char*)message_body.begin(), (char*)message_body.end()) {
    }
    ~MessageProcessor() {
    }

    void operator() () {
        AutoPtr delete_me(this);

        if (!header.has_destination_space() || header.destination_space() == SpaceID::null()) {
            header.set_source_space(SpaceID::null());
            ReturnStatus status = RoutableMessageHeader::SUCCESS;
            if (header.destination_object() == ObjectReference::spaceServiceID()) {
                MessageService *destService = parent->getService(header.destination_port());
                if (destService) {
                    destService->processMessage(header, MemoryReference(body));
                    return;
                }
                status = RoutableMessageHeader::PORT_FAILURE;
            } else {
                HostedObjectPtr dest = parent->getHostedObject(header.destination_object().getAsUUID());
                if (dest) {
                    dest->processRoutableMessage(header, MemoryReference(body));
                    return;
                }
                status = RoutableMessageHeader::UNKNOWN_OBJECT;
            }
            if (status) {
                RoutableMessageHeader response(header);
                response.swap_source_and_destination();
                response.set_return_status(status);
                if (header.source_object() == ObjectReference::spaceServiceID()) {
                    MessageService *srcService = parent->getService(header.destination_port());
                    if (srcService) {
                        srcService->processMessage(response, MemoryReference::null());
                    } else {
                        // Neither source service nor destination exist.
                    }
                } else {
                    HostedObjectPtr src = parent->getHostedObject(header.source_object().getAsUUID());
                    if (src) {
                        src->processRoutableMessage(response, MemoryReference::null());
                    } else {
                        // Neither source object nor destination exist.
                    }
                }
            }
        }
    }
};

void ObjectHost::processMessage(const RoutableMessageHeader&header, MemoryReference message_body) {
    DEPRECATED(ObjectHost);

    assert(header.has_destination_object());
    assert(header.has_source_object());
    assert(!header.has_source_space() || header.source_space() == header.destination_space());
    if (++*mEnqueuers>0) {
        Task::WorkQueue *queue = mMessageQueue;
        if (queue) {
            queue->enqueue(new MessageProcessor(this, header, message_body));
        }
    }
    --*mEnqueuers;
}

void ObjectHost::registerHostedObject(const HostedObjectPtr &obj) {
    mHostedObjects.insert(HostedObjectMap::value_type(obj->getUUID(), obj));
}
void ObjectHost::unregisterHostedObject(const UUID &objID) {
    HostedObjectMap::iterator iter = mHostedObjects.find(objID);
    if (iter != mHostedObjects.end()) {
        HostedObjectPtr obj (iter->second);
        mHostedObjects.erase(iter);
    }
}
HostedObjectPtr ObjectHost::getHostedObject(const UUID &id) const {
    HostedObjectMap::const_iterator iter = mHostedObjects.find(id);
    if (iter != mHostedObjects.end()) {
//        HostedObjectPtr obj(iter->second.lock());
//        return obj;
        return iter->second;
    }
    return HostedObjectPtr();
}


namespace{
    boost::recursive_mutex gSpaceConnectionMapLock;
}
void ObjectHost::insertAddressMapping(const Network::Address&addy, const std::tr1::weak_ptr<TopLevelSpaceConnection>&val){
    boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
    mAddressConnections[addy]=val;
}

void ObjectHost::removeTopLevelSpaceConnection(const SpaceID&id, const Network::Address& addy,const TopLevelSpaceConnection*example){
    boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
    {
        SpaceConnectionMap::iterator where=mSpaceConnections.find(id);
        for(;where!=mSpaceConnections.end()&&where->first==id;) {
            std::tr1::shared_ptr<TopLevelSpaceConnection> temp(where->second.lock());
            if (!temp) {
                where=mSpaceConnections.erase(where);
            }else if(&*temp==example) {
                where=mSpaceConnections.erase(where);
            }else {
                ++where;
            }
        }
        {
            AddressConnectionMap::iterator where=mAddressConnections.find(addy);
            if (where!=mAddressConnections.end()) {
                assert(!(addy==Network::Address::null()));
                std::tr1::shared_ptr<TopLevelSpaceConnection> temp(where->second.lock());
                if (!temp) {
                    mAddressConnections.erase(where);
                }else if(&*temp==example) {
                    mAddressConnections.erase(where);
                }
            }
        }
    }
    ConnectionEventProvider::notify(&ConnectionEventListener::onDisconnected, addy, false, "Unknown.");
}

void ObjectHost::poll() {
    for (HostedObjectMap::iterator iter = mHostedObjects.begin(); iter != mHostedObjects.end(); ++iter) {
        iter->second->tick();
    }
}

void ObjectHost::stop() {
    PollingService::stop();

    for(SpaceSessionManagerMap::iterator it = mSessionManagers.begin(); it != mSessionManagers.end(); it++) {
        SessionManager* sm = it->second;
        sm->stop();
    }
}

const Duration&ObjectHost::getSpaceTimeOffset(const SpaceID&id)const{
    SpaceConnectionMap::const_iterator where=mSpaceConnections.find(id);
    if (where!=mSpaceConnections.end()) {
        std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection=where->second.lock();
        if (topLevelConnection) {
            return topLevelConnection->getServerTimeOffset();
        }
    }
    static Duration nil(Duration::seconds(0));
    return nil;
}
const Duration&ObjectHost::getSpaceTimeOffset(const Network::Address&id)const{
    boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
    AddressConnectionMap::const_iterator where=mAddressConnections.find(id);
    if (where!=mAddressConnections.end()) {
        std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection=where->second.lock();
        if (topLevelConnection) {
            return topLevelConnection->getServerTimeOffset();
        }
    }
    static Duration nil(Duration::seconds(0));
    return nil;
}

void ObjectHost::dequeueAll() const {
    if (getWorkQueue()) {
        getWorkQueue()->dequeueAll();
    }
}

ProxyManager *ObjectHost::getProxyManager(const SpaceID&space) const {
    SpaceConnectionMap::const_iterator iter = mSpaceConnections.find(space);
    if (iter != mSpaceConnections.end()) {
        std::tr1::shared_ptr<TopLevelSpaceConnection> spaceConnPtr = iter->second.lock();
        if (spaceConnPtr) {
            return spaceConnPtr.get();
        }
    }
    return NULL;
}



} // namespace Sirikata
