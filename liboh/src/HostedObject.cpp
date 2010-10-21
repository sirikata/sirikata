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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/ProxyLightObject.hpp>
#include <sirikata/proxyobject/ProxyWebViewObject.hpp>
#include <sirikata/proxyobject/ProxyCameraObject.hpp>
#include <sirikata/proxyobject/LightInfo.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/oh/ObjectHost.hpp>

#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>

#include <sirikata/core/network/Frame.hpp>

#include "Protocol_Frame.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"

#define HO_LOG(lvl,msg) SILOG(ho,lvl,"[HO] " << msg);

namespace Sirikata {

class HostedObject::PerSpaceData {
public:
    HostedObject* parent;
    SpaceID space;
    ObjectReference object;
    ProxyObjectPtr mProxyObject;
    ProxyObject::Extrapolator mUpdatedLocation;

    ObjectHostProxyManagerPtr proxyManager;

    SpaceObjectReference id() const { return SpaceObjectReference(space, object); }

    PerSpaceData(HostedObject* _parent, const SpaceID& _space)
     : parent(_parent),
       space(_space),
       object(ObjectReference::null()),
       mUpdatedLocation(
            Duration::seconds(.1),
            TemporalValue<Location>::Time::null(),
            Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                     Vector3f(0,0,0),Vector3f(0,1,0),0),
            ProxyObject::UpdateNeeded()),
       proxyManager(new ObjectHostProxyManager(_space))
    {
    }

    void initializeAs(ProxyObjectPtr proxyobj) {
        object = proxyobj->getObjectReference().object();

        mProxyObject = proxyobj;
    }
};



HostedObject::HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID &objectName, bool is_camera)
 : mContext(ctx),
   mInternalObjectReference(objectName),
   mIsCamera(is_camera)
{
    mSpaceData = new SpaceDataMap;
    mNextSubscriptionID = 0;
    mObjectHost=parent;
    mObjectScript=NULL;

    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &HostedObject::createDelegateODPPort, this,
            _1, _2, _3
        )
    );
}

HostedObject::~HostedObject() {
    destroy();
    delete mSpaceData;
}

void HostedObject::init() {
    mObjectHost->registerHostedObject(getSharedPtr());
}

void HostedObject::destroy() {
    if (mObjectScript) {
        delete mObjectScript;
        mObjectScript=NULL;
    }
    mSpaceData->clear();
    mObjectHost->unregisterHostedObject(mInternalObjectReference);
}

struct HostedObject::PrivateCallbacks {
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

Time HostedObject::spaceTime(const SpaceID& space, const Time& t) {
    return t + mObjectHost->serverTimeOffset(space);
}

Time HostedObject::currentSpaceTime(const SpaceID& space) {
    return spaceTime(space, mContext->simTime());
}

Time HostedObject::localTime(const SpaceID& space, const Time& t) {
    return t + mObjectHost->clientTimeOffset(space);
}

Time HostedObject::currentLocalTime() {
    return mContext->simTime();
}


ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space) {
    SpaceDataMap::const_iterator it = mSpaceData->find(space);
    if (it == mSpaceData->end()) {
        it = mSpaceData->insert(
            SpaceDataMap::value_type( space, PerSpaceData(this, space) )
        ).first;
    }
    return it->second.proxyManager;
}


static ProxyObjectPtr nullPtr;
const ProxyObjectPtr &HostedObject::getProxy(const SpaceID &space) const {
    SpaceDataMap::const_iterator iter = mSpaceData->find(space);
    if (iter == mSpaceData->end()) {
        return nullPtr;
    }
    return iter->second.mProxyObject;
}

ProxyObjectPtr HostedObject::getProxy(const SpaceID& space, const ObjectReference& oref)
{
    ProxyManagerPtr proxy_manager = getProxyManager(space);
    ProxyObjectPtr  proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));
    return proxy_obj;
}

