/*  Sirikata Network Utilities
 *  ASIOStreamBuilder.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/util/UUID.hpp>
#include "TCPStreamListener.hpp"
#include "TCPStream.hpp"
#include <sirikata/core/network/IOTimer.hpp>

namespace Sirikata {
namespace Network {

class ASIOStreamBuilder : public Service {
public:
    ASIOStreamBuilder(TCPStreamListener::Data* parent);

    void start();
    void stop();

    /**
     * Begins a new stream based on a TCPSocket connection acception
     * with the following substream callback for stream creation Only
     * creates the stream if the handshake is complete and it has all
     * the resources (udp, tcp sockets, etc) necessary at the time
     */
    void beginNewStream(TCPSocket* socket, std::tr1::shared_ptr<TCPStreamListener::Data> data);

private:
    typedef Array<uint8,TCPStream::MaxWebSocketHeaderSize> TcpSstHeaderArray;

    void handleBuildStreamTimeout();

    void buildStream(TcpSstHeaderArray *buffer,
        TCPSocket *socket,
        std::tr1::shared_ptr<TCPStreamListener::Data> data,
        const boost::system::error_code &error,
        std::size_t bytes_transferred);

    class IncompleteStreamState {
    public:
        int mNumSockets;
        std::vector<TCPSocket*> mSockets;
        std::map<TCPSocket*, std::string> mWebSocketResponses;

        // Destroys the sockets associated with this IncompleteStreamState and
        // clears out the data. Only use this for failed connections.
        void destroy();
    };

    typedef std::map<UUID,IncompleteStreamState> IncompleteStreamMap;
    std::deque<UUID> mStaleUUIDs;
    IncompleteStreamMap mIncompleteStreams;

    Network::IOTimerPtr mBuildStreamTimer;
    // Consistent timeouts mean we can just use a queue here.
    typedef std::pair<Time, UUID> BuildStreamTimeout;
    typedef std::queue<BuildStreamTimeout> TimeoutStreamMap;
    TimeoutStreamMap mBuildingStreamTimeouts;
};

} // namespace Network
} // namespace Sirikata
