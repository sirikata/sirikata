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
namespace Sirikata { namespace Network {
typedef std::vector<uint8> Chunk;
/**
 * This is the stream interface by which applications will send packets to the world
 * Each protocol will provide a specific function similar to the connect and bind functions 
 * but with protocol-specific arguments (such as host, port)
 * As soon as connect is called, streams may be cloned or data may be sent across them
 * When the stream finds out whether a connection attempt failed or succeeded, 
 * it will call the connectionCallback
 * When the other side calls clone() the first side will receive a substreamCallback
 * 
 * -Unresolved:
 * Should a stream that clone()d a substream and then closes force the substream to close
 *  + this would help build compatibility with structured streams
 *  + It would become easier to shut the whole application down by closing the toplevel streams
 *  - This makes synchronization difficult, because the other ops can pretty much use os-stream level synchronization on the socket itself
 *  - It's unclear where, aside from shutdown, this sort of recursive closure helps out
 *  or we could do the worst of both worlds: do a best effort check in debug mode and call it 'undefined' behavior to use a closed substream
 */
class Stream {
protected:
    Stream(){}
public:
    class Callbacks;
    class StreamID{
        unsigned int mID;
    public:
        bool odd() const{
            return (mID&1);
        }
        //unserializes a streamID into a buffer of size at least 8. Caller should check return value to see if size is too small
        unsigned int serialize(uint8 *destination, unsigned int maxsize)const;
        //unserializes a streamID into a buffer where the size is at least size...puts bytes consumed into size variable
        bool unserialize(const uint8 *src, unsigned int &size);
        StreamID(){//control packet
            mID=0;
        }
        StreamID(unsigned int id){
            mID=id;
        }
		/// Hasher functor to be used in a hash_map.
		struct Hasher {
			size_t operator() (const StreamID &id) const{
				return std::tr1::hash<unsigned int>()(id.mID);
			}
		};

        bool operator == (const StreamID&other)const {
            return mID==other.mID;
        }
        bool operator != (const StreamID&other)const {
            return mID!=other.mID;
        }
        bool operator < (const StreamID&other)const {
            return mID<other.mID;
        }
    };
    enum ConnectionStatus {
        Connected,
        ConnectionFailed,
        Disconnected
    };
    enum Reliability {
        Unreliable,
        ReliableUnordered,
        Reliable
    };
    static Callbacks*ignoreSubstreamCallback(Stream * stream);
    static void ignoreConnectionStatus(ConnectionStatus status,const std::string&reason);
    static void ignoreBytesReceived(const Chunk&);
    typedef std::tr1::function<void(ConnectionStatus,const std::string&reason)> ConnectionCallback;
    ///The SubstreamCallback must return a new pointer to a set of callback function. These callback functions will live until the Stream is deleted and the Callbacks* returned
    ///might live longer
    typedef std::tr1::function<Callbacks*(Stream*)> SubstreamCallback;
    typedef std::tr1::function<void(const Chunk&)> BytesReceivedCallback;
    class Callbacks:public Noncopyable {        
    public:
        Stream::ConnectionCallback mConnectionCallback;
        Stream::SubstreamCallback mSubstreamCallback;
        Stream::BytesReceivedCallback mBytesReceivedCallback;
        Callbacks(const Stream::ConnectionCallback &connectionCallback,
                  const Stream::SubstreamCallback &substreamCallback,
                  const Stream::BytesReceivedCallback &bytesReceivedCallback):
            mConnectionCallback(connectionCallback),mSubstreamCallback(substreamCallback),mBytesReceivedCallback(bytesReceivedCallback){
        }
    };
    ///subclasses will expose these methods with similar arguments + protocol specific args
    virtual void connect(
        const Address& addy,
        const ConnectionCallback &connectionCallback,
        const SubstreamCallback &substreamCallback,
        const BytesReceivedCallback&chunkReceivedCallback)=0;
    ///Creates a new substream on this connection
    virtual Stream*clone(
        const ConnectionCallback &connectionCallback,
        const SubstreamCallback &substreamCallback,
        const BytesReceivedCallback&chunkReceivedCallback)=0;
    
    
    ///Send a chunk of data to the receiver
    virtual void send(const Chunk&data,Reliability)=0;
    ///close this stream: if it is the last stream, close the connection (unresolved: what if this stream has children streams--do those forcably close?)
    virtual void close()=0;
    virtual ~Stream(){};
};
} }
#endif