namespace {
bool myisalphanum(char c) {
    if (c>='a'&&c<='z') return true;
    if (c>='A'&&c<='Z') return true;
    if (c>='0'&&c<='9') return true;
    return false;
}
}
void HostedObject::initializeScript(const String& script, const ObjectScriptManager::Arguments &args) {
    assert(!mObjectScript); // Don't want to kill a live script!
    static ThreadIdCheck scriptId=ThreadId::registerThreadGroup(NULL);
    assertThreadGroup(scriptId);
    if (!ObjectScriptManagerFactory::getSingleton().hasConstructor(script)) {
        bool passed=true;
        for (std::string::const_iterator i=script.begin(),ie=script.end();i!=ie;++i) {
            if (!myisalphanum(*i)) {
                if (*i!='-'&&*i!='_') {
                    passed=false;
                }
            }
        }
        if (passed) {
            mObjectHost->getScriptPluginManager()->load(script);
        }
    }
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getConstructor(script)("");
    if (mgr) {
        mObjectScript = mgr->createObjectScript(this->getSharedPtr(), args);
    }
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const UUID&object_uuid_evidence)
{
    connect(spaceID, startingLocation, meshBounds, mesh, SolidAngle::Max, object_uuid_evidence);
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const SolidAngle& queryAngle,
        const UUID&object_uuid_evidence)
{
    if (spaceID == SpaceID::null())
        return;

    // Note: we always use Time::null() here.  The server will fill in the
    // appropriate value.  When we get the callback, we can fix this up.
    Time approx_server_time = Time::null();
    mObjectHost->connect(
        getSharedPtr(), spaceID,
        TimedMotionVector3f(approx_server_time, MotionVector3f( Vector3f(startingLocation.getPosition()), startingLocation.getVelocity()) ),
        TimedMotionQuaternion(approx_server_time,MotionQuaternion(startingLocation.getOrientation(),Quaternion(startingLocation.getAxisOfRotation(),startingLocation.getAngularSpeed()))),
        meshBounds,
        mesh,
        queryAngle,
        std::tr1::bind(&HostedObject::handleConnected, this, _1, _2, _3, _4, _5, _6),
        std::tr1::bind(&HostedObject::handleMigrated, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, this, _1)
    );

    if(mSpaceData->find(spaceID) == mSpaceData->end()) {
        mSpaceData->insert(
            SpaceDataMap::value_type( spaceID, PerSpaceData(this, spaceID) )
        );
    }
}

void HostedObject::handleConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds) {
    // FIXME should be using per presence SST layers
    mSSTDatagramLayers.push_back(
        BaseDatagramLayerType::createDatagramLayer(
            SpaceObjectReference(space, obj),
            mContext, mDelegateODPService
        )
    );

    // We have to manually do what mContext->mainStrand->wrap( ... ) should be
    // doing because it can't handle > 5 arguments.
    mContext->mainStrand->post(
        std::tr1::bind(&HostedObject::handleConnectedIndirect, this, space, obj, server, loc, orient, bnds)
    );
}

void HostedObject::handleConnectedIndirect(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds) {
    if (server == NullServerID) {
        HO_LOG(warning,"Failed to connect object (internal:" << mInternalObjectReference.toString() << ") to space " << space);
        return;
    }

    // Create
    SpaceObjectReference self_objref(space, obj);
    // Convert back to local time
    TimedMotionVector3f local_loc(localTime(space, loc.updateTime()), loc.value());
    TimedMotionQuaternion local_orient(localTime(space, orient.updateTime()), orient.value());
    ProxyObjectPtr self_proxy = createProxy(self_objref, self_objref, URI(), mIsCamera, local_loc, local_orient, bnds);

    // Use to initialize PerSpaceData
    SpaceDataMap::iterator psd_it = mSpaceData->find(space);
    PerSpaceData& psd = psd_it->second;
    initializePerSpaceData(psd, self_proxy);

    // Special case for camera
    ProxyCameraObjectPtr cam = std::tr1::dynamic_pointer_cast<ProxyCameraObject, ProxyObject>(self_proxy);
    if (cam)
        cam->attach(String(), 0, 0);
}

void HostedObject::handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server) {
    NOT_IMPLEMENTED(ho);
}

void HostedObject::handleStreamCreated(const SpaceObjectReference& spaceobj) {
    SSTStreamPtr sstStream = mObjectHost->getSpaceStream(spaceobj.space(), getUUID());

    if (sstStream != SSTStreamPtr() ) {
        sstStream->listenSubstream(OBJECT_PORT_LOCATION,
            std::tr1::bind(&HostedObject::handleLocationSubstream, this, spaceobj, _1, _2)
        );
        sstStream->listenSubstream(OBJECT_PORT_PROXIMITY,
            std::tr1::bind(&HostedObject::handleProximitySubstream, this, spaceobj, _1, _2)
        );
    }
}

