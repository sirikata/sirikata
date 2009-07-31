/*  Sirikata liboh -- Object Host
 *  SentMessage.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
/*  Created on: Jul 25, 2009 */

#ifndef _SIRIKATA_SentMessage_HPP_
#define _SIRIKATA_SentMessage_HPP_

#include "oh/QueryTracker.hpp"

namespace Sirikata {

class ProxyObject;
class ProxyManager;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;

/** A message/query that is sent to another object to have at least one
    response sent back. A timeout can be specified, in case the other object
    does not respond to our messages.
*/
class SIRIKATA_OH_EXPORT SentMessage {
public:
    /** QueryCallback will be called when the other client responds to our
        sent query.

        @param sentMessage  A pointer to this SentMessage.
        @param responseHeader  A RoutableMessageHeader
        @param responseBody    A MemoryReference to be parsed of the body
        @returns  If this is a one-shot message, this SentMessage should be deleted.

        @note You must check that responseHeader.return_status() == SUCCESS.
        QueryCallback will be called even in the case of a TIMEOUT_FAILURE.

        @note: Make sure to delete the sub-type: "delete static_cast<MySubType*>(sentMessage);"
    */
    typedef std::tr1::function<void (SentMessage* sentMessage, const RoutableMessageHeader &responseHeader, MemoryReference responseBody)> QueryCallback;

private:
    struct TimerHandler;
    TimerHandler *mTimerHandle; ///< Holds onto the timeout, if one exists.
    const int64 mId; ///< This query ID. Passed to the constructor.

    RoutableMessageHeader mHeader; ///< Header embedded into the struct

    QueryCallback mResponseCallback; ///< Callback, or null if not yet set.
    QueryTracker *const mTracker;

public:
    /// Destructor: Caution: NOT VIRTUAL!!! Make sure to downcast if necessary!!
    ~SentMessage();
    /// Constructor allocates a new queryId. Caller is expected to keep track of a map.
    SentMessage(QueryTracker *sender);

    /// Constructor takes in a queryId. Caller is expected to keep track of a map.
    SentMessage(int64 thisId, QueryTracker *sender);

    /// header accessor, like that of RoutableMessage. (const ver)
    const RoutableMessageHeader &header() const {
        return mHeader;
    }
    /// header accessor, like that of RoutableMessage.
    RoutableMessageHeader &header() {
        return mHeader;
    }

    /** Port of the other object: recipient of this message, and sender
        of the response(s) */
    MessagePort getPort() const {
        return header().destination_port();
    }
    /// Space of both this and the other object.
    const SpaceID &getSpace() const;
    /** ObjectReference of the other object: recipient of this message, and
        sender of the response(s) */
    const ObjectReference &getRecipient() const {
        return header().destination_object();
    }

    /** Port of the original sent message in this object. Note that this might
        differ from the port that the response was sent to
        (responseMessage.destination_port()), as long as the SpaceID and the
        ObjectReference is the same */
    MessagePort getThisPort() const {
        return header().source_port();
    }

    /// The ProxyObject, if one exists, of the other object. May return null.
    ProxyObjectPtr getRecipientProxy(const ProxyManager*pm) const;

    /// The query ID allocated--should be the same as header().id()
    int64 getId() const {
        return mId;
    }

    /// Returns the query map this SentMessage is a member of (which assigned its id)
    QueryTracker *getTracker() const {
        return mTracker;
    }

    /// sets the callback handler. Must be called at least once.
    void setCallback(const QueryCallback &cb) {
        mResponseCallback = cb;
    }

    /** Unsets any pending timeout. This will happen automatically upon
        receiving a callback. */
    void unsetTimeout();
    /**
        Set a timer for the next callback.
        @param timeout  If nonzero, will cause a timeout response. If zero,
          will cause a timeout to occur if we did not get a synchronous response.
        @note  You must call setTimeout each time you send().
    */
    void setTimeout(int timeout);

    /** Calls the callback handler for this message. Does not validate the
        sender of msg. If you need to check this, use QueryTracker::processMessage().
    */
    void processMessage(const RoutableMessageHeader &msg, MemoryReference body);

    /** Actually sends this message, after body() and header() have been set.
        Note that if resending, body() should still contain the same message
        that was originally sent out.
    */
    void send(MemoryReference body);
};

/** Subclass of SentMessage which holds onto the body for help when
    parsing response messages--also gives a clean place to keep track of
    several incomplete responses in a row.
*/
template <class Body>
class SentMessageBody : public SentMessage {
public:
private:
    Body mBody;
public:
    SentMessageBody(QueryTracker *tracker)
        : SentMessage(tracker) {
    }
    SentMessageBody(int64 id, QueryTracker *tracker)
        : SentMessage(id, tracker) {
    }
    /// Note: Not virtual, make sure to downcast!!! Template + Virtual = :-(
    ~SentMessageBody() {
    }

    /// body accessor, like that of RoutableMessage. (const ver)
    const Body &body() const {
        return mBody;
    }
    /// body accessor, like that of RoutableMessage.
    Body &body() {
        return mBody;
    }

    /// Equivalent to "string str; body().SerializeToString(str): send(str);"
    void serializeSend() {
        std::string bodyStr;
        body().SerializeToString(&bodyStr);
        send(MemoryReference(bodyStr));
    }
};

}

#endif
