/*  Sirikata Network Utilities
 *  MultiplexedSocket.hpp
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
#include "util/ThreadId.hpp"
namespace Sirikata { namespace Network {

class MultiplexedSocket:public SelfWeakPtr<MultiplexedSocket>,ThreadIdCheck {
public:
    class RawRequest {
    public:
        bool unordered;
        bool unreliable;
        Stream::StreamID originStream;
        Chunk * data;
    };
    enum SocketConnectionPhase{
        PRECONNECTION,
        WAITCONNECTING,//need to fetch the lock, but about to connect
        CONNECTED,
        DISCONNECTED
    };
private:

//Begin Members//

    ///ASIO io service running in a single thread we can expect callbacks from
    IOService*mIO;
    ///a vector of ASIO sockets (wrapped in with a simple send-full-packet abstraction)
    std::vector<ASIOSocketWrapper> mSockets;
    ///This callback is called whenever a newly encountered StreamID is picked up
    Stream::SubstreamCallback mNewSubstreamCallback;
    typedef std::tr1::unordered_map<Stream::StreamID,TCPStream::Callbacks*,Stream::StreamID::Hasher> CallbackMap;
	///Workaround for VC8 bug that does not define std::pair<Stream::StreamID,Callbacks*>::operator=
    class StreamIDCallbackPair{
    public:
        Stream::StreamID mID;
        TCPStream::Callbacks* mCallback;
        StreamIDCallbackPair(Stream::StreamID id,TCPStream::Callbacks* cb):mID(id) {
            mCallback=cb;
        }
        std::pair<Stream::StreamID,TCPStream::Callbacks*> pair() const{
           return std::pair<Stream::StreamID,TCPStream::Callbacks*>(mID,mCallback);
        }
    };
    /// these next items (mCallbackRegistration, mNewRequests, mSocketConnectionPhase) are synced together take the lock, check for preconnection,,, if connected, don't take lock...otherwise take lock and push data onto the new requests queue
    static boost::mutex sConnectingMutex;
    ///list of packets that must be sent before mSocketConnectionPhase switches to CONNECTION
    std::vector<RawRequest> mNewRequests;
    ///must be set to PRECONNECTION when items are being placed on mNewRequests queue and WAITCONNECTING when it is emptying the queue (with lock held) and finally CONNECTED when the user can send directly to the socket.  DISCONNECTED must be set as soon as the socket fails to write or read
    volatile SocketConnectionPhase mSocketConnectionPhase;
    ///This is a list of items for callback registration so that when packets are received by those streamIDs the appropriate callback may be called
    std::deque<StreamIDCallbackPair> mCallbackRegistration;
    ///a map of ID to callback, only to be touched by the io reactor thread
    CallbackMap mCallbacks;
    ///a map from StreamID to count of number of acked close requests--to avoid any unordered packets coming in
    std::tr1::unordered_map<Stream::StreamID,unsigned int,Stream::StreamID::Hasher>mAckedClosingStreams;
    ///a set of StreamIDs to hold the streams that were requested closed but have not been acknowledged, to prevent received packets triggering NewStream callbacks as if a new ID were received
    std::tr1::unordered_set<Stream::StreamID,Stream::StreamID::Hasher>mOneSidedClosingStreams;
#define ThreadSafeStack ThreadSafeQueue //FIXME this can be way more efficient
    ///The highest streamID that has been used for making new streams on this side
    AtomicValue<uint32> mHighestStreamID;
    ///actually free stream IDs that will not be sent out until recalimed by this side
    ThreadSafeStack<Stream::StreamID>mFreeStreamIDs;
#undef ThreadSafeStack

//Begin helper functions//

    ///Copies items from newcallback to mCallbacks: must be called from the single io thread so no one would be looking to call the callbacks at the same time
    void ioReactorThreadCommitCallback(StreamIDCallbackPair& newcallback);
    ///reads the current list of id-callback pairs to the registration list and if setConectedStatus is set, changes the status of the overall MultiplexedSocket at the same time
    bool CommitCallbacks(std::deque<StreamIDCallbackPair> &registration, SocketConnectionPhase status, bool setConnectedStatus=false);
    ///Returns the least busy stream upon which unordered data may be piled
    size_t leastBusyStream();
    /**
     *chance in the current load that an unreliable packet may be dropped 
     * (due to busy queues, etc). 
     * \returns drop chance which must be less than 1.0 and greater or equal to 0.0 
     */
    float dropChance(const Chunk*data,size_t whichStream);
    /**
     *  sends bytes to the network directly.
     *  assumes that the mSocketConnectionPhase in the CONNECTED state    
     */
    static void sendBytesNow(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data);
    /**
     * Calls the connected callback with the succeess or failure status. Sets status while holding the sConnectingMutex lock so that after that point no more Connected responses
     * will be sent out. Then inserts the registrations into the mCallbacks map during the ioReactor thread.
     */
    void connectionFailureOrSuccessCallback(SocketConnectionPhase status, Stream::ConnectionStatus reportedProblem, const std::string&errorMessage=std::string());
   /**
    * The connection failed before any sockets were established (or as a helper function after they have been cleaned)
    * This function will call all substreams disconnected methods
    */
    void connectionFailedCallback(const std::string& error);
   /**
    * The connection failed before any sockets were established (or as a helper function after they have been cleaned)
    * This function will call all substreams disconnected methods
    */
    void hostDisconnectedCallback(const std::string& error);