void HostedObject::initializePerSpaceData(PerSpaceData& psd, ProxyObjectPtr selfproxy) {
    psd.initializeAs(selfproxy);
}

void HostedObject::disconnectFromSpace(const SpaceID &spaceID) {
    SpaceDataMap::iterator where;
    where=mSpaceData->find(spaceID);
    if (where!=mSpaceData->end()) {
        mSpaceData->erase(where);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" when not connected to it...");
    }
}

void HostedObject::receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg) {
    // Convert to ODP runtime format
    ODP::Endpoint src_ep(space, ObjectReference(msg->source_object()), msg->source_port());
    ODP::Endpoint dst_ep(space, ObjectReference(msg->dest_object()), msg->dest_port());

    if (mDelegateODPService->deliver(src_ep, dst_ep, MemoryReference(msg->payload()))) {
        // if this was true, it got delivered
        delete msg;
    }
    else {
        SILOG(cppoh,debug,"[HO] Undelivered message from " << src_ep << " to " << dst_ep);
        delete msg;
    }
}

void HostedObject::handleLocationSubstream(const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s) {
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleLocationSubstreamRead, this, spaceobj, s, new std::stringstream(), _1, _2) );
}

void HostedObject::handleProximitySubstream(const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s) {
    std::stringstream** prevdataptr = new std::stringstream*;
    *prevdataptr = new std::stringstream();
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleProximitySubstreamRead, this, spaceobj, s, prevdataptr, _1, _2) );
}

void HostedObject::handleLocationSubstreamRead(const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    if (handleLocationMessage(spaceobj, prevdata->str())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s
        s->registerReadCallback(0);
    }
}

void HostedObject::handleProximitySubstreamRead(const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream** prevdataptr, uint8* buffer, int length) {
    std::stringstream* prevdata = *prevdataptr;
    prevdata->write((const char*)buffer, length);

    while(true) {
        std::string msg = Network::Frame::parse(*prevdata);
        if (prevdata->eof()) {
            delete prevdata;
            prevdata = new std::stringstream();
            *prevdataptr = prevdata;
        }

        // If we don't have a full message, just wait for more
        if (msg.empty()) return;

        // Otherwise, try to handle it
        handleProximityMessage(spaceobj, msg);
    }

    // FIXME we should be getting a callback on stream close so we can clean up!
    //s->registerReadCallback(0);
}

bool HostedObject::handleLocationMessage(const SpaceObjectReference& spaceobj, const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    contents.ParseFromString(frame.payload());

    SpaceID space = spaceobj.space();
    ProxyManagerPtr proxy_manager = getProxyManager(space);
    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

        //std::cout << "update.mesh(): " << update.mesh() << "\n";
        //std::cout << "Received location message about: " << update.object().toString() << "\n";

        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(), ObjectReference(update.object())));
        if (!proxy_obj) continue;

        if (update.has_location()) {

            Sirikata::Protocol::TimedMotionVector update_loc = update.location();
            TimedMotionVector3f loc(localTime(space, update_loc.t()), MotionVector3f(update_loc.position(), update_loc.velocity()));

            proxy_obj->setLocation(loc);

            CONTEXT_OHTRACE(objectLoc,
                getUUID(),
                update.object(),
                loc
            );
        }

        if (update.has_orientation()) {
            Sirikata::Protocol::TimedMotionQuaternion update_orient = update.orientation();
            TimedMotionQuaternion orient(localTime(space, update_orient.t()), MotionQuaternion(update_orient.position(), update_orient.velocity()));
            proxy_obj->setOrientation(orient);
        }

        if (update.has_mesh()) {
          std::string mesh = update.mesh();
          ProxyMeshObject *meshObj = dynamic_cast<ProxyMeshObject*>(proxy_obj.get());

          if (meshObj && mesh != "") {
            //std::cout << "MESH UPDATE: " << mesh  << "!!!\n";
            meshObj->setMesh(Transfer::URI(mesh));
          }
          else if (mesh != ""){
            //std::cout << "MESH UPDATE but no proxy object!\n";
          }
        }
    }

    return true;
}

