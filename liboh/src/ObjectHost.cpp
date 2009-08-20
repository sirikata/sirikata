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

#include <oh/Platform.hpp>
#include "oh/SpaceConnection.hpp"
#include <oh/HostedObject.hpp>
#include <oh/ObjectHost.hpp>
#include <boost/thread.hpp>
#include <network/IOServiceFactory.hpp>
#include <util/AtomicTypes.hpp>
#include <util/RoutableMessageHeader.hpp>
#include <task/WorkQueue.hpp>
#include "graphics/GraphicsObject.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/ObjectScriptManager.hpp"
#include "oh/ObjectScript.hpp"
#include "oh/ObjectScriptManagerFactory.hpp"


namespace Sirikata {

struct ObjectHost::AtomicInt : public AtomicValue<int> {
    ObjectHost::AtomicInt*mNext;
    AtomicInt(int val, std::auto_ptr<AtomicInt>&genq) : AtomicValue<int>(val) {mNext=genq.release();}
    ~AtomicInt() {
        if (mNext)
            delete mNext;
    }
};
ObjectHost::ObjectHost(SpaceIDMap *spaceMap, Task::WorkQueue *messageQueue, Network::IOService *ioServ) {
    mSpaceIDMap = spaceMap;
    mMessageQueue = messageQueue;
    mSpaceConnectionIO=ioServ;
    static std::auto_ptr<AtomicInt> gEnqueuers;
    mEnqueuers = new AtomicInt(0,gEnqueuers);
    std::auto_ptr<AtomicInt> tmp(mEnqueuers);
    gEnqueuers=tmp;
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
        objs.clear(); // The HostedObject destructor will attempt to delete from mHostedObjects
    }
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
        } else {
            std::tr1::shared_ptr<TopLevelSpaceConnection> tlsc;
            SpaceConnectionMap::iterator iter = parent->mSpaceConnections.find(header.destination_space());
            if (iter != parent->mSpaceConnections.end()) {
                tlsc = iter->second.lock();
            }
            HostedObjectPtr dest;
            if (!tlsc) {
                SILOG(cppoh, error, "Invalid destination space in ObjectHost::processMessage");
                // ERROR: the space does not exist on this OH.
                assert(tlsc);
                return;
            }
            if (header.destination_object() != ObjectReference::spaceServiceID()) {
                dest = tlsc->getHostedObject(header.destination_object());
            }
            if (dest) {
                header.set_source_space(header.destination_space());
                dest->processRoutableMessage(header, MemoryReference(body));
            } else {
                HostedObjectPtr src;
                if (tlsc) {
                    src = tlsc->getHostedObject(header.source_object());
                }
                if (src) {
                    src->sendViaSpace(header, MemoryReference(body));
                } else {
                    // ERROR: neither sender nor receiver exist on this OH...
                    return;
                }
            }
        }
    }
};

void ObjectHost::processMessage(const RoutableMessageHeader&header, MemoryReference message_body) {
    assert(header.has_destination_object());
    assert(header.has_source_object());
    assert(!header.has_source_space() || header.source_space() == header.destination_space());
    if (++mEnqueuers>0) {
        Task::WorkQueue *queue = mMessageQueue;
        if (queue) {
            queue->enqueue(new MessageProcessor(this, header, message_body));
        }
    }
    --mEnqueuers;
}

void ObjectHost::registerHostedObject(const HostedObjectPtr &obj) {
    mHostedObjects.insert(HostedObjectMap::value_type(obj->getUUID(), obj));
}
void ObjectHost::unregisterHostedObject(const UUID &objID) {
    HostedObjectMap::iterator iter = mHostedObjects.find(objID);
    if (iter != mHostedObjects.end()) {
        mHostedObjects.erase(iter);
    }
}
HostedObjectPtr ObjectHost::getHostedObject(const UUID &id) const {
    HostedObjectMap::const_iterator iter = mHostedObjects.find(id);
    if (iter != mHostedObjects.end()) {
//        HostedObjectPtr obj(iter->second.lock());
//        assert(obj);
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
std::tr1::shared_ptr<TopLevelSpaceConnection> ObjectHost::connectToSpace(const SpaceID&id){
    std::tr1::shared_ptr<TopLevelSpaceConnection> retval;
    {
        boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
        SpaceConnectionMap::iterator where=mSpaceConnections.find(id);
        if ((where==mSpaceConnections.end())||((retval=where->second.lock())==NULL)) {
            std::tr1::shared_ptr<TopLevelSpaceConnection> temp(new TopLevelSpaceConnection(mSpaceConnectionIO));
            temp->connect(temp,this,id);//inserts into mSpaceConnections and eventuallly mAddressConnections
            retval = temp;
            if (where==mSpaceConnections.end()) {
                mSpaceConnections.insert(SpaceConnectionMap::value_type(id,retval));
            }else {
                SILOG(oh,warning,"Null spaceid->connection mapping");
                where->second=retval;
            }
        }
    }
    return retval;
}


std::tr1::shared_ptr<TopLevelSpaceConnection> ObjectHost::connectToSpaceAddress(const SpaceID&id, const Network::Address&addy){
    std::tr1::shared_ptr<TopLevelSpaceConnection> retval;
    {
        boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
        AddressConnectionMap::iterator where=mAddressConnections.find(addy);
        if ((where==mAddressConnections.end())||(!(retval=where->second.lock()))) {
            std::tr1::shared_ptr<TopLevelSpaceConnection> temp(new TopLevelSpaceConnection(mSpaceConnectionIO));
            temp->connect(temp,this,id,addy);//inserts into mSpaceConnections and eventuallly mAddressConnections
            retval = temp;
            if (where==mAddressConnections.end()) {
                mAddressConnections[addy]=temp;
            }else {
                SILOG(oh,warning,"Null spaceid->connection mapping");
                where->second=retval;
            }
            mSpaceConnections.insert(SpaceConnectionMap::value_type(id,retval));
        }
    }
    return retval;
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
}

void ObjectHost::tick() {
    for (HostedObjectMap::iterator iter = mHostedObjects.begin(); iter != mHostedObjects.end(); ++iter) {
        iter->second->tick();
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

