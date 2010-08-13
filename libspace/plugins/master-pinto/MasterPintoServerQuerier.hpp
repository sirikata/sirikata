/*  Sirikata
 *  MasterPintoServerQuerier.hpp
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

#ifndef _SIRIKATA_LIBSPACE_MASTER_PINTO_SERVER_QUERIER_HPP_
#define _SIRIKATA_LIBSPACE_MASTER_PINTO_SERVER_QUERIER_HPP_

#include <sirikata/space/PintoServerQuerier.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/space/SpaceContext.hpp>

#define OPT_MASTER_PINTO_PROTOCOL             "protocol"
#define OPT_MASTER_PINTO_PROTOCOL_OPTIONS     "protocol-options"
#define OPT_MASTER_PINTO_HOST                 "host"
#define OPT_MASTER_PINTO_PORT                 "port"

namespace Sirikata {

/** MasterPintoServerQuerier uses a single, centralized master Pinto query
 *  server to discover which other space servers must be queried.
 */
class MasterPintoServerQuerier : public PintoServerQuerier, public Service {
public:
    MasterPintoServerQuerier(SpaceContext* ctx, const String& params);
    virtual ~MasterPintoServerQuerier();

    // Service Interface
    virtual void start();
    virtual void stop();

    // PintoServerQuerier Interface
    virtual void update(const BoundingBox3f& region, float max_radius);
    virtual void updateQuery(const SolidAngle& min_angle);

private:
    // Handlers for mServerStream
    void handleServerConnection(Network::Stream::ConnectionStatus status, const std::string &reason);
    void handleServerReceived(Network::Chunk& data, const Network::Stream::PauseReceiveCallback& pause);
    void handleServerReadySend();

    SpaceContext* mContext;
    Network::Stream* mServerStream;

    String mHost;
    String mPort;
}; // MasterPintoServerQuerier

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_MASTER_PINTO_SERVER_QUERIER_HPP_
