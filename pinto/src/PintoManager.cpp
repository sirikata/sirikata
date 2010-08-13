/*  Sirikata
 *  PintoManager.cpp
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

#include "PintoManager.hpp"
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

using namespace Sirikata::Network;

#define PINTO_LOG(lvl, msg) SILOG(pinto,lvl,"[PINTO] " << msg)

namespace Sirikata {

PintoManager::PintoManager(PintoContext* ctx)
 : mContext(ctx)
{
    String listener_protocol = GetOptionValue<String>(OPT_PINTO_PROTOCOL);
    String listener_protocol_options = GetOptionValue<String>(OPT_PINTO_PROTOCOL_OPTIONS);

    OptionSet* listener_protocol_optionset = StreamListenerFactory::getSingleton().getOptionParser(listener_protocol)(listener_protocol_options);

    mListener = StreamListenerFactory::getSingleton().getConstructor(listener_protocol)(mContext->ioService, listener_protocol_optionset);

    String listener_host = GetOptionValue<String>(OPT_PINTO_HOST);
    String listener_port = GetOptionValue<String>(OPT_PINTO_PORT);
    Address listenAddress(listener_host, listener_port);
    PINTO_LOG(debug, "Listening on " << listenAddress.toString());
    mListener->listen(
        listenAddress,
        std::tr1::bind(&PintoManager::newStreamCallback,this,_1,_2)
    );
}

PintoManager::~PintoManager() {
    delete mListener;
}

void PintoManager::start() {
}

void PintoManager::stop() {
    mListener->close();
}

void PintoManager::newStreamCallback(Stream* newStream, Stream::SetCallbacks& setCallbacks) {
    PINTO_LOG(debug,"New space server connection.");
}

} // namespace Sirikata
