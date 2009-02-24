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

namespace Sirikata { namespace Network {

class MultiplexedSocket:public SelfWeakPtr<MultiplexedSocket> {
public:
    class RawRequest {
    public:
        bool unordered;
        bool unreliable;
        Stream::StreamID originStream;
        Chunk * data;
    };
private:
    IOService*mIO;
    std::vector<ASIOSocketWrapper> mSockets;
/// these items are synced together take the lock, check for preconnection,,, if connected, don't take lock...otherwise take lock and push data onto the new requests queue
    static boost::mutex sConnectingMutex;
    std::vector<RawRequest> mNewRequests;
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
    std::deque<StreamIDCallbackPair> mCallbackRegistration;
    volatile enum SocketConnectionPhase{
        PRECONNECTION,
        WAITCONNECTING,//need to fetch the lock, but about to connect
        CONNECTED,
        DISCONNECTED
    }mSocketConnectionPhase;
    ///Copies items from CallbackRegistration to mCallbacks Assumes sConnectingMutex is taken
    void ioReactorThreadCommitCallback(StreamIDCallbackPair& newcallback);
    bool CommitCallbacks(std::deque<StreamIDCallbackPair> &registration, SocketConnectionPhase status, bool setConnectedStatus=false);
    size_t leastBusyStream();
    float dropChance(const Chunk*data,size_t whichStream);
    static void sendBytesNow(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data);
public:

    IOService&getASIOService(){return *mIO;}
    static void closeStream(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const Stream::StreamID&sid,TCPStream::TCPStreamControlCodes code=TCPStream::TCPStreamCloseStream) {
        RawRequest closeRequest;
        closeRequest.originStream=Stream::StreamID();//control packet
        closeRequest.unordered=false;
        closeRequest.unreliable=false;
        closeRequest.data=ASIOSocketWrapper::constructControlPacket(code,sid);
        sendBytes(thus,closeRequest);
    }
    static void sendBytes(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data);
    /**
     * Adds callbacks onto the queue of callbacks-to-be-added
     */
    void addCallbacks(const Stream::StreamID&sid, TCPStream::Callbacks* cb) {
        boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
        mCallbackRegistration.push_back(StreamIDCallbackPair(sid,cb));
    }
    AtomicValue<uint32> mHighestStreamID;
    ///a map from StreamID to count of number of acked close requests--to avoid any unordered packets coming in
    std::tr1::unordered_map<Stream::StreamID,unsigned int,Stream::StreamID::Hasher>mAckedClosingStreams;
    std::tr1::unordered_set<Stream::StreamID,Stream::StreamID::Hasher>mOneSidedClosingStreams;
#define ThreadSafeStack ThreadSafeQueue //FIXME this can be way more efficient
    ///actually free stream IDs
    ThreadSafeStack<Stream::StreamID>mFreeStreamIDs;
#undef ThreadSafeStack
    CallbackMap mCallbacks;

    Stream::StreamID getNewID();
    MultiplexedSocket(IOService*io, const Stream::SubstreamCallback&substreamCallback);
    MultiplexedSocket(const UUID&uuid,const std::vector<TCPSocket*>&sockets, const Stream::SubstreamCallback &substreamCallback);
    static void sendAllProtocolHeaders(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const UUID&syncedUUID);
    ///erase all sockets and callbacks since the refcount is now zero;
    ~MultiplexedSocket();
    ///a stream that has been closed and the other side has agreed not to send any more packets using that ID
    void shutDownClosedStream(unsigned int controlCode,const Stream::StreamID &id);
    void receiveFullChunk(unsigned int whichSocket, Stream::StreamID id,const Chunk&newChunk);
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
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    void connectionFailedCallback(unsigned int whichSocket, const std::string& error);
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
    * The connection failed before any sockets were established (or as a helper function after they have been cleaned)
    * This function will call all substreams disconnected methods
    */
    void hostDisconnectedCallback(const std::string& error);
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

    void connect(const Address&address, unsigned int numSockets);

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
