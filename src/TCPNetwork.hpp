#ifndef _CBR_TCP_NETWORK_HPP_
#define _CBR_TCP_NETWORK_HPP_

#include "Network.hpp"
#include <sirikata/network/Stream.hpp>
#include <sirikata/network/StreamListener.hpp>
#include <sirikata/util/PluginManager.hpp>
#include <sirikata/util/SizedThreadSafeQueue.hpp>

namespace CBR {

class TCPNetwork : public Network {
    // Data associated with a stream.  Note that this stream is
    // usually, but not always, unique to the endpoint pair.  Due to
    // the possibility of both sides initiating a connection at the
    // same time we may have more than one of these per remote
    // endpoint, so we maintain a bit more bookkeeping information to
    // manage that possibility.
    struct RemoteStream {
        typedef Sirikata::SizedThreadSafeQueue<Chunk*> SizedChunkBuffer;

        // parent is used to set the buffer size, stream is the
        // underlying stream
        RemoteStream(TCPNetwork* parent, Sirikata::Network::Stream*strm);

        ~RemoteStream();

        Sirikata::Network::Stream* stream;

        Address4 endpoint; // The remote endpoint we're talking to.
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

        Chunk *front; // A single buffered chunk. We buffer this since
                      // we need to be able to pass the front item up
                      // to the user, meaning it has to be available
                      // and stored.
        SizedChunkBuffer buffer; // Sized buffer, the main queue for
                                 // storing incoming packets.
        bool paused; // Indicates if receiving is currently paused for
                     // this stream.  If true, the stream must be
                     // unpaused the next time someone calls
                     // receiveOne
    };

    typedef std::tr1::shared_ptr<RemoteStream> RemoteStreamPtr;
    typedef std::tr1::weak_ptr<RemoteStream> RemoteStreamWPtr;
    typedef std::tr1::unordered_map<Address4, RemoteStreamPtr, Address4::Hasher> RemoteStreamMap;

    typedef std::set<Sirikata::Network::IOTimerPtr> TimerSet;

    // All the real data is handled in the main thread.
    Sirikata::PluginManager mPluginManager;
    String mStreamPlugin;
    Sirikata::OptionSet* mListenOptions;
    Sirikata::OptionSet* mSendOptions;

    IOService *mIOService;
    IOWork* mIOWork;
    Thread* mThread;

    Address4 mListenAddress;

    uint32 mIncomingBufferLength;
    uint32 mIncomingBandwidth;
    uint32 mOutgoingBandwidth;

    Sirikata::Network::StreamListener *mListener;

    RemoteStreamMap mRemoteStreams;
    RemoteStreamMap mClosingStreams;
    TimerSet mClosingStreamTimers; // Timers for streams that are still closing.

    RemoteStreamMap mFrontStreams; // Maintains pointers to the stream containing
                                   // the current front item for the address.
                                   // This is necessary since the selecetion of
                                   // the closing or active stream may change
                                   // between calls to front() and receiveOne().

    ReceiveListener* mReceiveListener; // Listener for our receive events

    // Main Thread/Strand Methods, allowed to access all the core data structures.  These are mainly utility methods
    // posted by the IO thread.

    // Mark a send stream as disconnected
    void markDisconnected(const Address4& addr);

    // Sends the introduction of this server to the remote server.  Should be
    // used for connections allocated when trying to send.
    void sendServerIntro(const RemoteStreamPtr& out_stream);

    // Handles the identification of a receive stream as a specific endpoint, adding it to the list of full connections
    void addNewStream(const Address4& remote_endpoint, RemoteStreamPtr source_stream);

    // Handles timeouts for closing streams -- forcibly closes them, removes
    // them from the closing set.
    void handleClosingStreamTimeout(Sirikata::Network::IOTimerPtr timer, const Address4& remote_endpoint);

    // Get the current queue for receiving data from the address.
    // This considers any closing streams first, then the main stream
    // in order to handle any data arriving on closing streams as
    // quickly as possible.  It takes into account whether any data is
    // available on the stream as well, so a closing stream with no
    // data available will *not* be returned when an active stream
    // with data available also exists.
    RemoteStreamPtr getReceiveQueue(const Address4& addr);
    // Clears the current front() queue for the address.  Should be called when
    // the front item is removed, allowing the selection of the front queue to
    // change.
    void clearReceiveQueue(const Address4& addr);

    // IO Thread/Strand.  They must be certain not to call any main thread methods or access any main thread data.
    // These are all callbacks from the network, so mostly they should
    // just be posting the results to the main thread.

    RemoteStreamMap mPendingStreams; // Streams initiated by the
                                     // remote endpoint that are
                                     // waiting for the initial
                                     // message specifying the remote
                                     // endpoint ID

    void newStreamCallback(Sirikata::Network::Stream* strm, Sirikata::Network::Stream::SetCallbacks& cb);

    void connectionCallback(const Address4& remote_addr, const RemoteStreamWPtr& rwstream, const Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason);
    Sirikata::Network::Stream::ReceivedResponse bytesReceivedCallback(const RemoteStreamWPtr& rwstream, Chunk& data);
    void readySendCallback(const Address4& addr);

public:
    TCPNetwork(SpaceContext* ctx, uint32 incomingBufferLength, uint32 icomingBandwidth,uint32 outgoingBandwidth);
    virtual ~TCPNetwork();

    // Checks if this chunk, when passed to send, would be successfully pushed.
    virtual bool canSend(const Address4&,uint32 size);
    virtual bool send(const Address4&,const Chunk&);

    virtual void listen(const Address4& addr, ReceiveListener* receive_listener);
    virtual Chunk* front(const Address4& from, uint32 max_size);
    virtual Chunk* receiveOne(const Address4& from, uint32 max_size);
};

} // namespace CBR


#endif //_CBR_TCP_NETWORK_HPP_
