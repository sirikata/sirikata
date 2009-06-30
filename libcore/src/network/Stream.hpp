/*  Sirikata Network Utilities
 *  Stream.hpp
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

#ifndef SIRIKATA_Stream_HPP__
#define SIRIKATA_Stream_HPP__
#include "Address.hpp"
namespace Sirikata {
/// Network contains Stream and TCPStream.
namespace Network {


///Codes indicating if packet sending should be reliable or not,and in order or not
enum StreamReliability {
    Unreliable,
    ReliableUnordered,
    ReliableOrdered
};


/**
 * This is the stream interface by which applications will send packets to the world
 * Each protocol will provide a specific function similar to the connect and bind functions 
 * but with protocol-specific arguments (such as host, port)
 * As soon as connect is called, streams may be cloned or data may be sent across them
 * When the stream finds out whether a connection attempt failed or succeeded, 
 * it will call the connectionCallback
 * When the other side calls clone() the first side will receive a substreamCallback with the callback it provided on connect() or listen()
 * Once a stream is clone()d, it may outlast the parent stream and may receive further substream callbacks on the original substreamCallback provided

 * 
 */
class SIRIKATA_EXPORT Stream {
protected:
    Stream(){}
public:
    /**
     * This class stores up to 30 bits of data in a 4 byte memory slot. It may be serialized to between 1 and 4 bytes
     * This class is useful for defining unique StreamIDs as well as other lowlevel data like packet lengths
     */
    class SIRIKATA_EXPORT uint30{
        uint32 mID;
    public:
        ///The maximum length that any uint30 will be serialized out to
        enum MaxSerializedLengthConstants{
            ///The maximum length that any uint30 will be serialized out to
            MAX_SERIALIZED_LENGTH=4
        };
        ///True if this uint32 is odd
        bool odd() const{
            return (mID&1);
        }
        ///Returns the inmemory value of this uint30
        uint32 read() const{
            return mID;
        }
        //unserializes a streamID into a buffer of size at least 8. Caller should check return value to see how much space actually used
        unsigned int serialize(uint8 *destination, unsigned int maxsize)const;
        //unserializes a streamID into a buffer where the size is at least size...puts bytes consumed into size variable returns false if size too small
        bool unserialize(const uint8 *src, unsigned int &size);
        ///Construct an integer filled with 0 sized value. Will be used for control packets for StreamIDs
        uint30(){
            mID=0;
        }
        ///Construct a uint30 based on given integer value
        uint30(unsigned int id){
            assert(id<(1<<30));
            mID=id;
        }
		uint30&operator=(const uint30&other) {
           this->mID=other.mID;
		   return *this;
		}
		uint30(const uint30&other) {
           this->mID=other.mID;
		}
		/// Hasher functor to be used in a hash_map.
		struct Hasher {
            ///returns a hash of the integer value
			size_t operator() (const uint30 &id) const{
				return std::tr1::hash<unsigned int>()(id.mID);
			}
		};
        ///Total ordering of uint30
        bool operator == (const uint30&other)const {
            return mID==other.mID;
        }
        ///Total ordering of uint30
        bool operator != (const uint30&other)const {
            return mID!=other.mID;
        }
        ///Total ordering of uint30
        bool operator < (const uint30&other)const {
            return mID<other.mID;
        }
    };
    ///The number of live streams on a single connection must fit into 30 bits. Streams will be reused when they are shutdown
    typedef uint30 StreamID ;
    ///Callback codes indicating whether the socket has connected, had a connection rejected or got a sudden disconnection
    enum ConnectionStatus {
        Connected,
        ConnectionFailed,
        Disconnected
    };
    ///Callback type for when a connection happens (or doesn't) Callees will receive a ConnectionStatus code indicating connection successful, rejected or a later disconnection event
    typedef std::tr1::function<void(ConnectionStatus,const std::string&reason)> ConnectionCallback;
    ///Callback type for when a full chunk of bytes are waiting on the stream
    typedef std::tr1::function<void(const Chunk&)> BytesReceivedCallback;
    /**
     *  This class is passed into any newSubstreamCallback functions so they may 
     *  immediately setup callbacks for connetion events and possibly start sending immediate responses.     
     */
    class SetCallbacks : Noncopyable{
    public:
        virtual ~SetCallbacks(){}
        /**
         * Function to be called from within a SubstreamCallback to set the callback functions 
         * of a newly cloned or received stream. This allows bytes to be immediately sent off
         */
        virtual void operator()(const Stream::ConnectionCallback &connectionCallback,
                                const Stream::BytesReceivedCallback &bytesReceivedCallback)=0;
    };
    /**
     * The substreamCallback must call SetCallbacks' operator() to activate the stream
     * Then it may send bytes asap
     * it will get called with NULL as the last stream shuts down
     */
    typedef std::tr1::function<void(Stream*,SetCallbacks&)> SubstreamCallback;
    ///Simple example function to ignore incoming streams. This is used when a socket is shutting down, or for convenience
    static void ignoreSubstreamCallback(Stream * stream, SetCallbacks&);
    ///Simple example function to ignore connection requests
    static void ignoreConnectionStatus(ConnectionStatus status,const std::string&reason);
    ///Simple example function to ignore incoming bytes on a connection
    static void ignoreBytesReceived(const Chunk&);
    /**
     * Will attempt to connect to the given provided address, specifying all callbacks for the first successful stream
     * The stream is immediately active and may have bytes sent on it immediately. 
     * A connectionCallback will be called as soon as connection has succeeded or failed
     */
    virtual void connect(
        const Address& addy,
        const SubstreamCallback &substreamCallback,
        const ConnectionCallback &connectionCallback,
        const BytesReceivedCallback&chunkReceivedCallback)=0;

    /**
     * Will specify all callbacks for the first successful stream and allow this stream to be cloned
     * The stream is immediately active and may have bytes sent on it immediately. 
     * A connectionCallback will be called as soon as connection has succeeded or failed
     */
    virtual void prepareOutboundConnection(
        const SubstreamCallback &substreamCallback,
        const ConnectionCallback &connectionCallback,
        const BytesReceivedCallback&chunkReceivedCallback)=0;
    /**
     * Will attempt to connect to the given provided address, specifying all callbacks for the first successful stream
     * A connectionCallback specified in prepareConnection will be called as soon as connection has succeeded or failed
     * must call prepareConnection prior to connect()
     */
    virtual void connect(
        const Address& addy)=0;

    ///Creates a stream of the same type as this stream
    virtual Stream*factory()=0;
    ///Makes this stream a clone of stream "s" if they are of the same type and immediately calls the callback 
    virtual Stream* clone(const SubstreamCallback&cb)=0;
    virtual Stream* clone(const ConnectionCallback &connectionCallback,
                          const BytesReceivedCallback&chunkReceivedCallback)=0;
    
    virtual void send(MemoryReference, StreamReliability)=0;
    virtual void send(MemoryReference, MemoryReference, StreamReliability)=0;
    ///Send a chunk of data to the receiver
    virtual void send(const Chunk&data,StreamReliability)=0;
    ///close this stream: if it is the last stream, close the connection as well
    virtual void close()=0;
    virtual ~Stream(){};
};
} }
#endif
