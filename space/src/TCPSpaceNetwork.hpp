/*  Sirikata
 *  TCPSpaceNetwork.hpp.hpp
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

#ifndef _SIRIKATA_TCP_SPACE_NETWORK_HPP_
#define _SIRIKATA_TCP_SPACE_NETWORK_HPP_

#include "SpaceNetwork.hpp"
#include <sirikata/core/network/Address4.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/queue/SizedThreadSafeQueue.hpp>
#include <sirikata/core/queue/CountResourceMonitor.hpp>

namespace Sirikata {

class TCPSpaceNetwork : public SpaceNetwork {
    // Data associated with a stream.  Note that this stream is
    // usually, but not always, unique to the endpoint pair.  Due to
    // the possibility of both sides initiating a connection at the
    // same time we may have more than one of these per remote
    // endpoint, so we maintain a bit more bookkeeping information to
    // manage that possibility.
    struct RemoteStream {
        enum Initiator {
            Us, Them
        };

        // parent is used to set the buffer size, stream is the
        // underlying stream
        RemoteStream(TCPSpaceNetwork* parent, Sirikata::Network::Stream*strm, ServerID remote_id, Address4 remote_net, Initiator init);

        ~RemoteStream();

        bool push(Chunk& data, bool* was_empty);
        Chunk* pop(Network::IOStrand* ios);

        Sirikata::Network::Stream* stream;

        Address4 network_endpoint; // The remote endpoint we're talking to.
        ServerID logical_endpoint; // The remote endpoint we're
                                   // talking to.

        Initiator initiator;

        bool connected; // Indicates if the initial header specifying
                        // the remote endpoint has been sent or
                        // received (depending on which side initiated
                        // the connection). If this is true then this
                        // connection should be in the map of
                        // connections and it should be safe to send
                        // data on it.
        bool shutting_down; // Inidicates if this connection is
                            // currently being shutdown. Will be true
                            // if another stream to the same endpoint
                            // was preferred over this one.
        typedef Sirikata::SizedThreadSafeQueue<Chunk*,CountResourceMonitor> SizedChunkReceiveQueue;
        SizedChunkReceiveQueue receive_queue; // Note: This can't be a single
                                              // front item or the receive queue
                                              // empties it too quickly and
                                              // loses bandwidth.
        bool paused; // Indicates if receiving is currently paused for
                     // this stream.  If true, the stream must be
                     // unpaused the next time someone calls
                     // receiveOne

        boost::mutex mPushPopMutex;
    };

    typedef std::tr1::shared_ptr<RemoteStream> RemoteStreamPtr;
    typedef std::tr1::weak_ptr<RemoteStream> RemoteStreamWPtr;
    typedef std::tr1::unordered_map<ServerID, RemoteStreamPtr> RemoteStreamMap;
    typedef std::tr1::unordered_map<Address4, RemoteStreamPtr, Address4::Hasher> RemoteNetStreamMap;


    /** RemoteSession is the data structure that ensures the appearance of
     *  a continuous connection to a remote server.  The send and receive
     *  streams will simply use one of these internally to perform their
     *  operations.
     *
     *  Note that these are created in a thread-safe manner (only by
     *  getRemoteData()), and should *only be manipulated by
     *  handleConnectedStream, handleDisconnectedStream, and
     *  handleClosingStreamTimeout*, which will guarantee correct order of
     *  operations for thread safety (and which will not conflict with each
     *  other since they all work in the network thread).
     */
    struct RemoteSession {
        RemoteSession(ServerID sid);
        ~RemoteSession();

        // Endpoint for this session, guaranteed not to change.
        ServerID logical_endpoint;

        // Streams for this session.
        RemoteStreamPtr remote_stream; // The stream we're normally trying to
                                       // receive from
        RemoteStreamPtr closing_stream; // A stream we're trying to shut down
        RemoteStreamPtr pending_out; // Remote stream that we've opened and
                                        // called connect on, but which isn't
                                        // ready yet
    };
    typedef std::tr1::shared_ptr<RemoteSession> RemoteSessionPtr;
    typedef std::tr1::weak_ptr<RemoteSession> RemoteSessionWPtr;

    class TCPSendStream : public SpaceNetwork::SendStream {
    public:
        TCPSendStream(ServerID sid, RemoteSessionPtr s);
        ~TCPSendStream();

        virtual ServerID id() const;
        virtual bool send(const Chunk&);

    private:
        ServerID logical_endpoint;
        RemoteSessionPtr session;
    };
    typedef std::tr1::unordered_map<ServerID, TCPSendStream*> SendStreamMap;

    class TCPReceiveStream : public SpaceNetwork::ReceiveStream {
    public:
        TCPReceiveStream(ServerID sid, RemoteSessionPtr s, Network::IOStrand* _ios);
        ~TCPReceiveStream();
        virtual ServerID id() const;
        virtual Chunk* front();
        virtual Chunk* pop();

    private:
        // Get the current queue for receiving data from the address.
        // This considers any closing streams first, then the main stream
        // in order to handle any data arriving on closing streams as
        // quickly as possible.  It takes into account whether any data is
        // available on the stream as well, so a closing stream with no
        // data available will *not* be returned when an active stream
        // with data available also exists.
        // NOTE: Now this just ensures that front_stream will either
        // point to the right thing or to NULL, so there is no return value.
        void getCurrentRemoteStream();

        bool canReadFrom(RemoteStreamPtr& strm);

        ServerID logical_endpoint;
        RemoteSessionPtr session;
        RemoteStreamPtr front_stream; // Stream which we already got a front()
                                      // item from
        Chunk* front_elem; // The front item, left out here to make it
                           // accessible since the RemoteStream doesn't give
                           // easy access
        Network::IOStrand* ios;
    };
    typedef std::tr1::unordered_map<ServerID, TCPReceiveStream*> ReceiveStreamMap;

    // Simple storage for the session, send stream, and receive stream
    // associated with a single endpoint.  There should only ever be *one* of
    // these per remote endpoint.
    struct RemoteData {
        RemoteData(RemoteSessionPtr s)
         :session(s),
          send(NULL),
          receive(NULL)
        {}

        ~RemoteData() {
            delete send;
            delete receive;
            session.reset();
        }

        RemoteSessionPtr session;
        TCPSendStream* send;
        TCPReceiveStream* receive;
    };

    // This is the one chunk of truly thread-safe data, followed by a small
    // number of truly thread-safe methods to manipulate it.  These control the
    // central repository of RemoteSessionPtrs, ReceiveStreams, and SendStreams,
    // each unique per ServerID. This map is really only used to retrieve or
    // initialize each, after which all operations are directly on those objects
    // instead of through the map.  It is also locked on new connections to
    // make lookups so tradeoffs between underlying connections are possible.
    typedef std::tr1::unordered_map<ServerID, RemoteData*> RemoteDataMap;
    RemoteDataMap mRemoteData;
    boost::recursive_mutex mRemoteDataMutex;
    RemoteData* getRemoteData(ServerID sid);
    RemoteSessionPtr getRemoteSession(ServerID sid);
    TCPSendStream* getNewSendStream(ServerID sid);
    TCPReceiveStream* getNewReceiveStream(ServerID sid);
    // Get a new uninitialized outgoing stream that connect can be called on.
    // Will return NULL if another outgoing stream has already been requested,
    // guaranteeing that multiple outgoing stream connections won't be
    // attempted.
    RemoteStreamPtr getNewOutgoingStream(ServerID sid, Address4 remote_net, RemoteStream::Initiator init);
    RemoteStreamPtr getNewIncomingStream(Address4 remote_net, RemoteStream::Initiator init, Sirikata::Network::Stream* strm);
    typedef std::set<Sirikata::Network::IOTimerPtr> TimerSet;

    // All the real data is handled in the main thread.
    Sirikata::PluginManager mPluginManager;
    String mStreamPlugin;
    Sirikata::OptionSet* mListenOptions;
    Sirikata::OptionSet* mSendOptions;

    Network::IOStrand *mIOStrand;
    Network::IOWork* mIOWork;

    RemoteStreamMap mClosingStreams;
    TimerSet mClosingStreamTimers; // Timers for streams that are still closing.

    // Open a new connection.  Should be called when an existing connection
    // isn't available.
    TCPSendStream* openConnection(const ServerID& dest);

    // Add stream to system, possibly resolving conflicting sets of
    // streams. Return corresponding TCPReceiveStream*
    TCPReceiveStream* handleConnectedStream(RemoteStreamPtr source_stream);
    // Mark a send stream as disconnected
    void handleDisconnectedStream(const RemoteStreamPtr& wstream);



    Address4 mListenAddress;

    Sirikata::Network::StreamListener *mListener;

    SendListener* mSendListener; // Listener for our send events
    ReceiveListener* mReceiveListener; // Listener for our receive events

    // Main Thread/Strand Methods, allowed to access all the core data structures.  These are mainly utility methods
    // posted by the IO thread.

    // Sends the introduction of this server to the remote server.  Should be
    // used for connections allocated when trying to send.
    void sendServerIntro(const RemoteStreamPtr& out_stream);

    // Handles timeouts for closing streams -- forcibly closes them, removes
    // them from the closing set.
    void handleClosingStreamTimeout(Sirikata::Network::IOTimerPtr timer, RemoteStreamPtr& wstream);

    // IO Thread/Strand.  They must be certain not to call any main thread methods or access any main thread data.
    // These are all callbacks from the network, so mostly they should
    // just be posting the results to the main thread.

    RemoteNetStreamMap mPendingStreams; // Streams initiated by the remote
                                        // endpoint that are waiting for the
                                        // initial message specifying the remote
                                        // endpoint ID

    void newStreamCallback(Sirikata::Network::Stream* strm, Sirikata::Network::Stream::SetCallbacks& cb);

    void connectionCallback(RemoteStreamWPtr wstream, const Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason);
    // NOTE: The ind_recv_strm is a bit of an odd construction.  Essentially, we
    // are forced to register callbacks immediately upon connection, but at that
    // time we don't have all the information we will need.  However, we also
    // don't want to have to take a lock and look it up on every callback.
    // Therefore, we pass through a location for the recv_strm to be stored.
    // Further, we wrap it in a shared_ptr so it gets cleaned up when no longer
    // needed (the storage for the pointer, not the TCPReceiveStream itself,
    // which is cleaned up separately.)
    typedef std::tr1::shared_ptr<TCPReceiveStream*> IndirectTCPReceiveStream;
    void bytesReceivedCallback(RemoteStreamWPtr wstream, IndirectTCPReceiveStream ind_recv_strm, Chunk& data, const Sirikata::Network::Stream::PauseReceiveCallback& pause);
    void readySendCallback(RemoteStreamWPtr wstream);

public:
    TCPSpaceNetwork(SpaceContext* ctx);
    virtual ~TCPSpaceNetwork();

    virtual void setSendListener(SendListener* sl);

    virtual void listen(const ServerID& addr, ReceiveListener* receive_listener);
    virtual SendStream* connect(const ServerID& addr);
};

} // namespace Sirikata


#endif //_SIRIKATA_TCP_SPACE_NETWORK_HPP_
