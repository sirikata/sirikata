#ifndef _CBR_TCP_NETWORK_HPP_
#define _CBR_TCP_NETWORK_HPP_

#include "Network.hpp"
#include "Address4.hpp"
#include <sirikata/network/Stream.hpp>
#include <sirikata/network/StreamListener.hpp>
#include <sirikata/util/PluginManager.hpp>

namespace CBR {

class TCPNetwork : public Network {
    // Data associated with a stream.  Note that this stream is
    // usually, but not always, unique to the endpoint pair.  Due to
    // the possibility of both sides initiating a connection at the
    // same time we may have more than one of these per remote
    // endpoint, so we maintain a bit more bookkeeping information to
    // manage that possibility.
    struct RemoteStream {
        // parent is used to set the buffer size, stream is the
        // underlying stream
        RemoteStream(TCPNetwork* parent, Sirikata::Network::Stream*strm);

        ~RemoteStream();

        Sirikata::Network::Stream* stream;

        Address4 network_endpoint; // The remote endpoint we're talking to.
        ServerID logical_endpoint; // The remote endpoint we're talking to.
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
        Sirikata::AtomicValue<Chunk*> front; // A single buffered, thread-safe chunk. We
                                   // buffer this since we need to be able to
                                   // pass the front item up to the user,
                                   // meaning it has to be available and
                                   // stored.
        bool paused; // Indicates if receiving is currently paused for
                     // this stream.  If true, the stream must be
                     // unpaused the next time someone calls
                     // receiveOne
    };

    typedef std::tr1::shared_ptr<RemoteStream> RemoteStreamPtr;
    typedef std::tr1::weak_ptr<RemoteStream> RemoteStreamWPtr;
    typedef std::tr1::unordered_map<ServerID, RemoteStreamPtr> RemoteStreamMap;
    typedef std::tr1::unordered_map<Address4, RemoteStreamPtr, Address4::Hasher> RemoteNetStreamMap;

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

    Sirikata::Network::StreamListener *mListener;

    RemoteStreamMap mRemoteStreams;
    RemoteStreamMap mClosingStreams;
    TimerSet mClosingStreamTimers; // Timers for streams that are still closing.

    RemoteStreamMap mFrontStreams; // Maintains pointers to the stream containing
                                   // the current front item for the address.
                                   // This is necessary since the selecetion of
                                   // the closing or active stream may change
                                   // between calls to front() and receiveOne().

    SendListener* mSendListener; // Listener for our send events
    ReceiveListener* mReceiveListener; // Listener for our receive events

    // Main Thread/Strand Methods, allowed to access all the core data structures.  These are mainly utility methods
    // posted by the IO thread.

    // Open a new connection.  Should be called when an existing connection
    // couldn't be found in mRemoteStreams.
    void openConnection(const ServerID& dest);

    // Mark a send stream as disconnected
    void markDisconnected(const RemoteStreamWPtr& wstream);

    // Sends the introduction of this server to the remote server.  Should be
    // used for connections allocated when trying to send.
    void sendServerIntro(const RemoteStreamPtr& out_stream);

    // Handles the identification of a receive stream as a specific endpoint, adding it to the list of full connections
    void addNewStream(RemoteStreamPtr source_stream);

    // Handles timeouts for closing streams -- forcibly closes them, removes
    // them from the closing set.
    void handleClosingStreamTimeout(Sirikata::Network::IOTimerPtr timer, RemoteStreamWPtr& wstream);

    // Get the current queue for receiving data from the address.
    // This considers any closing streams first, then the main stream
    // in order to handle any data arriving on closing streams as
    // quickly as possible.  It takes into account whether any data is
    // available on the stream as well, so a closing stream with no
    // data available will *not* be returned when an active stream
    // with data available also exists.
    RemoteStreamPtr getReceiveQueue(const ServerID& addr);
    // Clears the current front() queue for the address.  Should be called when
    // the front item is removed, allowing the selection of the front queue to
    // change.
    void clearReceiveQueue(const ServerID& from);

    // IO Thread/Strand.  They must be certain not to call any main thread methods or access any main thread data.
    // These are all callbacks from the network, so mostly they should
    // just be posting the results to the main thread.

    RemoteNetStreamMap mPendingStreams; // Streams initiated by the remote
                                        // endpoint that are waiting for the
                                        // initial message specifying the remote
                                        // endpoint ID

    void newStreamCallback(Sirikata::Network::Stream* strm, Sirikata::Network::Stream::SetCallbacks& cb);

    void connectionCallback(const RemoteStreamWPtr& rwstream, const Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason);
    Sirikata::Network::Stream::ReceivedResponse bytesReceivedCallback(const RemoteStreamWPtr& rwstream, Chunk& data);
    void readySendCallback(const RemoteStreamWPtr& rwstream);

    void notifyListenersOfNewStream(const ServerID& remote);
public:
    TCPNetwork(SpaceContext* ctx);
    virtual ~TCPNetwork();

    virtual void setSendListener(SendListener* sl);
    // Checks if this chunk, when passed to send, would be successfully pushed.
    virtual bool canSend(const ServerID&,uint32 size);
    virtual bool send(const ServerID&,const Chunk&);

    virtual void listen(const ServerID& addr, ReceiveListener* receive_listener);
    virtual Chunk* front(const ServerID& from);
    virtual Chunk* receiveOne(const ServerID& from);
};

} // namespace CBR


#endif //_CBR_TCP_NETWORK_HPP_
