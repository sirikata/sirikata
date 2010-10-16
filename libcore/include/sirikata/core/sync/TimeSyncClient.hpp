/*  Sirikata
 *  TimeSyncClient.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_CORE_SYNC_TIME_SYNC_CLIENT_HPP_
#define _SIRIKATA_CORE_SYNC_TIME_SYNC_CLIENT_HPP_

#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/odp/Service.hpp>

namespace Sirikata {

/** TimeSyncClient communicates with a server for simple time synchronization.
 *  protocol. You give it a Context to get time information from, an
 *  ODP::Port to handle messaging, and an ODP::Endpoint for the server to
 *  sync with.
 */
class SIRIKATA_EXPORT TimeSyncClient : public PollingService {
public:
    TimeSyncClient(Context* ctx, ODP::Port* odp_port, const ODP::Endpoint& sync_server, const Duration& polling_interval);
    ~TimeSyncClient();
private:

    virtual void poll();

    void handleSyncMessage(const ODP::Endpoint &src, const ODP::Endpoint &dst, MemoryReference payload);

    Context* mContext;
    ODP::Port* mPort;
    ODP::Endpoint mSyncServer;
    uint8 mSeqno;
    Time mRequestTimes[256];

    Duration mOffset;

}; // class TimeSyncServer

} // namespace Sirikata

#endif //_SIRIKATA_CORE_SYNC_TIME_SYNC_CLIENT_HPP_
