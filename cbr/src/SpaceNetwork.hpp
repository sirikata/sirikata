/*  Sirikata
 *  SpaceNetwork.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#ifndef _SIRIKATA_SPACE_NETWORK_HPP_
#define _SIRIKATA_SPACE_NETWORK_HPP_

#include <sirikata/cbrcore/Utility.hpp>
#include <sirikata/cbrcore/SpaceContext.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/cbrcore/PollingService.hpp>

namespace Sirikata {

class ServerIDMap;

class SpaceNetwork : public Service {
public:
    typedef Sirikata::Network::Chunk Chunk;

    /** Network::SendStream represents an incoming stream from a single
     *  remote space server.
     */
    class SendStream {
    public:
        virtual ~SendStream() {}

        virtual ServerID id() const = 0;
        virtual bool send(const Chunk&) = 0;
    };

    /** The Network::SendListener interface should be implemented by the object
     *  sending data to the network.  The listener actively pushes data to the
     *  Network queues, but this interface allows it to be notified when it is
     *  going to be able to successfully push more data.
     */
    class SendListener {
      public:
        virtual ~SendListener() {}

        /** Invoked when, after a call to Network::send() fails, the network
         *  determines it can accept more data.
         */
        virtual void networkReadyToSend(const ServerID& from) = 0;
    };

    /** Network::ReceiveStream represents an incoming stream from a single
     *  remote space server.
     */
    class ReceiveStream {
    public:
        virtual ~ReceiveStream() {}

        virtual ServerID id() const = 0;
        virtual Chunk* front() = 0;
        virtual Chunk* pop() = 0;
    };

    /** The Network::ReceiveListener interface should be implemented by the
     *  object receiving data from the network.  The listener must actively pull
     *  data from the Network queues, but the Listener interface allows this
     *  process to be event driven by notifying the object when new data has
     *  been received.
     */
    class ReceiveListener {
      public:
        virtual ~ReceiveListener() {}

        /** Invoked when a new connection is received from a remote host. This
         *  should be used as an indication that a receive queue should be
         *  allocated and serviced for that connection.
         */
        virtual void networkReceivedConnection(ReceiveStream* strm) = 0;

        /** Invoked by the Network when data has been received on a queue that
         * was previously empty, i.e. when data is received taht causes
         * front(from) to change.
         */
        virtual void networkReceivedData(ReceiveStream* strm) = 0;
    };


    virtual ~SpaceNetwork();

    virtual void setSendListener(SendListener* sl) = 0;

    virtual void listen (const ServerID& addr, ReceiveListener* receive_listener)=0;
    virtual SendStream* connect(const ServerID& addr) = 0;

    // ServerIDMap -- used for converting received server ID to a (ip,port) pair.  We have to do
    // this because the remote side might just report 127.0.0.1 + its port.  FIXME We'd like to
    // get rid of this or change this entire interface to just use ServerIDMap and ServerIDs.
    void setServerIDMap(ServerIDMap* sidmap);

protected:
    SpaceNetwork(SpaceContext* ctx);

    // Service Interface
    virtual void start();
    virtual void stop();

    SpaceContext* mContext;
    ServerIDMap* mServerIDMap;
};

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_NETWORK_HPP_
