/*  cbr
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "ObjectHost.hpp"
#include "Statistics.hpp"
#include "Object.hpp"
#include "ObjectFactory.hpp"
#include "Server.hpp"

namespace CBR {

ObjectHost::ObjectHost(ObjectHostID _id, ObjectFactory* obj_factory, Trace* trace)
 : mContext( new ObjectHostContext(_id) )
{
    mContext->objectHost = this;
    mContext->objectFactory = obj_factory;
    mContext->trace = trace;
}

ObjectHost::~ObjectHost() {
    delete mContext;
}

const ObjectHostContext* ObjectHost::context() const {
    return mContext;
}

SpaceConnection* ObjectHost::openConnection(Object* obj) {
    assert(false); // FIXME need to open connection
    //mServer->handleOpenConnection(conn);
    return NULL;
}

bool ObjectHost::send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    CBR::Protocol::Object::ObjectMessage obj_msg;
    obj_msg.set_source_object(src->uuid());
    obj_msg.set_source_port(src_port);
    obj_msg.set_dest_object(dest);
    obj_msg.set_dest_port(dest_port);
    obj_msg.set_unique(GenerateUniqueID(mContext->id));
    obj_msg.set_payload( payload );

    mOutgoingQueue.push( new std::string(serializePBJMessage(obj_msg)) );

    return true;
}

void ObjectHost::migrate(Object* src, ServerID sid) {
    assert(false); // FIXME update, must go to sid
    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IMigrate migrate_msg = session_msg.mutable_migrate();
    migrate_msg.set_object(src->uuid());
    std::string session_serialized = serializePBJMessage(session_msg);
    bool success = this->send(
        src, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg)
    );
    // FIXME do something on failure
}

void ObjectHost::tick(const Time& t) {
    mContext->lastTime = mContext->time;
    mContext->time = t;

    mContext->objectFactory->tick();

    // Service outgoing queue
    assert(false); // FIXME need to service this queue on connections
/*
    uint32 nserviced = 0;
    while( !mOutgoingQueue.empty() && mServer->receiveObjectHostMessage( *mOutgoingQueue.front() ) ) {
        nserviced++;
        delete mOutgoingQueue.front();
        mOutgoingQueue.pop();
    }
*/
    //if (mOutgoingQueue.size() > 1000)
    //    SILOG(oh,warn,"[OH] Warning: outgoing queue size > 1000: " << mOutgoingQueue.size());
}

} // namespace CBR
