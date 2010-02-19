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
#include "../util/Time.hpp"

namespace Sirikata {
namespace Network {

/// Indicates which combination of reliability and ordered-ness should be used.
enum StreamReliability {
    Unreliable,
    ReliableUnordered,
    ReliableOrdered
};

/** Stream interface for network connections.
 *
 *  Streams are lightweight communication primitives, backed by a shared connection.
 *  An individual stream is a reliable, ordered stream of arbitrarily sized messages.
 *  The stream only returns full messages to the user.
 *
 *  Streams can be created or cloned efficiently and can start accepting data without
 *  having to first go through a connection procedure. When substreams are created by
 *  the remote endpoint, the user is notified via a callback.  Data can be sent from
 *  any thread.  Data is received via a callback, but the user can pause callbacks if
 *  they cannot handle the rate of callbacks provided by the Stream.
 *
 *  Note that substreams may outlast their parent streams.  In this case, further substream
 *  callbacks are dispatched to the original callback.
 */
class SIRIKATA_EXPORT Stream {
protected:
    Stream(){}
public:
    class SetCallbacks;

    virtual ~Stream(){};

    /** Unique identifier for streams backed by the same connection.  Identifiers
     *  are always less than 2^30 and are reused when streams are closed.
     */
    class StreamID: public vuint32 {
    public:
        StreamID(uint32 input) :vuint32(input){
        }
        StreamID(){}
        enum {
            MAX_HEX_SERIALIZED_LENGTH=sizeof(uint32)*2+1
        };
        static uint8 fromHex(char c) {
            if (c>='0'&&c<='9') return c-'0';
            if (c>='A'&&c<='Z') return (c-'A')+10;
            return (c-'a')+10;
        }
        static char toHex(uint8 h) {
            if (h<=9) return '0'+h;
            return (h-10)+'A';
        }
        unsigned int serializeToHex(uint8 *destination, unsigned int maxsize) const {
            assert(maxsize>1);
            uint32 tmp = read();
            unsigned int count=0;
            do {
                destination[count++]=toHex(tmp%16);
                tmp/=16;
            }while(count<maxsize&&tmp);
            std::reverse(destination,destination+count);
            destination[count++]='%';
            return count;
        }        
        bool unserializeFromHex(const uint8*src, unsigned int&size) {
            if (size==0) return false;
            unsigned int maxsize=size;
            uint64 retval=0;
            uint64 temp=0;
            bool success=false;
            for (size=0;size<maxsize;++size) {
                char c=src[size];
                if (c=='%') {
                    ++size;
                    success=true;
                    break;
                }
                retval*=16;
                retval+=fromHex(c);
            }
            *this=StreamID((uint32)retval);
            return success;
        }
        
    };

    /// Specifies the current state of a connection.
    enum ConnectionStatus {
        Connected,
        ConnectionFailed,
        Disconnected
    };

    /** Specifies whether the user accepted a chunk or wants the stream to retain ownership
     *  and pause receive callbacks.
     */
    enum ReceivedResponse {
        AcceptedData,
        PauseReceive
    };

    /** Callback generated when the current state of the underlying connection changes.
     *  The parameters are a ConnectionStatus indicating the event and a string specifying
     *  the reason for the event, if any.
     */
    typedef std::tr1::function<void(ConnectionStatus,const std::string&reason)> ConnectionCallback;

    /** Callback generated when a new substream is initiated by the remote endpoint.
     *  The parameters are a pointer to the new Stream and a functor which allows the user
     *  to set the callbacks for the new Stream.  The substream's callbacks may only be
     *  set in this callback. The user may start sending on the stream as soon as the
     *  callbacks have been set.
     */
    typedef std::tr1::function<void(Stream*,SetCallbacks&)> SubstreamCallback;

    /** Callback generated when another chunk of data is ready. The chunk is an entire chunk
     *  provided by the sender and will not be fragmented.
     *
     *  The return value provided by the user specifies whether they have accepted the data
     *  or if the stream should retain ownership. If the stream retains ownership, receive
     *  calbacks are paused and will not resume until the user callse readyRead().
     *
     *  The chunk is mutable.  It may be destroyed by the user during the callback, but if
     *  it is, the user must be sure to return Accepted to indicate it has taken ownership.
     */
    typedef std::tr1::function<ReceivedResponse(Chunk&)> ReceivedCallback;

    /** Callback generated when the previous send failed and the stream is now ready to accept
     *  a message the same size as the message that caused the failure.
     */
    typedef std::tr1::function<void()> ReadySendCallback;

    /** Functor which allows the user to set callbacks for the stream. */
    class SetCallbacks : Noncopyable {
    public:
        virtual ~SetCallbacks(){}
        /**
         * Function to be called from within a SubstreamCallback to set the callback functions
         * of a newly cloned or received stream. This allows bytes to be immediately sent off
         */
        virtual void operator()(const Stream::ConnectionCallback &connectionCallback,
                                const Stream::ReceivedCallback &receivedCallback,
                                const Stream::ReadySendCallback&readySendCallback)=0;
    };

