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

/////////////// Inner Classes

    class PerSpaceData;
public:
    struct ReceivedMessage {
        SpaceObjectReference sourceObject;
        MessagePort sourcePort;
        MessagePort destinationPort;

        MessagePort getPort() const {
            return sourcePort;
        }
        const SpaceID &getSpace() const {
            return sourceObject.space();
        }
        const ObjectReference &getSender() const {
            return sourceObject.object();
        }

        MessagePort getThisPort() const {
            return destinationPort;
        }
        const ProxyObjectPtr &getThisProxy(const HostedObject &hosted) const {
            return hosted.getProxy(getSpace());
        }
        ObjectHostProxyManager *getProxyManager(const HostedObject &hosted) const {
            const ProxyObjectPtr &obj = getThisProxy(hosted);
            if (obj) {
                return static_cast<ObjectHostProxyManager*>(obj->getProxyManager());
            }
            return NULL;
        }

        ProxyObjectPtr getSenderProxy(const HostedObject &hosted) const {
            ObjectHostProxyManager *ohpm = getProxyManager(hosted);
			if (ohpm) {
				ProxyObjectPtr proxyPtr (ohpm->getProxyObject(SpaceObjectReference(getSpace(), getSender())));
				return proxyPtr;
			}
			return ProxyObjectPtr();
        }

        String name;
        MemoryReference body;

        ReceivedMessage(const SpaceID &space, const ObjectReference &source,
                        MessagePort sourcePort, MessagePort destinationPort)
            : sourceObject(space,source), sourcePort(sourcePort), destinationPort(destinationPort), body("",0) {
        }
    };

    class SentMessage;
    typedef std::tr1::unordered_map<int64, SentMessage*> SentMessageMap;

    class SentMessage {
    public:
        // QueryCallback returns true if the query should remain (this is something agreed upon in the protocol, such as a "recurring" flag.
        typedef std::tr1::function<bool (const HostedObjectPtr &thus, SentMessage* sentMessage, const RoutableMessage &responseMessage)> QueryCallback;

    private:
        struct TimerHandler;
        TimerHandler *mTimerHandle;
        int64 mId;

        QueryCallback mResponseCallback;
        RoutableMessageHeader mHeader;
        RoutableMessageBody *mBody;
        HostedObjectWPtr mParent;
    public:
        SentMessage(const HostedObjectPtr &parent);

        ~SentMessage();

        void setCallback(const QueryCallback &cb) {
            mResponseCallback = cb;
        }

        const RoutableMessageBody &body() const {
            return *mBody;
        }
        const RoutableMessageHeader &header() const {
            return mHeader;
        }
        RoutableMessageBody &body() {
            return *mBody;
        }
        RoutableMessageHeader &header() {
            return mHeader;
        }

        MessagePort getPort() const {
            return header().destination_port();
        }
        const SpaceID &getSpace() const {
            return header().destination_space();
        }
        const ObjectReference &getRecipient() const {
            return header().destination_object();
        }

        MessagePort getThisPort() const {
            return header().source_port();
        }
        const ProxyObjectPtr &getThisProxy(const HostedObject &hosted) const {
            return hosted.getProxy(getSpace());
        }
        ObjectHostProxyManager *getProxyManager(const HostedObject &hosted) const {
            const ProxyObjectPtr &obj = getThisProxy(hosted);
            if (obj) {
                return static_cast<ObjectHostProxyManager*>(obj->getProxyManager());
            }
            return NULL;
        }

        ProxyObjectPtr getRecipientProxy(const HostedObject &hosted) const {
            ObjectHostProxyManager *ohpm = getProxyManager(hosted);
			if (ohpm) {
				ProxyObjectPtr proxyPtr (ohpm->getProxyObject(SpaceObjectReference(getSpace(), getRecipient())));
				return proxyPtr;
			}
			return ProxyObjectPtr();
        }

        int64 getId() const {
            return mId;
        }

        void send(const HostedObjectPtr &parent, int timeout=0);
        void gotResponse(const HostedObjectPtr &hostedObj, const SentMessageMap::iterator &iter, const RoutableMessage &msg);
    };

private:
////////// Members

    SentMessageMap mSentMessages;
    int64 mNextQueryId;

    typedef std::map<SpaceID, PerSpaceData> SpaceDataMap;
    SpaceDataMap mSpaceData;

    // name -> encoded property message
    typedef std::map<String, String> PropertyMap;
    PropertyMap mProperties;

    ObjectHost *mObjectHost;
    UUID mInternalObjectReference;

///////// Constructors/Destructors
private:
    friend class ::Sirikata::SelfWeakPtr<HostedObject>;
/// Private: Use "SelfWeakPtr<HostedObject>::construct(ObjectHost*)"
    HostedObject(ObjectHost*parent);

public:
/// Destructor: will only be called from shared_ptr::~shared_ptr.
    ~HostedObject();


private:
//////// Private member functions:
    PerSpaceData &cloneTopLevelStream(const SpaceID&,const std::tr1::shared_ptr<TopLevelSpaceConnection>&);

//////// Callbacks:
    struct PrivateCallbacks;

public:
//////// Public member functions:

    ///makes a new objects with objectName startingLocation mesh and a space to connect to
    void initializeConnect(const UUID &objectName, const Location&startingLocation,const String&mesh, const BoundingSphere3f&meshBounds, const LightInfo *lights, const SpaceID&, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr());
    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space
    void initializeScripted(const UUID &objectName, const String&script, const SpaceID&id, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr() );
    //.attempt to restore this item from database including script
    void initializeRestoreFromDatabase(const UUID &objectName);
    ObjectHost *getObjectHost()const {return mObjectHost;}

    const ProxyObjectPtr &getProxy(const SpaceID &space) const;

    bool hasProperty(const String &propName) const;
    const String &getProperty(const String &propName) const;
    String *propertyPtr(const String &propName);
    void setProperty(const String &propName, const String &encodedValue=String());
    void unsetProperty(const String &propName);

    //FIXME implement SpaceConnection& connect(const SpaceID&space);
    //FIXME implement SpaceConnection& connect(const SpaceID&space, const SpaceConnection&example);
    bool send(const RoutableMessageHeader &hdr, const MemoryReference &body);

    void processMessage(const ReceivedMessage &msg, String *returnValue);
    void receivedPropertyUpdate(const ProxyObjectPtr &proxy, const String &propertyName, const String &arguments);
    void receivedPositionUpdate(const ProxyObjectPtr &proxy, const ObjLoc &objLoc, bool force_reset);

    void setScale(Vector3f scale); ///< temporary--ideally this property could be set by others.

};

typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;

}

#endif
