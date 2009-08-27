/*  Sirikata liboh -- Object Host
 *  HostedObject.cpp
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

#include <util/Platform.hpp>
#include <oh/Platform.hpp>
#include <ObjectHost_Sirikata.pbj.hpp>
#include <ObjectHost_Persistence.pbj.hpp>
#include <task/WorkQueue.hpp>
#include "util/RoutableMessage.hpp"
#include "util/KnownServices.hpp"
#include "persistence/PersistenceSentMessage.hpp"
#include "network/Stream.hpp"
#include "util/SpaceObjectReference.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "util/SentMessage.hpp"
#include "oh/ObjectHost.hpp"
#include "oh/ProxyMeshObject.hpp"
#include "oh/ProxyLightObject.hpp"
#include "oh/ProxyCameraObject.hpp"
#include "oh/LightInfo.hpp"
#include "oh/ObjectScriptManager.hpp"
#include "oh/ObjectScript.hpp"
#include "oh/ObjectScriptManagerFactory.hpp"
#include <util/KnownServices.hpp>

namespace Sirikata {

typedef SentMessageBody<RoutableMessageBody> RPCMessage;

class HostedObject::PerSpaceData {
public:
    SpaceConnection mSpaceConnection;
    ProxyObjectPtr mProxyObject;
    ProxyObject::Extrapolator mUpdatedLocation;

    void locationWasReset(Time timestamp, Location loc) {
        loc.setVelocity(Vector3f::nil());
        loc.setAngularSpeed(0);
        mUpdatedLocation.resetValue(timestamp, loc);
    }
    void locationWasSet(const Protocol::ObjLoc &msg) {
        Time timestamp = msg.timestamp();
        Location loc = mUpdatedLocation.extrapolate(timestamp);
        ProxyObject::updateLocationWithObjLoc(loc, msg);
        loc.setVelocity(Vector3f::nil());
        loc.setAngularSpeed(0);
        mUpdatedLocation.updateValue(timestamp, loc);
    }

    void updateLocation(HostedObject *ho) {
        if (!mProxyObject) {
            return;
        }
        SpaceID space = mProxyObject->getObjectReference().space();
        Time now = Time::now(ho->getSpaceTimeOffset(space));
        Location realLocation = mProxyObject->globalLocation(now);
        if (mUpdatedLocation.needsUpdate(now, realLocation)) {
            Protocol::ObjLoc toSet;
            toSet.set_position(realLocation.getPosition());
            toSet.set_velocity(realLocation.getVelocity());
            RoutableMessageBody body;
            toSet.SerializeToString(body.add_message("ObjLoc"));
            RoutableMessageHeader header;
            header.set_destination_port(Services::LOC);
            header.set_destination_object(ObjectReference::spaceServiceID());
            header.set_destination_space(space);
            std::string bodyStr;
            body.SerializeToString(&bodyStr);
            // Avoids waiting a loop.
            ho->sendViaSpace(header, MemoryReference(bodyStr));

            locationWasSet(toSet);
        }
    }

    typedef std::map<uint32, std::set<ObjectReference> > ProxQueryMap;
    ProxQueryMap mProxQueryMap; ///< indexed by ProxCall::query_id()

    PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream)
        :mSpaceConnection(topLevel,stream),
        mUpdatedLocation(
            Duration::seconds(.1),
            TemporalValue<Location>::Time::null(),
            Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                     Vector3f(0,0,0),Vector3f(0,1,0),0),
            ProxyObject::UpdateNeeded()) {
    }
};


 
HostedObject::HostedObject(ObjectHost*parent, const UUID &objectName)
    : mTracker(parent->getSpaceIO()),
      mInternalObjectReference(objectName) {
    mSpaceData = new SpaceDataMap;
    mObjectHost=parent;
    mObjectScript=NULL;
    mSendService.ho = this;
    mReceiveService.ho = this;
    mTracker.forwardMessagesTo(&mSendService);
}

HostedObject::~HostedObject() {
    if (mObjectScript) {
        delete mObjectScript;
    }
    for (SpaceDataMap::const_iterator iter = mSpaceData->begin();
         iter != mSpaceData->end();
         ++iter) {
        for (PerSpaceData::ProxQueryMap::const_iterator qiter = iter->second.mProxQueryMap.begin();
             qiter != iter->second.mProxQueryMap.end();
             ++qiter) {
            for (std::set<ObjectReference>::const_iterator oriter = qiter->second.begin();
                 oriter != qiter->second.end();
                 ++oriter)
            {
                iter->second.mSpaceConnection.getTopLevelStream()->
                    destroyViewedObject(SpaceObjectReference(iter->first, *oriter), getTracker());
            }
        }
    }
    mObjectHost->unregisterHostedObject(mInternalObjectReference);
    mTracker.endForwardingMessagesTo(&mSendService);
    delete mSpaceData;
}

struct HostedObject::PrivateCallbacks {

    static void initializeDatabaseCallback(
        HostedObject *realThis,
        const SpaceID &spaceID,
        Persistence::SentReadWriteSet *msg,
        const RoutableMessageHeader &lastHeader,
        Persistence::Protocol::Response::ReturnStatus errorCode)
    {
        if (lastHeader.has_return_status() || errorCode) {
            SILOG(cppoh,error,"Database error recieving Loc and scripting info: "<<(int)lastHeader.return_status()<<": "<<(int)errorCode);
            delete msg;
            return; // unable to get starting position.
        }
        String scriptName;
        std::map<String,String> scriptParams;
        Location location(Vector3d::nil(),Quaternion::identity(),Vector3f::nil(),Vector3f(1,0,0),0);
        for (int i = 0; i < msg->body().reads_size(); i++) {
            String name = msg->body().reads(i).field_name();
            if (msg->body().reads(i).has_return_status() || !msg->body().reads(i).has_data()) {
                continue;
            }
            if (!name.empty() && name[0] != '_') {
                realThis->setProperty(name, msg->body().reads(i).data());
            }
            if (name == "Loc") {
                ObjLoc loc;
                loc.ParseFromString(msg->body().reads(i).data());
                SILOG(cppoh,debug,"Creating object "<<ObjectReference(realThis->getUUID())
                      <<" at position "<<loc.position());
                if (loc.has_position()) {
                    location.setPosition(loc.position());
                }
                if (loc.has_orientation()) {
                    location.setOrientation(loc.orientation());
                }
                if (loc.has_velocity()) {
                    location.setVelocity(loc.velocity());
                }
                if (loc.has_rotational_axis()) {
                    location.setAxisOfRotation(loc.rotational_axis());
                }
                if (loc.has_angular_speed()) {
                    location.setAngularSpeed(loc.angular_speed());
                }
            }
            if (name == "_Script") {
                Protocol::StringProperty scrProp;
                scrProp.ParseFromString(msg->body().reads(i).data());
                scriptName = scrProp.value();
            }
            if (name == "_ScriptParams") {
                Protocol::StringMapProperty scrProp;
                scrProp.ParseFromString(msg->body().reads(i).data());
                int numkeys = scrProp.keys_size();
                {
                    int numvalues = scrProp.values_size();
                    if (numvalues < numkeys) {
                        numkeys = numvalues;
                    }
                }
                for (int i = 0; i < numkeys; i++) {
                    scriptParams[scrProp.keys(i)] = scrProp.values(i);
                }
            }
        }
        // Temporary Hack because we do not have access to the CDN here.
        BoundingSphere3f sphere(Vector3f::nil(),1);
        realThis->sendNewObj(location, sphere, spaceID);
        delete msg;
        if (!scriptName.empty()) {
            realThis->initializeScript(scriptName, scriptParams);
        }
        realThis->mObjectHost->getWorkQueue()->dequeueAll();
    }

    static void receivedRoutableMessage(const HostedObjectWPtr&thus,const SpaceID&sid, const Network::Chunk&msgChunk) {
        HostedObjectPtr realThis (thus.lock());

        RoutableMessageHeader header;
        MemoryReference bodyData = header.ParseFromArray(&(msgChunk[0]),msgChunk.size());
        header.set_source_space(sid);
        header.set_destination_space(sid);
        {
            ProxyObjectPtr destinationObject = realThis->getProxy(header.source_space());
            if (destinationObject) {
                header.set_destination_object(destinationObject->getObjectReference().object());
            }
            if (!header.has_source_object()) {
                header.set_source_object(ObjectReference::spaceServiceID());
            }
        }

        if (!realThis) {
            SILOG(objecthost,error,"Received message for dead HostedObject. SpaceID = "<<sid<<"; DestObject = "<<header.destination_object());
            return;
        }

        realThis->processRoutableMessage(header, bodyData);
    }

    static void handlePersistenceResponse(
        HostedObject *realThis,
        const RoutableMessageHeader &origHeader,
        SentMessage *sent,
        const RoutableMessageHeader &header,
        MemoryReference bodyData)
    {
        std::auto_ptr<SentMessageBody<Persistence::Protocol::ReadWriteSet> > sentDestruct(static_cast<SentMessageBody<Persistence::Protocol::ReadWriteSet> *>(sent));
        SILOG(cppoh,debug,"Got some persistence back: stat = "<<(int)header.return_status());
        if (header.has_return_status()) {
            Persistence::Protocol::Response resp;
            for (int i = 0, respIndex=0; i < sentDestruct->body().reads_size(); i++, respIndex++) {
                Persistence::Protocol::IStorageElement field = resp.add_reads();
                if (sentDestruct->body().reads(i).has_index()) {
                    field.set_index(sentDestruct->body().reads(i).index());
                }
                if (sentDestruct->body().options() & Persistence::Protocol::ReadWriteSet::RETURN_READ_NAMES) {
                    field.set_field_name(sentDestruct->body().reads(i).field_name());
                }
                field.set_return_status(Persistence::Protocol::StorageElement::KEY_MISSING);
            }
            std::string errorData;
            resp.SerializeToString(&errorData);
            realThis->sendReply(origHeader, MemoryReference(errorData));
        } else {
            realThis->sendReply(origHeader, bodyData);
        }
        realThis->mObjectHost->getWorkQueue()->dequeueAll();
    }
    static void handlePersistenceMessage(HostedObject *realThis, const RoutableMessageHeader &header, MemoryReference bodyData) {
        using namespace Persistence::Protocol;

        ReadWriteSet rws;
        rws.ParseFromArray(bodyData.data(), bodyData.length());

        Response immedResponse;
        int immedIndex = 0;

        SentMessageBody<ReadWriteSet> *persistenceMsg = new SentMessageBody<ReadWriteSet>(&realThis->mTracker,std::tr1::bind(&handlePersistenceResponse, realThis, header, _1, _2, _3));
        int outIndex = 0;
        ReadWriteSet &outMessage = persistenceMsg->body();
        if (rws.has_options()) {
            outMessage.set_options(rws.options());
        }
        SILOG(cppoh,debug,"Got a Persistence message: reads size = "<<rws.reads_size()<<
              " writes size = "<<rws.writes_size());

        for (int i = 0, rwsIndex=0 ; i < rws.reads_size(); i++, rwsIndex++) {
            if (rws.reads(i).has_index()) {
                rwsIndex = rws.reads(i).index();
            }
            std::string name;
            if (rws.reads(i).has_field_name()) {
                name = rws.reads(i).field_name();
            }
            bool fail = false;
            if (name.empty() || name[0] == '_') {
                SILOG(cppoh,debug,"Invalid GetProp: "<<name);
                fail = true;
            } else {
                if (realThis->hasProperty(name)) {
                    // Cached property--respond immediately.
                    SILOG(cppoh,debug,"Cached GetProp: "<<name<<" = "<<realThis->getProperty(name));
                    IStorageElement el = immedResponse.add_reads();
                    if (immedIndex != rwsIndex) {
                        el.set_index(rwsIndex);
                    }
                    immedIndex = rwsIndex+1;
                    if (rws.options() & ReadWriteSet::RETURN_READ_NAMES) {
                        el.set_field_name(rws.reads(i).field_name());
                    }
                    el.set_data(realThis->getProperty(name));
                } else {
                    SILOG(cppoh,debug,"Forward GetProp: "<<name<<" to Persistence");
                    IStorageElement el = outMessage.add_reads();
                    if (outIndex != rwsIndex) {
                        el.set_index(rwsIndex);
                    }
                    outIndex = rwsIndex+1;
                    el.set_field_name(rws.reads(i).field_name());
                    el.set_object_uuid(realThis->getUUID());
                }
            }
            if (fail) {
                IStorageElement el = immedResponse.add_reads();
                if (immedIndex != rwsIndex) {
                    el.set_index(rwsIndex);
                }
                immedIndex = rwsIndex+1;
                if (rws.options() & ReadWriteSet::RETURN_READ_NAMES) {
                    el.set_field_name(rws.reads(i).field_name());
                }
                el.set_return_status(StorageElement::KEY_MISSING);
            }
        }
        outIndex = 0;
        for (int i = 0, rwsIndex=0 ; i < rws.writes_size(); i++, rwsIndex++) {
            if (rws.writes(i).has_index()) {
                rwsIndex = rws.writes(i).index();
            }
            std::string name;
            if (rws.writes(i).has_field_name()) {
                name = rws.writes(i).field_name();
            }
            bool fail = false;
            if (name.empty() || name[0] == '_') {
                SILOG(cppoh,debug,"Invalid SetProp: "<<name);
                fail = true;
            } else {
                if (rws.writes(i).has_data()) {
                    realThis->setProperty(name, rws.writes(i).data());
                    SpaceDataMap::iterator iter;
                    for (iter = realThis->mSpaceData->begin();
                         iter != realThis->mSpaceData->end();
                         ++iter) {
                        realThis->receivedPropertyUpdate(iter->second.mProxyObject, name, rws.writes(i).data());
                    }
                } else {
                    if (name != "LightInfo" && name != "MeshURI" && name != "IsCamera") {
                        // changing the type of this object has to wait until we reload from database.
                        realThis->unsetProperty(name);
                    }
                }
                SILOG(cppoh,debug,"Forward SetProp: "<<name<<" to Persistence");
                IStorageElement el = outMessage.add_writes();
                if (outIndex != rwsIndex) {
                    el.set_index(rwsIndex);
                }
                outIndex = rwsIndex+1;
                el.set_field_name(rws.writes(i).field_name());
                if (rws.writes(i).has_data()) {
                    el.set_data(rws.writes(i).data());
                }
                el.set_object_uuid(realThis->getUUID());
            }
            // what to do if a write fails?
        }

        if (immedResponse.reads_size()) {
            SILOG(cppoh,debug,"ImmedResponse: "<<immedResponse.reads_size());
            std::string respStr;
            immedResponse.SerializeToString(&respStr);
            RoutableMessageHeader respHeader (header);
            realThis->sendReply(respHeader, MemoryReference(respStr));
        }
        if (outMessage.reads_size() || outMessage.writes_size()) {
            SILOG(cppoh,debug,"ForwardToPersistence: "<<outMessage.reads_size()<<
                  " reads and "<<outMessage.writes_size()<<"writes");
            persistenceMsg->header().set_destination_space(SpaceID::null());
            persistenceMsg->header().set_destination_object(ObjectReference::spaceServiceID());
            persistenceMsg->header().set_destination_port(Services::PERSISTENCE);

            persistenceMsg->serializeSend();
        } else {
            delete persistenceMsg;
        }
        realThis->mObjectHost->getWorkQueue()->dequeueAll();
    }
    static void handleRPCMessage(HostedObject *realThis, const RoutableMessageHeader &header, MemoryReference bodyData) {
        /// Parse message_names and message_arguments.

        RoutableMessageBody msg;
        msg.ParseFromArray(bodyData.data(), bodyData.length());
        int numNames = msg.message_size();
        if (numNames <= 0) {
            // Invalid message!
            realThis->sendErrorReply(header, RoutableMessageHeader::PROTOCOL_ERROR);
            return;
        }

        RoutableMessageBody responseMessage;
        for (int i = 0; i < numNames; ++i) {
            std::string name = msg.message_names(i);
            MemoryReference body(msg.message_arguments(i));

            if (header.has_id()) {
                std::string response;
                /// Pass response parameter if we expect a response.
                realThis->processRPC(header, name, body, &response);
                responseMessage.add_message_reply(response);
            } else {
                /// Return value not needed.
                realThis->processRPC(header, name, body, NULL);
            }
        }

        if (header.has_id()) {
            std::string serializedResponse;
            responseMessage.SerializeToString(&serializedResponse);
            realThis->sendReply(header, MemoryReference(serializedResponse));
        }
    }

    static void receivedProxObjectLocation(
        const HostedObjectWPtr &weakThis,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData,
        int32 queryId)
    {
        std::auto_ptr<RPCMessage> destructor(static_cast<RPCMessage*>(sentMessage));
        HostedObjectPtr realThis(weakThis.lock());
        if (!realThis) {
            return;
        }

        RoutableMessage responseMessage(hdr, bodyData.data(), bodyData.length());
        if (responseMessage.header().return_status() != RoutableMessageHeader::SUCCESS) {
            SILOG(cppoh,info,"FAILURE receiving prox object properties "<<sentMessage->getRecipient());
            return;
        }
        ObjLoc objLoc;
        objLoc.ParseFromString(responseMessage.body().message_arguments(0));

        Persistence::SentReadWriteSet *request = new Persistence::SentReadWriteSet(&realThis->mTracker);

        request->header().set_destination_space(sentMessage->getSpace());
        request->header().set_destination_object(sentMessage->getRecipient());
        request->header().set_destination_port(Services::PERSISTENCE);

        request->body().add_reads().set_field_name("MeshURI");
        request->body().add_reads().set_field_name("MeshScale");
        request->body().add_reads().set_field_name("Name");
        request->body().add_reads().set_field_name("PhysicalParameters");
        request->body().add_reads().set_field_name("LightInfo");
        request->body().add_reads().set_field_name("_Passwd");
        request->body().add_reads().set_field_name("IsCamera");
        request->body().add_reads().set_field_name("Parent");
        request->setPersistenceCallback(std::tr1::bind(&PrivateCallbacks::receivedProxObjectProperties,
                                            weakThis, _1, _2, _3,
                                            queryId, objLoc));
        request->setTimeout(Duration::seconds(5.0));
        request->serializeSend();
    }
    static void receivedProxObjectProperties(
        const HostedObjectWPtr &weakThis,
        SentMessage* sentMessageBase,
        const RoutableMessageHeader &hdr,
        Persistence::Protocol::Response::ReturnStatus returnStatus,
        int32 queryId,
        const ObjLoc &objLoc)
    {
        using namespace Persistence::Protocol;
        std::auto_ptr<Persistence::SentReadWriteSet> sentMessage(Persistence::SentReadWriteSet::cast_sent_message(sentMessageBase));
        HostedObjectPtr realThis(weakThis.lock());
        if (!realThis) {
            return;
        }
        SpaceDataMap::iterator iter = realThis->mSpaceData->find(sentMessage->getSpace());
        if (iter == realThis->mSpaceData->end()) {
            return;
        }
        ObjectHostProxyManager *proxyMgr = iter->second.mSpaceConnection.getTopLevelStream().get();
        PerSpaceData::ProxQueryMap::iterator qmiter = iter->second.mProxQueryMap.find(queryId);
        if (qmiter == iter->second.mProxQueryMap.end()) {
            return;
        }
        std::set<ObjectReference>::iterator proxyiter = qmiter->second.find(sentMessage->getRecipient());
        if (proxyiter == qmiter->second.end()) {
            return;
        }
        SpaceObjectReference proximateObjectId(sentMessage->getSpace(), sentMessage->getRecipient());
        bool persistence_error = false;
        if (hdr.return_status() != RoutableMessageHeader::SUCCESS ||returnStatus) {
            SILOG(cppoh,info,"FAILURE receiving prox object properties "<<
                  proximateObjectId.object()<<": Error = "<<(int)hdr.return_status());
            persistence_error = true;
        }
        SILOG(cppoh,debug,"Received prox object properties "<<proximateObjectId.object()<<": reads size = ?");
        int index = 0;
        bool haveAll = true;
        for (int i = 0; i < sentMessage->body().reads_size(); i++) {
            if (!sentMessage->body().reads(i).has_data() && !sentMessage->body().reads(i).has_return_status()) {
                haveAll = false;
            }
        }
        if (persistence_error || haveAll) {

        } else {
            assert(false&&"Received incomplete persistence callback for the same read/write set: should only ever get one");
            return; // More messages would have come before the new design.
        }
        bool hasMesh=false;
        bool hasLight=false;
        bool isCamera=false;
        ProxyObjectPtr proxyObj;
        for (int i = 0; i < sentMessage->body().reads_size(); ++i) {
            if (sentMessage->body().reads(i).has_return_status()) {
                continue;
            }
            const std::string &field = sentMessage->body().reads(i).field_name();
            if (field == "MeshURI") {
                hasMesh = true;
            }
            if (field == "LightInfo") {
                hasLight = true;
            }
            if (field == "IsCamera") {
                isCamera = true;
            }
        }
        ObjectReference myObjectReference;
        if (isCamera) {
            SILOG(cppoh,info, "* I found a camera named " << proximateObjectId.object());
            proxyObj = ProxyObjectPtr(new ProxyCameraObject(proxyMgr, proximateObjectId));
        } else if (hasLight && !hasMesh) {
            SILOG(cppoh,info, "* I found a light named " << proximateObjectId.object());
            proxyObj = ProxyObjectPtr(new ProxyLightObject(proxyMgr, proximateObjectId));
        } else {
            SILOG(cppoh,info, "* I found a MESH named " << proximateObjectId.object());
            proxyObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, proximateObjectId));
        }
        proxyObj->setLocal(false);
        realThis->receivedPositionUpdate(proxyObj, objLoc, true);
        proxyMgr->createViewedObject(proxyObj, realThis->getTracker());
        for (int i = 0; i < sentMessage->body().reads_size(); ++i) {
            if (sentMessage->body().reads(i).has_return_status()) {
                continue;
            }
            const std::string &field = sentMessage->body().reads(i).field_name();
            realThis->receivedPropertyUpdate(proxyObj, sentMessage->body().reads(i).field_name(), sentMessage->body().reads(i).data());
        }
        {
            RPCMessage *request = new RPCMessage(&realThis->mTracker,std::tr1::bind(&receivedPositionUpdateResponse, weakThis, _1, _2, _3));
            request->header().set_destination_space(proximateObjectId.space());
            request->header().set_destination_object(proximateObjectId.object());
            Protocol::LocRequest loc;
            loc.SerializeToString(request->body().add_message("LocRequest"));
            request->serializeSend();
        }
        return;
    }

    static void receivedPositionUpdateResponse(
        const HostedObjectWPtr &weakThus,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData)
    {
        RoutableMessage responseMessage(hdr, bodyData.data(), bodyData.length());
        HostedObjectPtr thus(weakThus.lock());
        if (!thus) {
            delete static_cast<RPCMessage*>(sentMessage);
        }
        if (responseMessage.header().return_status()) {
            return;
        }
        Protocol::ObjLoc loc;
        loc.ParseFromString(responseMessage.body().message_arguments(0));

        const SpaceID &space = sentMessage->getSpace();
        ProxyManager *pm = thus->getObjectHost()->getProxyManager(space);
        if (pm) {
            ProxyObjectPtr obj(pm->getProxyObject(
                SpaceObjectReference(space, sentMessage->getRecipient())));
            if (obj) {
                thus->receivedPositionUpdate(obj,loc,false);
            }
        }

        static_cast<RPCMessage*>(sentMessage)->serializeSend(); // Resend position update each time we get one.
    }

    static void disconnectionEvent(const HostedObjectWPtr&weak_thus,const SpaceID&sid, const String&reason) {
        std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
        if (thus) {
            SpaceDataMap::iterator where=thus->mSpaceData->find(sid);
            if (where!=thus->mSpaceData->end()) {
                thus->mSpaceData->erase(where);//FIXME do we want to back this up to the database first?
            }
        }
    }

    static void connectionEvent(const HostedObjectWPtr&thus,
                                const SpaceID&sid,
                                Network::Stream::ConnectionStatus ce,
                                const String&reason) {
        if (ce!=Network::Stream::Connected) {
            disconnectionEvent(thus,sid,reason);
        }
    }
};

HostedObject::PerSpaceData& HostedObject::cloneTopLevelStream(const SpaceID&sid,const std::tr1::shared_ptr<TopLevelSpaceConnection>&tls) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    SpaceDataMap::iterator iter = mSpaceData->insert(
        SpaceDataMap::value_type(
            sid,
            PerSpaceData(tls,
                         tls->topLevelStream()->clone(
                             std::tr1::bind(&PrivateCallbacks::connectionEvent,
                                            getWeakPtr(),
                                            sid,
                                            _1,
                                            _2),
                             std::tr1::bind(&PrivateCallbacks::receivedRoutableMessage,
                                            getWeakPtr(),
                                            sid,
                                            _1))))).first;
    return iter->second;
}

static String nullProperty;
bool HostedObject::hasProperty(const String &propName) const {
    PropertyMap::const_iterator iter = mProperties.find(propName);
    return (iter != mProperties.end());
}
const String &HostedObject::getProperty(const String &propName) const {
    PropertyMap::const_iterator iter = mProperties.find(propName);
    if (iter != mProperties.end()) {
        return (*iter).second;
    }
    return nullProperty;
}
String *HostedObject::propertyPtr(const String &propName) {
    return &(mProperties[propName]);
}
void HostedObject::setProperty(const String &propName, const String &encodedValue) {
    mProperties.insert(PropertyMap::value_type(propName, encodedValue));
}
void HostedObject::unsetProperty(const String &propName) {
    PropertyMap::iterator iter = mProperties.find(propName);
    if (iter != mProperties.end()) {
        mProperties.erase(iter);
    }
}


static ProxyObjectPtr nullPtr;
const ProxyObjectPtr &HostedObject::getProxy(const SpaceID &space) const {
    SpaceDataMap::const_iterator iter = mSpaceData->find(space);
    if (iter == mSpaceData->end()) {
        return nullPtr;
    }
    return iter->second.mProxyObject;
}


using Sirikata::Protocol::NewObj;
using Sirikata::Protocol::IObjLoc;

void HostedObject::sendNewObj(
    const Location&startingLocation,
    const BoundingSphere3f &meshBounds,
    const SpaceID&spaceID)
{

    RoutableMessageHeader messageHeader;
    messageHeader.set_destination_object(ObjectReference::spaceServiceID());
    messageHeader.set_destination_space(spaceID);
    messageHeader.set_destination_port(Services::REGISTRATION);
    NewObj newObj;
    newObj.set_object_uuid_evidence(getUUID());
    newObj.set_bounding_sphere(meshBounds);
    IObjLoc loc = newObj.mutable_requested_object_loc();
    loc.set_timestamp(Time::now(getSpaceTimeOffset(spaceID)));
    loc.set_position(startingLocation.getPosition());
    loc.set_orientation(startingLocation.getOrientation());
    loc.set_velocity(startingLocation.getVelocity());
    loc.set_rotational_axis(startingLocation.getAxisOfRotation());
    loc.set_angular_speed(startingLocation.getAngularSpeed());

    RoutableMessageBody messageBody;
    newObj.SerializeToString(messageBody.add_message("NewObj"));

    std::string serializedBody;
    messageBody.SerializeToString(&serializedBody);
    sendViaSpace(messageHeader, MemoryReference(serializedBody));
}

void HostedObject::initializeConnect(
    const Location&startingLocation,
    const String&mesh, const BoundingSphere3f&meshBounds,
    const LightInfo *lightInfo,
    const SpaceID&spaceID, const HostedObjectPtr&spaceConnectionHint)
{
    mObjectHost->registerHostedObject(getSharedPtr());
    connectToSpace(spaceID, spaceConnectionHint);
    sendNewObj(startingLocation, meshBounds, spaceID);
    mObjectHost->getWorkQueue()->dequeueAll(); // don't need to wait until next frame.

    if (!mesh.empty()) {
        Protocol::StringProperty meshprop;
        meshprop.set_value(mesh);
        meshprop.SerializeToString(propertyPtr("MeshURI"));
        Protocol::Vector3fProperty scaleprop;
        scaleprop.set_value(Vector3f(1,1,1)); // default value, set it manually if you want different.
        scaleprop.SerializeToString(propertyPtr("MeshScale"));
        Protocol::PhysicalParameters physicalprop;
        physicalprop.set_mode(Protocol::PhysicalParameters::NONPHYSICAL);
        physicalprop.SerializeToString(propertyPtr("PhysicalParameters"));
    } else if (lightInfo) {
        Protocol::LightInfoProperty lightProp;
        lightInfo->toProtocol(lightProp);
        lightProp.SerializeToString(propertyPtr("LightInfo"));
    } else {
        setProperty("IsCamera");
    }
}
void HostedObject::initializePythonScript() {
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getDefaultConstructor()("");
    if (mgr) {
        ObjectScriptManager::Arguments args;
        args["Assembly"]="Sirikata.Runtime";
        args["Class"]="PythonObject";
        args["Namespace"]="Sirikata.Runtime";
        args["PythonModule"]="test";
        args["PythonClass"]="exampleclass";

        mObjectScript=mgr->createObjectScript(this,args);
        if (mObjectScript) {
            mObjectScript->tick();
        }
    }
}

void HostedObject::initializeRestoreFromDatabase(const SpaceID&spaceID, const HostedObjectPtr&spaceConnectionHint) {
    mObjectHost->registerHostedObject(getSharedPtr());
    connectToSpace(spaceID, spaceConnectionHint);

    Persistence::SentReadWriteSet *msg;
    msg = new Persistence::SentReadWriteSet(&mTracker);
    msg->setPersistenceCallback(std::tr1::bind(
                         &PrivateCallbacks::initializeDatabaseCallback,
                         this, spaceID,
                         _1, _2, _3));
    msg->body().add_reads().set_field_name("MeshURI");
    msg->body().add_reads().set_field_name("MeshScale");
    msg->body().add_reads().set_field_name("Name");
    msg->body().add_reads().set_field_name("PhysicalParameters");
    msg->body().add_reads().set_field_name("LightInfo");
    msg->body().add_reads().set_field_name("IsCamera");
    msg->body().add_reads().set_field_name("Parent");
    msg->body().add_reads().set_field_name("Loc");
    msg->body().add_reads().set_field_name("_Script");
    msg->body().add_reads().set_field_name("_ScriptParams");
    for (int i = 0; i < msg->body().reads_size(); i++) {
        msg->body().reads(i).set_object_uuid(getUUID()); // database assumes uuid 0 if omitted
    }
    msg->header().set_destination_object(ObjectReference::spaceServiceID());
    msg->header().set_destination_port(Services::PERSISTENCE);
    msg->serializeSend();
    mObjectHost->getWorkQueue()->dequeueAll(); // don't need to wait until next frame.
}
void HostedObject::initializeScript(const String& script, const ObjectScriptManager::Arguments &args) {
    assert(!mObjectScript); // Don't want to kill a live script!
    mObjectHost->registerHostedObject(getSharedPtr());
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getConstructor(script)("");
    if (mgr) {
        mObjectScript = mgr->createObjectScript(this, args);
    }
}
void HostedObject::connectToSpace(const SpaceID&id,const HostedObjectPtr&spaceConnectionHint) {
    if (id!=SpaceID::null()) {
        //bind script to object...script might be a remote ID, so need to bind download target, etc
        std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection;
        SpaceDataMap::iterator where;
        if (spaceConnectionHint&&(where=spaceConnectionHint->mSpaceData->find(id))!=mSpaceData->end()) {
            topLevelConnection=where->second.mSpaceConnection.getTopLevelStream();
        }else {
            topLevelConnection=mObjectHost->connectToSpace(id);
        }

        // sending initial packet is done by the script!
        //conn->send(initializationPacket,Network::ReliableOrdered);
        PerSpaceData &psd = cloneTopLevelStream(id,topLevelConnection);
        // return &(psd.mSpaceConnection);
    }
}

void HostedObject::processRoutableMessage(const RoutableMessageHeader &header, MemoryReference bodyData) {
    std::string myself_name;
    {
        std::ostringstream os;
        if (header.has_destination_object()) {
            os << header.destination_object();
        } else {
            os << "[Temporary UUID " << mInternalObjectReference.toString() << "]";
        }
        myself_name = os.str();
    }
    {
        std::ostringstream os;
        os << "** Message from: " << header.source_object() << " port " << header.source_port() << " to "<<myself_name<<" port " << header.destination_port();
        SILOG(cppoh,debug,os.str());
    }
    /// Handle Return values to queries we sent to someone:
    if (header.has_reply_id()) {
        mTracker.processMessage(header, bodyData);
        return; // Not a message for us to process.
    }

    if (header.destination_port() == 0) {
        PrivateCallbacks::handleRPCMessage(this, header, bodyData);
    } else if (header.destination_port() == Services::PERSISTENCE) {
        PrivateCallbacks::handlePersistenceMessage(this, header, bodyData);
    } else {
        if (mObjectScript) {
            mObjectScript->processMessage(header, bodyData);
        } else {
            sendErrorReply(header, RoutableMessageHeader::PORT_FAILURE);
        }
    }
}

void HostedObject::sendViaSpace(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    ///// MessageService::processMessage
    assert(hdrOrig.has_destination_object());
    assert(hdrOrig.has_destination_space());
    SpaceDataMap::iterator where=mSpaceData->find(hdrOrig.destination_space());
    if (where!=mSpaceData->end()) {
        RoutableMessageHeader hdr (hdrOrig);
        hdr.clear_destination_space();
        hdr.clear_source_space();
        hdr.clear_source_object();
        String serialized_header;
        hdr.SerializeToString(&serialized_header);
        where->second.mSpaceConnection.getStream()->send(MemoryReference(serialized_header),body, Network::ReliableOrdered);
    }
    assert(where!=mSpaceData->end());
}

void HostedObject::send(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    assert(hdrOrig.has_destination_object());
    if (!hdrOrig.has_destination_space() || hdrOrig.destination_space() == SpaceID::null()) {
        RoutableMessageHeader hdr (hdrOrig);
        hdr.set_destination_space(SpaceID::null());
        hdr.set_source_object(ObjectReference(mInternalObjectReference));
        mObjectHost->processMessage(hdr, body);
        return;
    }
    SpaceDataMap::iterator where=mSpaceData->find(hdrOrig.destination_space());
    if (where!=mSpaceData->end()) {
        const ProxyObjectPtr &obj = where->second.mProxyObject;
        if (obj) {
            RoutableMessageHeader hdr (hdrOrig);
            hdr.set_source_object(obj->getObjectReference().object());
            mObjectHost->processMessage(hdr, body);
        } else {
            sendViaSpace(hdrOrig, body);
        }
    }
}

void HostedObject::tick() {
    for (SpaceDataMap::iterator iter = mSpaceData->begin(); iter != mSpaceData->end(); ++iter) {
        // send update to LOC (2) service in the space, if necessary
        iter->second.updateLocation(this);
        // Is it useful to call every script's tick() function?
    }
}

void HostedObject::receivedPositionUpdate(
    const ProxyObjectPtr &proxy,
    const ObjLoc &objLoc,
    bool force_reset)
{
    if (!proxy) {
        return;
    }
    force_reset = force_reset || (objLoc.update_flags() & ObjLoc::FORCE);
    if (!objLoc.has_timestamp()) {
        objLoc.set_timestamp(Time::now(getSpaceTimeOffset(proxy->getObjectReference().space())));
    }
    Location currentLoc = proxy->globalLocation(objLoc.timestamp());
    if (force_reset || objLoc.has_position()) {
        currentLoc.setPosition(objLoc.position());
    }
    if (force_reset || objLoc.has_orientation()) {
        currentLoc.setOrientation(objLoc.orientation());
    }
    if (force_reset || objLoc.has_velocity()) {
        currentLoc.setVelocity(objLoc.velocity());
    }
    if (force_reset || objLoc.has_rotational_axis()) {
        currentLoc.setAxisOfRotation(objLoc.rotational_axis());
    }
    if (force_reset || objLoc.has_angular_speed()) {
        currentLoc.setAngularSpeed(objLoc.angular_speed());
    }
    if (force_reset) {
        proxy->resetLocation(objLoc.timestamp(), currentLoc);
    } else {
        std::ostringstream os;
        os << "Received position update to "<<currentLoc;
        SILOG(cppoh,debug,os.str());
        proxy->setLocation(objLoc.timestamp(), currentLoc);
    }
}


static int32 query_id = 0;
using Protocol::LocRequest;
void HostedObject::processRPC(const RoutableMessageHeader &msg, const std::string &name, MemoryReference args, String *response) {
    std::ostringstream printstr;
    printstr<<"\t";
    ProxyObjectPtr thisObj = getProxy(msg.source_space());

    if (name == "LocRequest") {
        LocRequest query;
        printstr<<"LocRequest: ";
        query.ParseFromArray(args.data(), args.length());
        ObjLoc loc;
        Time now = Time::now(getSpaceTimeOffset(msg.source_space()));
        if (thisObj) {
            Location globalLoc = thisObj->globalLocation(now);
            loc.set_timestamp(now);
            uint32 fields = 0;
            bool all_fields = true;
            if (query.has_requested_fields()) {
                fields = query.requested_fields();
                all_fields = false;
            }
            if (all_fields || (fields & LocRequest::POSITION))
                loc.set_position(globalLoc.getPosition());
            if (all_fields || (fields & LocRequest::ORIENTATION))
                loc.set_orientation(globalLoc.getOrientation());
            if (all_fields || (fields & LocRequest::VELOCITY))
                loc.set_velocity(globalLoc.getVelocity());
            if (all_fields || (fields & LocRequest::ROTATIONAL_AXIS))
                loc.set_rotational_axis(globalLoc.getAxisOfRotation());
            if (all_fields || (fields & LocRequest::ANGULAR_SPEED))
                loc.set_angular_speed(globalLoc.getAngularSpeed());
            if (response)
                loc.SerializeToString(response);
        } else {
            SILOG(objecthost, error, "LocRequest message not for any known object.");
        }
        // loc requests need to be fast, unlikely to land in infinite recursion.
        mObjectHost->getWorkQueue()->dequeueAll();
        return;             /// comment out if we want scripts to see these requests
    }
    else if (name == "SetLoc") {
        ObjLoc setloc;
        printstr<<"Someone wants to set my position: ";
        setloc.ParseFromArray(args.data(), args.length());
        if (thisObj) {
            printstr<<setloc.position();
            receivedPositionUpdate(thisObj, setloc, false);
        }
    }
    else if (name == "DelObj") {
        SpaceDataMap::iterator perSpaceIter = mSpaceData->find(msg.source_space());
        if (perSpaceIter == mSpaceData->end()) {
            SILOG(objecthost, error, "DelObj message not for any known space.");
            return;
        }
        TopLevelSpaceConnection *proxyMgr =
            perSpaceIter->second.mSpaceConnection.getTopLevelStream().get();
        if (thisObj && proxyMgr) {
            proxyMgr->unregisterHostedObject(thisObj->getObjectReference().object());
        }
    }
    else if (name == "RetObj") {
        SpaceDataMap::iterator perSpaceIter = mSpaceData->find(msg.source_space());
        if (msg.source_object() != ObjectReference::spaceServiceID()) {
            SILOG(objecthost, error, "RetObj message not coming from space: "<<msg.source_object());
            return;
        }
        if (perSpaceIter == mSpaceData->end()) {
            SILOG(objecthost, error, "RetObj message not for any known space.");
            return;
        }
        // getProxyManager() does not work because we have not yet created our ProxyObject.
        TopLevelSpaceConnection *proxyMgr =
            perSpaceIter->second.mSpaceConnection.getTopLevelStream().get();

        Protocol::RetObj retObj;
        retObj.ParseFromArray(args.data(), args.length());
        if (retObj.has_object_reference() && retObj.has_location()) {
            SpaceObjectReference objectId(msg.source_space(), ObjectReference(retObj.object_reference()));
            ProxyObjectPtr proxyObj;
            if (hasProperty("IsCamera")) {
                printstr<<"RetObj: I am now a Camera known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyCameraObject(proxyMgr, objectId));
            } else if (hasProperty("LightInfo") && !hasProperty("MeshURI")) {
                printstr<<"RetObj. I am now a Light known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyLightObject(proxyMgr, objectId));
            } else {
                printstr<<"RetObj: I am now a Mesh known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, objectId));
            }
            proxyObj->setLocal(true);
            perSpaceIter->second.mProxyObject = proxyObj;
            proxyMgr->registerHostedObject(objectId.object(), getSharedPtr());
            receivedPositionUpdate(proxyObj, retObj.location(), true);
            perSpaceIter->second.locationWasReset(retObj.location().timestamp(), proxyObj->getLastLocation());
            if (proxyMgr) {
                proxyMgr->createObject(proxyObj);
                ProxyCameraObject* cam = dynamic_cast<ProxyCameraObject*>(proxyObj.get());
                if (cam) {
                    /* HACK: Because we have no method of scripting yet, we force
                       any local camera we create to attach for convenience. */
                    cam->attach(String(), 0, 0);
                    uint32 my_query_id = query_id;
                    query_id++;
                    Protocol::NewProxQuery proxQuery;
                    proxQuery.set_query_id(my_query_id);
                    proxQuery.set_max_radius(1.0e+30f);
                    String proxQueryStr;
                    proxQuery.SerializeToString(&proxQueryStr);
                    RoutableMessageBody body;
                    body.add_message("NewProxQuery", proxQueryStr);
                    String bodyStr;
                    body.SerializeToString(&bodyStr);
                    RoutableMessageHeader proxHeader;
                    proxHeader.set_destination_port(Services::GEOM);
                    proxHeader.set_destination_object(ObjectReference::spaceServiceID());
                    proxHeader.set_destination_space(objectId.space());
                    send(proxHeader, MemoryReference(bodyStr));
                }
                for (PropertyMap::const_iterator iter = mProperties.begin();
                        iter != mProperties.end();
                        ++iter) {
                    receivedPropertyUpdate(proxyObj, iter->first, iter->second);
                }
            }
        }
    }
    else if (name == "ProxCall") {
        ObjectHostProxyManager *proxyMgr;
        if (false && msg.source_object() != ObjectReference::spaceServiceID()) {
            SILOG(objecthost, error, "ProxCall message not coming from space: "<<msg.source_object());
            return;
        }
        if (!thisObj) {
            SILOG(objecthost, error, "ProxCall message with null ProxyManager.");
            return;
        }

        SpaceDataMap::iterator sditer = mSpaceData->find(msg.source_space());
        assert (sditer != mSpaceData->end());
        proxyMgr = sditer->second.mSpaceConnection.getTopLevelStream().get();

        Protocol::ProxCall proxCall;
        proxCall.ParseFromArray(args.data(), args.length());
        SpaceObjectReference proximateObjectId (msg.source_space(), ObjectReference(proxCall.proximate_object()));
        ProxyObjectPtr proxyObj (proxyMgr->getProxyObject(proximateObjectId));
        switch (proxCall.proximity_event()) {
          case Protocol::ProxCall::EXITED_PROXIMITY:
            printstr<<"ProxCall EXITED "<<proximateObjectId.object();
            if (proxyObj) {
                PerSpaceData::ProxQueryMap::iterator iter = sditer->second.mProxQueryMap.find(proxCall.query_id());
                if (iter != sditer->second.mProxQueryMap.end()) {
                    std::set<ObjectReference>::iterator proxyiter = iter->second.find(proximateObjectId.object());
                    assert (proxyiter != iter->second.end());
                    if (proxyiter != iter->second.end()) {
                        iter->second.erase(proxyiter);
                    }
                }
                proxyMgr->destroyViewedObject(proxyObj->getObjectReference(), this->getTracker());
            } else {
                printstr<<" (unknown obj)";
            }
            break;
          case Protocol::ProxCall::ENTERED_PROXIMITY:
            printstr<<"ProxCall ENTERED "<<proximateObjectId.object();
            {
                PerSpaceData::ProxQueryMap::iterator iter =
                    sditer->second.mProxQueryMap.insert(
                        PerSpaceData::ProxQueryMap::value_type(proxCall.query_id(), std::set<ObjectReference>())
                        ).first;
                iter->second.insert(proximateObjectId.object());
            }
            if (!proxyObj) { // FIXME: We may get one of these for each prox query. Keep track of in-progress queries in ProxyManager.
                printstr<<" (Requesting information...)";

                {
                    RPCMessage *locRequest = new RPCMessage(&mTracker,std::tr1::bind(&PrivateCallbacks::receivedProxObjectLocation,
                                                                                     getWeakPtr(), _1, _2, _3,
                                                        proxCall.query_id()));
                    locRequest->header().set_destination_space(proximateObjectId.space());
                    locRequest->header().set_destination_object(proximateObjectId.object());
                    LocRequest loc;
                    loc.SerializeToString(locRequest->body().add_message("LocRequest"));

                  
                    locRequest->setTimeout(Duration::seconds(5.0));
                    locRequest->serializeSend();
                }
            } else {
                printstr<<" (Already known)";
                proxyMgr->createViewedObject(proxyObj, this->getTracker());
            }
            break;
          case Protocol::ProxCall::STATELESS_PROXIMITY:
            printstr<<"ProxCall Stateless'ed "<<proximateObjectId.object();
            // Do not create a proxy object in this case: This message is for one-time queries
            break;
        }
    } else {
        printstr<<"Message to be handled in script: "<<name;
    }
    SILOG(cppoh,debug,printstr.str());
    if (mObjectScript) {
        MemoryBuffer returnCopy;
        mObjectScript->processRPC(msg, name, args, returnCopy);
        if (response) {
            response->reserve(returnCopy.size());
            std::copy(returnCopy.begin(), returnCopy.end(),
                      std::insert_iterator<std::string>(*response, response->begin()));
        }
    }
}
const Duration&HostedObject::getSpaceTimeOffset(const SpaceID&space) {
    static Duration nil(Duration::seconds(0));
    SpaceDataMap::iterator where=mSpaceData->find(space);
    if (where!=mSpaceData->end()) 
        return where->second.mSpaceConnection.getTopLevelStream()->getServerTimeOffset();
    return nil;
}

