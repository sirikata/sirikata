/*  Sirikata
 *  StreamEcho.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOStrand.hpp>

#include "Protocol_Empty.pbj.hpp"
#include <sirikata/core/network/PBJDebug.hpp>

void handleConnectionEvent(Sirikata::Network::Stream* strm, Sirikata::Network::Stream::ConnectionStatus, const std::string&reason);
void handleSubstream(Sirikata::Network::Stream* substream, Sirikata::Network::Stream::SetCallbacks&);
void handlePBJSubstream(Sirikata::Network::Stream* substream, Sirikata::Network::Stream::SetCallbacks&);
void handleReceived(Sirikata::Network::Stream* strm, Sirikata::Network::Chunk& payload, const Sirikata::Network::Stream::PauseReceiveCallback& pausecb);
void handlePBJReceived(Sirikata::Network::Stream* strm, Sirikata::Network::Chunk& payload, const Sirikata::Network::Stream::PauseReceiveCallback& pausecb);
void handleReadySend(Sirikata::Network::Stream* strm);

void handleNewConnection(Sirikata::Network::Stream* strm, Sirikata::Network::Stream::SetCallbacks& sc) {
    printf("Received connection\n");
    // Do same for main connection stream and substreams
    handleSubstream(strm, sc);
}

void handleNewPBJConnection(Sirikata::Network::Stream* strm, Sirikata::Network::Stream::SetCallbacks& sc) {
    printf("Received PBJ connection\n");
    // Do same for main connection stream and substreams
    handlePBJSubstream(strm, sc);
}

void handleConnectionEvent(Sirikata::Network::Stream* strm, Sirikata::Network::Stream::ConnectionStatus status, const std::string&reason) {
    if (status == Sirikata::Network::Stream::Disconnected)
        printf("Stream disconnected\n");
}

void handleSubstream(Sirikata::Network::Stream* substream, Sirikata::Network::Stream::SetCallbacks& sc) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    printf("New substream\n");
    sc(
        std::tr1::bind(handleConnectionEvent, substream, _1, _2),
        std::tr1::bind(handleReceived, substream, _1, _2),
        std::tr1::bind(handleReadySend, substream)
    );
}

void handlePBJSubstream(Sirikata::Network::Stream* substream, Sirikata::Network::Stream::SetCallbacks& sc) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    printf("New PBJ substream\n");
    sc(
        std::tr1::bind(handleConnectionEvent, substream, _1, _2),
        std::tr1::bind(handlePBJReceived, substream, _1, _2),
        std::tr1::bind(handleReadySend, substream)
    );
}

void handleReceived(Sirikata::Network::Stream* strm, Sirikata::Network::Chunk& payload, const Sirikata::Network::Stream::PauseReceiveCallback& pausecb) {
    printf("Received %d bytes\n", (int)payload.size());
    strm->send(payload, Sirikata::Network::ReliableOrdered);
}

void handlePBJReceived(Sirikata::Network::Stream* strm, Sirikata::Network::Chunk& payload, const Sirikata::Network::Stream::PauseReceiveCallback& pausecb) {
    printf("Received %d PBJ bytes: ", (int)payload.size());
    bool parsed = Sirikata::printPBJMessageArray(payload);

    if (!parsed) {
        // Indicate error to other side
        const char* error_msg = "Error"; // "Error" in base 64
        strm->send(Sirikata::MemoryReference(error_msg, strlen(error_msg)), Sirikata::Network::ReliableOrdered);
        return;
    }

    Sirikata::Protocol::Empty msg;
    msg.ParseFromArray(&payload[0], payload.size());

    // Finally, reply. Reserialize to test encoding consistency.
    std::string return_payload;
    bool serialized_success = msg.SerializeToString(&return_payload);
    strm->send(Sirikata::MemoryReference(return_payload.data(), return_payload.size()), Sirikata::Network::ReliableOrdered);
}

void handleReadySend(Sirikata::Network::Stream* strm) {
}

int main(int argc, char** argv) {
    using namespace Sirikata;
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    InitOptions();
    ParseOptions(argc, argv);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* iostrand = ios->createStrand();

    String stream_type = GetOptionValue<String>("ohstreamlib");
    String stream_opts = GetOptionValue<String>("ohstreamoptions");

    Sirikata::Network::StreamListener* listener =
        Sirikata::Network::StreamListenerFactory::getSingleton()
        .getConstructor(stream_type)
        (iostrand,
            Sirikata::Network::StreamListenerFactory::getSingleton()
            .getOptionParser(stream_type)
            (stream_opts));
    Sirikata::Network::Address addr("localhost", "9999");
    listener->listen(
        addr,
        std::tr1::bind(handleNewConnection,_1,_2)
    );


    Sirikata::Network::StreamListener* pbj_listener =
        Sirikata::Network::StreamListenerFactory::getSingleton()
        .getConstructor(stream_type)
        (iostrand,
            Sirikata::Network::StreamListenerFactory::getSingleton()
            .getOptionParser(stream_type)
            (stream_opts));
    Sirikata::Network::Address pbj_addr("localhost", "9998");
    pbj_listener->listen(
        pbj_addr,
        std::tr1::bind(handleNewPBJConnection,_1,_2)
    );

    ios->run();

    delete iostrand;
    Network::IOServiceFactory::destroyIOService(ios);

    return 0;
}