public:
    ///public io service accessor for new stream construction
    IOService&getASIOService(){return *mIO;}
    
    /**
     * Sends a packet telling the other side that this stream is closed (or alternatively if its a closeAck that the close request was received and no further packets for that
     * stream will be sent with that streamID
     */
    static void closeStream(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const Stream::StreamID&sid,TCPStream::TCPStreamControlCodes code=TCPStream::TCPStreamCloseStream);

    /**
     * Either sends or queues bytes in the data request depending on the connection state 
     * if the state is not connected then it must take a lock and place them on the mNewRequests queue
     */
    static void sendBytes(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data);
    /**
     * Adds callbacks onto the queue of callbacks-to-be-added
     * Returns true if the callbacks will be actually used or false if the socket is already disconnected
     */
    SocketConnectionPhase addCallbacks(const Stream::StreamID&sid, TCPStream::Callbacks* cb);
    ///function that searches mFreeStreamIDs or uses the mHighestStreamID to find the next unused free stream ID
    Stream::StreamID getNewID();
    ///Constructor for a connecting stream
    MultiplexedSocket(IOService*io, const Stream::SubstreamCallback&substreamCallback);
    ///Constructor for a listening stream with a prebuilt connection of ASIO sockets
    MultiplexedSocket(IOService*io, const UUID&uuid,const std::vector<TCPSocket*>&sockets, const Stream::SubstreamCallback &substreamCallback);
    ///Sends the protocol headers to all ASIO socket wrappers when a known fully open connection has been listened for
    static void sendAllProtocolHeaders(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const UUID&syncedUUID);
    ///erase all sockets and callbacks since the refcount is now zero;
    ~MultiplexedSocket();
    ///a stream that has been closed and the other side has agreed not to send any more packets using that ID
    void shutDownClosedStream(unsigned int controlCode,const Stream::StreamID &id);
    /**
     * Process an entire packet when received from the IO reactor thread.
     * Control packets come in on Stream::StreamID() and others should be directed
     * to the appropriate callback
     */
    void receiveFullChunk(unsigned int whichSocket, Stream::StreamID id,const Chunk&newChunk);
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    void connectionFailedCallback(unsigned int whichSocket, const std::string& error);
   /**
    * The connection failed to connect before any sockets had been established (ex: host not found)
    * This function will call all substreams disconnected methods
    */
    template <class ErrorCode> void connectionFailedCallback(const ErrorCode& error) {
        connectionFailedCallback(error.message());
    }
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    template <class ErrorCode> void connectionFailedCallback(unsigned int whichSocket, const ErrorCode& error) {
        connectionFailedCallback(whichSocket,error.message());
    }
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    template <class ErrorCode> void connectionFailedCallback(const ASIOSocketWrapper* whichSocket, const ErrorCode &error) {
        unsigned int which=0;
        for (std::vector<ASIOSocketWrapper>::iterator i=mSockets.begin(),ie=mSockets.end();i!=ie;++i,++which) {
            if (&*i==whichSocket)
                break;
        }
        connectionFailedCallback(which==mSockets.size()?0:which,error.message());
    }

   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    void hostDisconnectedCallback(unsigned int whichSocket, const std::string& error);
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    template <class ErrorCode> void hostDisconnectedCallback(unsigned int whichSocket, const ErrorCode& error) {
        hostDisconnectedCallback(whichSocket,error.message());
    }
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    template <class ErrorCode> void hostDisconnectedCallback(const ASIOSocketWrapper* whichSocket, const ErrorCode &error) {
        unsigned int which=0;
        for (std::vector<ASIOSocketWrapper>::iterator i=mSockets.begin(),ie=mSockets.end();i!=ie;++i,++which) {
            if (&*i==whichSocket)
                break;
        }
        hostDisconnectedCallback(which==mSockets.size()?0:which,error.message());
    }


   /**
    * The a particular established a connection:
    * This function will call all substreams connected methods
    */
    void connectedCallback() {
        connectionFailureOrSuccessCallback(CONNECTED,Stream::Connected);
    }
/**
 *  Connect a newly constructed MultiplexedSocket to a given address
 * \param address is a protocol-agnostic string of endpoint and service ID
 * \param numSockets indicates how many TCP sockets should manage the orderlessness of this connection
 */
    void connect(const Address&address, unsigned int numSockets);

/**
 *  Prepare a socket for an outbound connection. 
 *  After this call messages may be queued and number of redundant connections set
 *  Additionally this socket may now be cloned
 */
    void prepareConnect(unsigned int numSockets);

    unsigned int numSockets() const {
        return mSockets.size();
    }
    ASIOSocketWrapper&getASIOSocketWrapper(unsigned int whichSocket){
        return mSockets[whichSocket];
    }
    const ASIOSocketWrapper&getASIOSocketWrapper(unsigned int whichSocket)const{
        return mSockets[whichSocket];
    }
};
} }