void HostedObject::receivedPropertyUpdate(
        const ProxyObjectPtr &proxy,
        const std::string &propertyName,
        const std::string &arguments)
{
    if (propertyName == "MeshURI") {
        Protocol::StringProperty parsedProperty;
        parsedProperty.ParseFromString(arguments);
        if (parsedProperty.has_value()) {
            ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
            if (proxymesh) {
                SILOG(cppoh,info, "* Received MESH property update for " << proxy->getObjectReference().object() <<" has mesh URI "<<URI(parsedProperty.value()));
                proxymesh->setMesh(URI(parsedProperty.value()));
            }
        }
    }
    if (propertyName == "MeshScale") {
        Protocol::Vector3fProperty parsedProperty;
        parsedProperty.ParseFromString(arguments);
        if (parsedProperty.has_value()) {
            ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
            if (proxymesh) {
                proxymesh->setScale(parsedProperty.value());
            }
        }
    }
    if (propertyName == "LightInfo") {
        Protocol::LightInfoProperty parsedLight;
        parsedLight.ParseFromString(arguments);
        ProxyLightObject *proxylight = dynamic_cast<ProxyLightObject*>(proxy.get());
        if (proxylight) {
            proxylight->update(LightInfo(parsedLight));
        }
    }
    if (propertyName == "PhysicalParameters") {
        Protocol::PhysicalParameters parsedProperty;
        parsedProperty.ParseFromString(arguments);
        ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
        if (proxymesh) {
            // FIXME: allow missing fields, and do not hardcode enum values.
            PhysicalParameters params;
            switch (parsedProperty.mode()) {
            case Protocol::PhysicalParameters::NONPHYSICAL:
                params.mode = PhysicalParameters::Disabled;
                break;
            case Protocol::PhysicalParameters::STATIC:
                params.mode = PhysicalParameters::Static;
                break;
            case Protocol::PhysicalParameters::DYNAMICBOX:
                params.mode = PhysicalParameters::DynamicBox;
                break;
            case Protocol::PhysicalParameters::DYNAMICSPHERE:
                params.mode = PhysicalParameters::DynamicSphere;
                break;
            case Protocol::PhysicalParameters::DYNAMICCYLINDER:
                params.mode = PhysicalParameters::DynamicCylinder;
                break;
            case Protocol::PhysicalParameters::CHARACTER:
                params.mode = PhysicalParameters::Character;
                break;
            default:
                params.mode = PhysicalParameters::Disabled;
            }
            params.density = parsedProperty.density();
            params.friction = parsedProperty.friction();
            params.bounce = parsedProperty.bounce();
            params.colMsg = parsedProperty.collide_msg();
            params.colMask = parsedProperty.collide_mask();
            params.hull = parsedProperty.hull();
            params.gravity = parsedProperty.gravity();
            params.name = proxymesh->getPhysical().name;        /// otherwise setPhysical will wipe it
            proxymesh->setPhysical(params);
        }
    }
    if (propertyName == "Parent") {
        Protocol::ParentProperty parsedProperty;
        parsedProperty.ParseFromString(arguments);
        if (parsedProperty.has_value()) {
            ProxyObjectPtr obj =proxy->getProxyManager()->getProxyObject(
                SpaceObjectReference(
                    proxy->getObjectReference().space(),
                    ObjectReference(parsedProperty.value())));
            if (obj && obj != proxy) {
                proxy->setParent(obj, Time::now(getSpaceTimeOffset(proxy->getObjectReference().space())));
            }
        }
    }
    if (propertyName == "Name") {
        Protocol::StringProperty parsedProperty;
        ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
        parsedProperty.ParseFromString(arguments);
        if (proxymesh && parsedProperty.has_value()) {
            PhysicalParameters params = proxymesh->getPhysical();
            params.name = parsedProperty.value();
            proxymesh->setPhysical(params);
        }
    }
}

}
