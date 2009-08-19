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
#include "ObjectMessageQueue.hpp"

namespace CBR {

ObjectHost::ObjectHost(const ServerID& sid, ObjectFactory* obj_factory, Trace* trace)
 : mContext( new ObjectHostContext() ),
   mOHId((uint64)sid),
   mObjectMessageQueue(NULL)
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

bool ObjectHost::send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
    obj_msg->set_source_object(src->uuid());
    obj_msg->set_source_port(src_port);
    obj_msg->set_dest_object(dest);
    obj_msg->set_dest_port(dest_port);
    obj_msg->set_unique(GenerateUniqueID(mOHId));
    obj_msg->set_payload( payload );

    return mObjectMessageQueue->send(obj_msg);
}

void ObjectHost::tick(const Time& t) {
    mContext->lastTime = mContext->time;
    mContext->time = t;

    mContext->objectFactory->tick();
}

void ObjectHost::setObjectMessageQueue(ObjectMessageQueue* omq) {
    mObjectMessageQueue = omq;
}

} // namespace CBR