bool HostedObject::handleProximityMessage(const SpaceObjectReference& spaceobj, const std::string& payload) {
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(payload);
    if (!parse_success) return false;

    SpaceID space = spaceobj.space();
    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = contents.update(idx);

        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);

            //std::cout << "Proximity addition: " << addition.object().toString()  << " , "
            //          << addition.mesh() << " , "  <<  addition.location().position()  << "\n";

            SpaceObjectReference proximateID(spaceobj.space(), ObjectReference(addition.object()));
            TimedMotionVector3f loc(localTime(space, addition.location().t()), MotionVector3f(addition.location().position(), addition.location().velocity()));

            CONTEXT_OHTRACE(prox,
                getUUID(),
                addition.object(),
                true,
                loc
            );

            if (!getProxyManager(proximateID.space())->getProxyObject(proximateID)) {
                TimedMotionQuaternion orient(localTime(space, addition.orientation().t()), MotionQuaternion(addition.orientation().position(), addition.orientation().velocity()));

                URI meshuri;
                if (addition.has_mesh()) meshuri = URI(addition.mesh());

                // FIXME use weak_ptr instead of raw
                BoundingSphere3f bnds = addition.bounds();
                ProxyObjectPtr proxy_obj = createProxy(proximateID, spaceobj, meshuri, false, loc, orient, bnds);
            }
            else {
              ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space());
              ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(),
                                                                     ObjectReference(addition.object())));
              if (!proxy_obj) continue;

              ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(proxy_obj.get());
              if (mesh) {
                mesh->setMesh( Transfer::URI(addition.mesh()));
              }
            }
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);

            ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space());
            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(),
                                                                     ObjectReference(removal.object())));
            if (!proxy_obj) continue;

            ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(proxy_obj.get());
            if (mesh) {
              mesh->setMesh( Transfer::URI(""));
            }
            else continue;

            //std::cout << "Proximity removal: " << removal.object().toString()  <<  "\n";

            CONTEXT_OHTRACE(prox,
                getUUID(),
                removal.object(),
                false,
                TimedMotionVector3f()
            );
        }
    }

    return true;
}


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const URI& meshuri, bool is_camera, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs)
{
    ProxyObjectPtr returner = buildProxy(objref,owner_objref,meshuri,is_camera);
    returner->setLocation(tmv);
    returner->setOrientation(tmq);
    returner->setBounds(bs);

    if (!is_camera && meshuri) {
        ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(returner.get());
        if (mesh) mesh->setMesh(meshuri);
    }

    return returner;
}

//should only be called from within createProxy functions.  Otherwise, will not
//initilize position and quaternion correctly
ProxyObjectPtr HostedObject::buildProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const URI& meshuri, bool is_camera)
{

    ProxyManagerPtr proxy_manager = getProxyManager(objref.space());
    ProxyObjectPtr proxy_obj;

    if (is_camera) proxy_obj = ProxyObject::construct<ProxyCameraObject>
                       (proxy_manager.get(), objref, getSharedPtr(), owner_objref);
    else proxy_obj = ProxyObject::construct<ProxyMeshObject>(proxy_manager.get(), objref, getSharedPtr(), owner_objref);

// The call to createObject must occur before trying to do any other
// operations so that any listeners will be set up.
    proxy_manager->createObject(proxy_obj);
    return proxy_obj;
}

// Identification
SpaceObjectReference HostedObject::id(const SpaceID& space) const {
    SpaceDataMap::const_iterator it = mSpaceData->find(space);
    if (it == mSpaceData->end()) return SpaceObjectReference::null();
    return it->second.id();
}

