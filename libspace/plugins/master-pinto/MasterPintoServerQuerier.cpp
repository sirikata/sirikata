/*  Sirikata
 *  MasterPintoServerQuerier.cpp
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

#include "MasterPintoServerQuerier.hpp"
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/options/Options.hpp>

using namespace Sirikata::Network;

#define MP_LOG(lvl,msg) SILOG(masterpinto,lvl,"[MASTERPINTO] " << msg);

namespace Sirikata {

MasterPintoServerQuerier::MasterPintoServerQuerier(SpaceContext* ctx, const String& params)
 : mContext(ctx)
{
    OptionSet* optionsSet = OptionSet::getOptions("space_master_pinto",NULL);
    optionsSet->parse(params);

    String server_protocol = optionsSet->referenceOption(OPT_MASTER_PINTO_PROTOCOL)->as<String>();
    String server_protocol_options = optionsSet->referenceOption(OPT_MASTER_PINTO_PROTOCOL_OPTIONS)->as<String>();

    OptionSet* server_protocol_optionset = StreamFactory::getSingleton().getOptionParser(server_protocol)(server_protocol_options);

    mServerStream = StreamFactory::getSingleton().getConstructor(server_protocol)(mContext->ioService, server_protocol_optionset);

    mHost = optionsSet->referenceOption(OPT_MASTER_PINTO_HOST)->as<String>();
    mPort = optionsSet->referenceOption(OPT_MASTER_PINTO_PORT)->as<String>();

    // Register us as a service for startup/shutdown
    mContext->add(this);
}

MasterPintoServerQuerier::~MasterPintoServerQuerier() {
}

void MasterPintoServerQuerier::start() {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    Address serverAddress(mHost, mPort);
    MP_LOG(debug, "Trying to connect to " << serverAddress.toString());
    mServerStream->connect(
        serverAddress,
        &Network::Stream::ignoreSubstreamCallback,
        std::tr1::bind(&MasterPintoServerQuerier::handleServerConnection, this, _1, _2),
        std::tr1::bind(&MasterPintoServerQuerier::handleServerReceived, this, _1, _2),
        std::tr1::bind(&MasterPintoServerQuerier::handleServerReadySend, this)
    );
}

void MasterPintoServerQuerier::stop() {
    mServerStream->close();
}

void MasterPintoServerQuerier::update(const BoundingBox3f& region, float max_radius) {
}

void MasterPintoServerQuerier::updateQuery(const SolidAngle& min_angle) {

}

void MasterPintoServerQuerier::handleServerConnection(Network::Stream::ConnectionStatus status, const std::string &reason) {
    if (status == Network::Stream::Connected) {
        MP_LOG(debug, "Connected to master pinto server.");
    }
    else if (status == Network::Stream::ConnectionFailed) {
        MP_LOG(debug, "Connection to master pinto server failed.");
    }
    else if (status == Network::Stream::Disconnected) {
        MP_LOG(debug, "Disconnected from pinto server.");
    }
}

void MasterPintoServerQuerier::handleServerReceived(Chunk& data, const Network::Stream::PauseReceiveCallback& pause) {
}

void MasterPintoServerQuerier::handleServerReadySend() {
}

} // namespace Sirikata