    /** Default SubstreamCallback which ignores the incoming stream. The stream is destroyed,
     *  causing the remote endpoint to receive a disconnect.
     */
    static void ignoreSubstreamCallback(Stream * stream, SetCallbacks&);
    /** Default ConnectionCallback which ignores the update. */
    static void ignoreConnectionCallback(ConnectionStatus status,const std::string&reason);
    /** Default ReceivedCallback which accepts, but ignores, the data. */
    static ReceivedResponse ignoreReceivedCallback(const Chunk&);
    /** Default ReadySendCallback which ignores the update. */
    static void ignoreReadySendCallback();

    /** Connect the the specified address and use the callbacks for the new stream.
     *  \param addr remote endpoint to connect to
     *  \param substreamCallback callback to invoke when new substreams are created, including the
     *                           first stream that will result from this connection
     *  \param connectionCallback callback to invoke on connection events
     *  \param receivedCallback callback to invoke when a full message has been received
     *  \param readySendCallback callback to invoke when the stream is able to send again after
     *                           previously failing to accept a message
     */
    virtual void connect(
        const Address& addr,
        const SubstreamCallback& substreamCallback,
        const ConnectionCallback& connectionCallback,
        const ReceivedCallback& receivedCallback,
        const ReadySendCallback& readySendCallback)=0;

    /** Create a new Stream of the same type as this stream.
     *  \returns a new Stream, completely independent of this one, of the same type and implementation.
     */
    virtual Stream* factory()=0;

    /** Create a new substream backed by the same connection as this stream.
     *  \param cb callback which is called during this method's invocation to allow the user to set
     *            the callbacks on the new Stream
     *  \returns a new Stream, or NULL if the Stream cannot be created
     */
    virtual Stream* clone(const SubstreamCallback&cb)=0;
    /** Create a new substream backed by the same connection as this stream.
     *  \param connectionCallback callback to invoke on connection events
     *  \param chunkReceivedCallback callback to invoke when messages are received
     *  \param readySendCallback callback to invoke when the Stream is ready to accept more data to send
     *  \returns a new Stream, or NULL if the Stream cannot be created
     */
    virtual Stream* clone(const ConnectionCallback &connectionCallback,
                          const ReceivedCallback&chunkReceivedCallback,
                          const ReadySendCallback&readySendCallback)=0;

    /** Indicate to the Stream that the user is ready to accept more received data.  This should be called
     *  when the user is ready to receive more data after having returned PauseReceive from a
     *  ReceivedCallback.
     */
    virtual void readyRead()=0;

    /* Request that the Stream call the ReadySendCallback specified by the user at stream creation when the
     * Stream is able to send additional data.  This can be called after a call to send() fails so the user
     * is notified when a message of the same size can be enqueued for sending. Calling this method
     * guarantees the user will receive the callback, even if space became available between the failure
     * of the send() call and the call to this method. Note that multiple requests may be handled by a
     * single callback and the callback may be invoked inside this method.
     */
    virtual void requestReadySendCallback()=0;

    /** Enqueue a message to be sent, using the specified level of reliability.
     *  \param msg the message to send
     *  \param reliability the reliability and ordering to send the message with
     *  \returns true if the message was accepted, false if the send failed due to a lost connection or
     *           insufficient queue space
     */
    virtual bool send(MemoryReference msg, StreamReliability reliability)=0;
    /** Enqueue a message which is split in two pieces to be sent, using the specified level of reliability.
     *  \param first_msg the first part of the message to send
     *  \param second_msg the second part of the message to send
     *  \param reliability the reliability and ordering to send the message with
     *  \returns true if the message was accepted, false if the send failed due to a lost connection or
     *           insufficient queue space
     */
    virtual bool send(MemoryReference first_msg, MemoryReference second_msg, StreamReliability reliability)=0;
    /** Enqueue a message to be sent, using the specified level of reliability.
     *  \param data the message to send
     *  \param reliability the reliability and ordering to send the message with
     *  \returns true if the message was accepted, false if the send failed due to a lost connection or
     *           insufficient queue space
     */
    virtual bool send(const Chunk&data, StreamReliability reliability)=0;

    /** Determine if a message of the specified size could be enqueued to be sent.
     *  \returns true if a message of the specified size could be successfully enqueued
     */
    virtual bool canSend(size_t dataSize)const=0;

    /** Get the remote endpoint's address.
     *  \returns the remote endpoint's address, or Address::null() if this Stream has not fully connected yet
     */
    virtual Address getRemoteEndpoint() const=0;
    /** Get the local endpoint's address.
     *  \returns the local endpoint's address, or Address::null() if this Stream has not fully connected yet
     */
    virtual Address getLocalEndpoint() const=0;

    /** Close this Stream, notifying the remote endpoint.  If this is the last substream surviving from the
     *  original Stream, the underlying connection is closed as well.
     */
    virtual void close()=0;


    // -- Statistics

    /** Get the average latency spent in queues in the stream.  This does not include time
     *  spent locally but buffered in an underlying network library or the OS.
     */
    virtual Duration averageSendLatency() const {
        return Duration::zero();
    }

    /** Get the average latency spent in queues in the stream, waiting for deliver to the
     *  user.  This does not include time spent locally in the underlying network library
     *  or OS.
     */
    virtual Duration averageReceiveLatency() const {
        return Duration::zero();
    }

};
} // namespace Network
} // namespace Sirikata

#endif //SIRIKATA_Stream_HPP__