// ODP::Service Interface
ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(space, objref, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(sor, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref) {
    return mDelegateODPService->bindODPPort(space, objref);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor) {
    return mDelegateODPService->bindODPPort(sor);
}

void HostedObject::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

ODP::DelegatePort* HostedObject::createDelegateODPPort(ODP::DelegateService* parentService, const SpaceObjectReference& spaceobj, ODP::PortID port) {
    assert(spaceobj.space() != SpaceID::any());
    assert(spaceobj.space() != SpaceID::null());

    // FIXME We used to check if we had a presence in the space, but are now
    // allocating based on the full SpaceObjectReference (presence ID).  We
    // should verify we have this presence.
    //SpaceDataMap::const_iterator space_data_it = mSpaceData->find(space);
    //if (space_data_it == mSpaceData->end())
    //    throw ODP::PortAllocationException("HostedObject::createDelegateODPPort can't allocate port because the HostedObject is not connected to the specified space.");

    ODP::Endpoint port_ep(spaceobj, port);
    return new ODP::DelegatePort(
        mDelegateODPService,
        port_ep,
        std::tr1::bind(
            &HostedObject::delegateODPPortSend, this,
            port_ep, _1, _2
        )
    );
}

bool HostedObject::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload) {
    assert(source_ep.space() == dest_ep.space());
    return mObjectHost->send(getSharedPtr(), source_ep.space(), source_ep.port(), dest_ep.object().getAsUUID(), dest_ep.port(), payload);
}

// Movement Interface

void HostedObject::requestLocationUpdate(const SpaceID& space, const TimedMotionVector3f& loc)
{
    sendLocUpdateRequest(space, &loc, NULL, NULL, NULL);
}

//only update the position of the object, leave the velocity and orientation unaffected
void HostedObject::requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos)
{
    Vector3f curVel = requestCurrentVelocity(space,oref);
    TimedMotionVector3f tmv (currentSpaceTime(space),MotionVector3f(pos,curVel));
//FIXME: re-write the requestLocationUpdate function so that takes in object
//reference as well
    requestLocationUpdate(space,tmv);
}

//only update the velocity of the object, leave the position and the orientation
//unaffected
void HostedObject::requestVelocityUpdate(const SpaceID& space,  const ObjectReference& oref, const Vector3f& vel)
{
    Vector3f curPos = Vector3f(requestCurrentPosition(space,oref));
    TimedMotionVector3f tmv (currentSpaceTime(space),MotionVector3f(curPos,vel));

    //FIXME: re-write the requestLocationUpdate function so that takes in object
    //reference as well
    requestLocationUpdate(space,tmv);
}

//goes into proxymanager and gets out the current location of the presence
//associated with
Vector3d HostedObject::requestCurrentPosition (const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj  = getProxy(space,oref);
//BFTM_FIXME: need to decide whether want the extrapolated position or last
//known position.  (Right now, we're going with last known position.)
    Vector3d currentPosition = proxy_obj->getPosition();
    return currentPosition;
}

Vector3f HostedObject::requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref)
{
    // ProxyManagerPtr proxy_manager = getProxyManager(space);
    // ProxyObjectPtr  proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return (Vector3f)proxy_obj->getVelocity();
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const TimedMotionQuaternion& orient) {
    sendLocUpdateRequest(space, NULL, &orient, NULL, NULL);
}

void HostedObject::requestBoundsUpdate(const SpaceID& space, const BoundingSphere3f& bounds) {
    sendLocUpdateRequest(space, NULL, NULL, &bounds, NULL);
}

void HostedObject::requestMeshUpdate(const SpaceID& space, const String& mesh) {
    sendLocUpdateRequest(space, NULL, NULL, NULL, &mesh);
}

void HostedObject::sendLocUpdateRequest(const SpaceID& space, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh) {
    // Generate and send an update to Loc
    Protocol::Loc::Container container;
    Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
    if (loc != NULL) {
        Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t( spaceTime(space, loc->updateTime()) );
        requested_loc.set_position(loc->position());
        requested_loc.set_velocity(loc->velocity());
    }
    if (orient != NULL) {
        Protocol::ITimedMotionQuaternion requested_orient = loc_request.mutable_orientation();
        requested_orient.set_t( spaceTime(space, orient->updateTime()) );
        requested_orient.set_position(orient->position());
        requested_orient.set_velocity(orient->velocity());
    }
    if (bounds != NULL)
        loc_request.set_bounds(*bounds);
    if (mesh != NULL)
        loc_request.set_mesh(*mesh);

    std::string payload = serializePBJMessage(container);

    SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != SSTStreamPtr()) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
            OBJECT_PORT_LOCATION, NULL);
    }
}

}
