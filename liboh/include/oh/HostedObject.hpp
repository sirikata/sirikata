/*  Sirikata liboh -- Object Host
 *  HostedObject.hpp
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
#ifndef _SIRIKATA_HOSTED_OBJECT_HPP_
#define _SIRIKATA_HOSTED_OBJECT_HPP_

#include <util/SpaceObjectReference.hpp>
#include <util/RoutableMessageHeader.hpp>
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/ProxyObject.hpp"

namespace Sirikata {
class ObjectHost;
class ProxyObject;
class ProxyObject;
class LightInfo;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;
class TopLevelSpaceConnection;
// ObjectHost_Sirikata.pbj.hpp
namespace Protocol {
class ObjLoc;
}
using Protocol::ObjLoc;

class HostedObject;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
class SIRIKATA_OH_EXPORT HostedObject : public SelfWeakPtr<HostedObject> {
    class PerSpaceData;
public:
    /** Structure passed to processMessage() upon receiving a message.
        Contains a source SpaceObjectReference, and both ports, as well as
        the method name, and the encoded arguments for that method.
        Note that the full RoutableMessageBody is split up, and only one method
        is sent at a time.
     */
    struct ReceivedMessage {
        SpaceObjectReference sourceObject; ///< Full SpaceObjectReference of sender.
        MessagePort sourcePort; ///< Port of sender.
        MessagePort destinationPort; ///< Port of reciever.

        String name; ///< method name
        MemoryReference body; ///< Protobuf encoded body.

        /// Gets the port of the sender (other client), default 0.
        MessagePort getPort() const {
            return sourcePort;
        }
        /** Gets the space we are in.
            Note: both sender and receiver must be in the same space. */
        const SpaceID &getSpace() const {
            return sourceObject.space();
        }
        /// Gets the ObjectReference of the sender (other client).
        const ObjectReference &getSender() const {
            return sourceObject.object();
        }

        /// Gets the port of the recipient (this object). default 0.
        MessagePort getThisPort() const {
            return destinationPort;
        }
        /// Gets the ProxyObject of the receiver (this object).
        const ProxyObjectPtr &getThisProxy(const HostedObject &hosted) const {
            return hosted.getProxy(getSpace());
        }
        /** Gets the ProxyManager representing this space. It must contain
            this object, and it might contain the sender object. */
        ObjectHostProxyManager *getProxyManager(const HostedObject &hosted) const {
            const ProxyObjectPtr &obj = getThisProxy(hosted);
            if (obj) {
                return static_cast<ObjectHostProxyManager*>(obj->getProxyManager());
            }
            return NULL;
        }

        /// Gets the ProxyObject of the sender (other object), if in range.
        ProxyObjectPtr getSenderProxy(const HostedObject &hosted) const {
            ObjectHostProxyManager *ohpm = getProxyManager(hosted);
			if (ohpm) {
				ProxyObjectPtr proxyPtr (ohpm->getProxyObject(SpaceObjectReference(getSpace(), getSender())));
				return proxyPtr;
			}
			return ProxyObjectPtr();
        }

        /// Simple constructor, takes only the arguments for the RoutableMessageHeader.
        ReceivedMessage(const SpaceID &space, const ObjectReference &source,
                        MessagePort sourcePort, MessagePort destinationPort)
            : sourceObject(space,source), sourcePort(sourcePort), destinationPort(destinationPort), body("",0) {
        }
    };

    class SentMessage;
    /// Each sent query has an int64 id. This maps that id to a SentMessage structure.
    typedef std::tr1::unordered_map<int64, SentMessage*> SentMessageMap;

    /** A message/query that is sent to another object to have at least one
        response sent back. A timeout can be specified, in case the other object
        does not respond to our messages.
     */
    class SentMessage {
    public:
        /** QueryCallback will be called when the other client responds to our
            sent query.

            @param thus  A shared_ptr to the HostedObject who sent the message.
            @param sentMessage  A pointer to this SentMessage.
            @param responseMessage  A RoutableMessage to be parsed.
                   The method list of the original message is available in
                   sentMessage->body().message_names(), and should line up with
                   the number of arguments in responseMessage.body().message_arguments().
            @returns  true if the query should remain (this is something agreed upon in the protocol, such as a "recurring" flag), or false if this is a one-shot message.

            @note You must check that responseMessage.return_status() == SUCCESS.
                  QueryCallback will be called even in the case of a TIMEOUT_FAILURE.
        */
        typedef std::tr1::function<bool (const HostedObjectPtr &thus, SentMessage* sentMessage, const RoutableMessage &responseMessage)> QueryCallback;

    private:
        struct TimerHandler;
        TimerHandler *mTimerHandle; ///< Holds onto the timeout, if one exists.
        int64 mId; ///< This query ID. Chosen randomly in the constructor.

        QueryCallback mResponseCallback; ///< Callback, or null if not yet set.
        RoutableMessageHeader mHeader; ///< Header embedded into the struct
        RoutableMessageBody *mBody; ///< Body allocated in the constructor.
    public:
        /// Constructor takes in a HostedObject.
        SentMessage(const HostedObjectPtr &parent);
        /// Destructor to deallocate mBody.
        ~SentMessage();

        /// sets the callback handler. Must be called at least once.
        void setCallback(const QueryCallback &cb) {
            mResponseCallback = cb;
        }

        /// body accessor, like that of RoutableMessage. (const ver)
        const RoutableMessageBody &body() const {
            return *mBody;
        }
        /// header accessor, like that of RoutableMessage. (const ver)
        const RoutableMessageHeader &header() const {
            return mHeader;
        }
        /// body accessor, like that of RoutableMessage.
        RoutableMessageBody &body() {
            return *mBody;
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
        const SpaceID &getSpace() const {
            return header().destination_space();
        }
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
        /// The selected ProxyObject corresponding to this object/space combo.
        const ProxyObjectPtr &getThisProxy(const HostedObject &hosted) const {
            return hosted.getProxy(getSpace());
        }
        /// The ProxyManager corresponding to getSpace().
        ObjectHostProxyManager *getProxyManager(const HostedObject &hosted) const {
            const ProxyObjectPtr &obj = getThisProxy(hosted);
            if (obj) {
                return static_cast<ObjectHostProxyManager*>(obj->getProxyManager());
            }
            return NULL;
        }

        /// The ProxyObject, if one exists, of the other object. May return null.
        ProxyObjectPtr getRecipientProxy(const HostedObject &hosted) const {
            ObjectHostProxyManager *ohpm = getProxyManager(hosted);
			if (ohpm) {
				ProxyObjectPtr proxyPtr (ohpm->getProxyObject(SpaceObjectReference(getSpace(), getRecipient())));
				return proxyPtr;
			}
			return ProxyObjectPtr();
        }

        /// The query ID, which can be used to lookup this in mSentMessages.
        int64 getId() const {
            return mId;
        }

        /** Actually sends this message, after body() and header() have been set.
            Note that if resending, body() should still contain the same message 
            that was originally sent out.
            @param parent  the HostedObject sending this message.
            @param timeout  If nonzero, will cause a timeout response.
                            Timeouts only apply to the next callback.
            @note: send must be called *only* when created, or from a callback.
            @note: This must be the same object passed into the constructor!
                   Otherwise, this will not be in the mSentMessagesMap.
         */
        void send(const HostedObjectPtr &parent, int timeout=0);

        /// Simulates a response being received from the other object.
        void gotResponse(const HostedObjectPtr &hostedObj, const SentMessageMap::iterator &iter, const RoutableMessage &msg);
    };

protected:
//------- Members

    SentMessageMap mSentMessages;
    int64 mNextQueryId;

    typedef std::map<SpaceID, PerSpaceData> SpaceDataMap;
    SpaceDataMap mSpaceData;

    // name -> encoded property message
    typedef std::map<String, String> PropertyMap;
    PropertyMap mProperties;

    ObjectHost *mObjectHost;
    UUID mInternalObjectReference;

//------- Constructors/Destructors
private:
    friend class ::Sirikata::SelfWeakPtr<HostedObject>;
/// Private: Use "SelfWeakPtr<HostedObject>::construct(ObjectHost*)"
    HostedObject(ObjectHost*parent);

public:
/// Destructor: will only be called from shared_ptr::~shared_ptr.
    ~HostedObject();


private:
//------- Private member functions:
    PerSpaceData &cloneTopLevelStream(const SpaceID&,const std::tr1::shared_ptr<TopLevelSpaceConnection>&);

//------- Callbacks:
    struct PrivateCallbacks;

public:
//------- Public member functions:

    ///makes a new objects with objectName startingLocation mesh and a space to connect to
    void initializeConnect(const UUID &objectName, const Location&startingLocation,const String&mesh, const BoundingSphere3f&meshBounds, const LightInfo *lights, const SpaceID&, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr());
    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space [not implemented]
    void initializeScripted(const UUID &objectName, const String&script, const SpaceID&id, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr() );
    /// Attempt to restore this item from database including script [not implemented]
    void initializeRestoreFromDatabase(const UUID &objectName);
    /** Gets the ObjectHost (usually one per host).
        See getProxy(space)->getProxyManger() for the per-space object.
    */
    ObjectHost *getObjectHost()const {return mObjectHost;}

    /// Gets the proxy object representing this HostedObject inside space.
    const ProxyObjectPtr &getProxy(const SpaceID &space) const;

    /// Checks for a pulbic property named propName.
    bool hasProperty(const String &propName) const;
    /// Gets the protobuf encoded value of a pulbic property named propName.
    const String &getProperty(const String &propName) const;

    /** Gets a modifiable value for a (often new) public property.
        Is usually used in conjunction with google::protobuf::message::SerializeToString:
        myMessage.SerializeToString(propertyPtr("SomeProperty"));
    */
    String *propertyPtr(const String &propName);
    /// Sets a property with an optional encoded value.
    void setProperty(const String &propName, const String &encodedValue=String());
    /// Deletes a public property from this object.
    void unsetProperty(const String &propName);

    //FIXME implement SpaceConnection& connect(const SpaceID&space);
    //FIXME implement SpaceConnection& connect(const SpaceID&space, const SpaceConnection&example);

    /** Sends a message from the space hdr.destination_space() to the object
        hdr.destination_object().
        @param header  A RoutableMessageHeader: must include destination_space/object.
        @param body  An encoded RoutableMessageBody.
    */
    bool send(const RoutableMessageHeader &hdr, const MemoryReference &body);

    /** Handles a single RPC out of a received message.
        @param msg  A ReceivedMessage struct with sender, message_name, and arguments.
        @param returnValue  A serialized message indicating a return value.
               If NULL, no ID was passed, and no response will be sent.
               If non-NULL, a message is expected to be encoded, but the empty
               string is a valid message if a return value does not apply.
        @see ReceivedMessage
    */
    void processMessage(const ReceivedMessage &msg, String *returnValue);
    /// Call if you know that a property from some other ProxyObject has changed. FIXME: should this be made private?
    void receivedPropertyUpdate(const ProxyObjectPtr &proxy, const String &propertyName, const String &arguments);
    /// Call if you know that a position for some other ProxyObject has changed. FIXME: should this be made private?
    void receivedPositionUpdate(const ProxyObjectPtr &proxy, const ObjLoc &objLoc, bool force_reset);

    void setScale(Vector3f scale); ///< temporary--ideally this property could be set by others.

};

/// shared_ptr, keeps a reference to the HostedObject. Do not store one of these.
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
/// weak_ptr, to be used to hold onto a HostedObject reference. @see SelfWeakPtr.
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;

}

#endif
