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

namespace Sirikata {
class ObjectHost;
class ProxyObject;
class ProxyPositionObject;
class LightInfo;
typedef std::tr1::shared_ptr<ProxyPositionObject> ProxyPositionObjectPtr;
class TopLevelSpaceConnection;
// ObjectHost_Sirikata.pbj.hpp
namespace Protocol {
class ReadOnlyMessage;
class ObjLoc;
}
using Protocol::ReadOnlyMessage;
using Protocol::ObjLoc;


class HostedObject;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
class SIRIKATA_OH_EXPORT HostedObject : public SelfWeakPtr<HostedObject> {
    class PerSpaceData;
public:
    struct ReceivedMessage {
        SpaceObjectReference sourceObject;
        MessagePort sourcePort;
        MessagePort destinationPort;
        String name;
        MemoryReference body;
        PerSpaceData *perSpaceData;

        ReceivedMessage(const SpaceID &space, const ObjectReference &source,
                        MessagePort sourcePort, MessagePort destinationPort)
            : sourceObject(space,source), sourcePort(sourcePort), destinationPort(destinationPort), body("",0), perSpaceData(NULL) {
        }
    };
private:
    class PerSpaceData {
    public:
        SpaceConnection mSpaceConnection;
        ProxyPositionObjectPtr mProxyObject; /// 
        PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream);
    };

    typedef std::tr1::function<void (const ReadOnlyMessage &responseMessage)> QueryCallback;
    typedef std::map<int64, QueryCallback> RunningQueryMap;
    RunningQueryMap mQueries;

    typedef std::map<SpaceID, PerSpaceData> SpaceDataMap;
    SpaceDataMap mSpaceData;

    // name -> encoded property message
    typedef std::map<String, String> PropertyMap;
    PropertyMap mProperties;

    ObjectHost *mObjectHost;
    UUID mInternalObjectReference;
    friend class ::Sirikata::SelfWeakPtr<HostedObject>;
    HostedObject(ObjectHost*parent);
    PerSpaceData &cloneTopLevelStream(const SpaceID&,const std::tr1::shared_ptr<TopLevelSpaceConnection>&);
    void receivedProxObjectProperties(const SpaceObjectReference &proximateObjectId, int32 queryId, const std::vector<std::string> &propertyRequests, const ReadOnlyMessage &responseMessage);
    static void receivedRoutableMessage(const HostedObjectWPtr&thus,const SpaceID&sid,const Network::Chunk&);
public:
    static void disconnectionEvent(const HostedObjectWPtr&thus,const SpaceID&sid,const String&reason);
    ///makes a new objects with objectName startingLocation mesh and a space to connect to
    void initializeConnect(const UUID &objectName, const Location&startingLocation,const String&mesh, const BoundingSphere3f&meshBounds, const LightInfo *lights, const SpaceID&, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr());
    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space
    void initializeScripted(const UUID &objectName, const String&script, const SpaceID&id, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr() );
    //.attempt to restore this item from database including script
    void initializeRestoreFromDatabase(const UUID &objectName);
    ObjectHost *getObjectHost()const {return mObjectHost;}

    std::tr1::shared_ptr<ProxyObject> getProxy(const SpaceID &space) const;

    bool hasProperty(const String &propName) const;
    const String &getProperty(const String &propName) const;
    String *propertyPtr(const String &propName);
    void setProperty(const String &propName, const String &encodedValue=String());
    void unsetProperty(const String &propName);

    //FIXME implement SpaceConnection& connect(const SpaceID&space);
    //FIXME implement SpaceConnection& connect(const SpaceID&space, const SpaceConnection&example);
    bool send(const RoutableMessageHeader &hdr, const MemoryReference &body);

    void processMessage(const ReceivedMessage &msg, String *returnValue);
    void receivedPropertyUpdate(const ProxyPositionObjectPtr &proxy, const String &propertyName, const String &arguments);
    void receivedPositionUpdate(const ProxyPositionObjectPtr &proxy, const ObjLoc &objLoc, bool force_reset);

    void setScale(Vector3f scale); ///< temporary--ideally this property could be set by others.

};

typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;

}

#endif
