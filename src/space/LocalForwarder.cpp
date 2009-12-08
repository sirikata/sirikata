/*  cbr
 *  LocalForwarder.cpp
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

#include "LocalForwarder.hpp"
#include "Statistics.hpp"

namespace CBR {


LocalForwarder::LocalForwarder(SpaceContext* ctx, IOStrand* net_strand)
        : mContext(ctx),
          mNetStrand(net_strand)
{
}

void LocalForwarder::addActiveConnection(ObjectConnection* conn) {
    mNetStrand->post(
        std::tr1::bind(&LocalForwarder::handleAddActiveConnection, this, conn)
                     );
}

void LocalForwarder::handleAddActiveConnection(ObjectConnection* conn) {
    assert(mActiveConnections.find(conn->id()) == mActiveConnections.end());

    mActiveConnections[conn->id()] = conn;
}

void LocalForwarder::removeActiveConnection(const UUID& objid) {
    mNetStrand->post(
        std::tr1::bind(&LocalForwarder::handleRemoveActiveConnection, this, objid)
                     );
}

void LocalForwarder::handleRemoveActiveConnection(const UUID& objid) {
    ObjectConnectionMap::iterator it = mActiveConnections.find(objid);
    if (it == mActiveConnections.end())
        return;

    ObjectConnection* conn = it->second;
    mActiveConnections.erase(it);
}

bool LocalForwarder::tryForward(CBR::Protocol::Object::ObjectMessage* msg) {
    ObjectConnectionMap::iterator it = mActiveConnections.find(msg->dest_object());

    if (it == mActiveConnections.end())
        return false;

    ObjectConnection* conn = it->second;

    if (!conn->enabled())
        return false;

    TIMESTAMP_START(tstamp, msg);

    Trace::MessagePath msg_path = Trace::FORWARDED_LOCALLY;

    bool send_success = conn->send(msg);
    if (!send_success) {
        // FIXME do anything on failure?
        delete msg;
        msg_path = Trace::DROPPED;
    }

    TIMESTAMP_END(tstamp, msg_path);

    // At this point we've handled it, regardless of send's success
    return true;
}

} // namespace CBR
