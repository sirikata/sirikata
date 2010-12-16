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
#include <sirikata/core/util/KnownMessages.hpp>
#include <sirikata/core/util/KnownScriptTypes.hpp>

#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <list>
#include <vector>
#include <sirikata/proxyobject/SimulationFactory.hpp>
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Frame.pbj.hpp"
#include "Protocol_JSMessage.pbj.hpp"

#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"


#define HO_LOG(lvl,msg) SILOG(ho,lvl,"[HO] " << msg);

namespace Sirikata {



HostedObject::HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID &objectName)
 : mContext(ctx),
   mObjectHost(parent),
   mObjectScript(NULL),
   mPresenceData(new PresenceDataMap),
   mNextSubscriptionID(0),
   mInternalObjectReference(objectName),
   mOrphanLocUpdates(ctx, ctx->mainStrand, Duration::seconds(10))
{
    mContext->add(&mOrphanLocUpdates);

    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &HostedObject::createDelegateODPPort, this,
            _1, _2, _3
        )
    );
}



//Need to define this function so that can register timeouts in jscript
Network::IOService* HostedObject::getIOService()
{
    return mContext->ioService;
}


TimeSteppedSimulation* HostedObject::runSimulation(const SpaceObjectReference& sporef, const String& simName)
{
    TimeSteppedSimulation* sim = NULL;

    PresenceDataMap::iterator psd_it = mPresenceData->find(sporef);
    if (psd_it == mPresenceData->end())
    {
        std::cout<<"\n\nERROR: should have an entry for this space object reference in presencedatamap.  Aborting.\n\n";
        assert(false);
    }

    PerPresenceData& pd =  psd_it->second;
    addSimListeners(pd,simName,sim);

    if (sim != NULL)
    {
        HO_LOG(info, "Adding simulation to context");
        mContext->add(sim);
    }
    return sim;
}


HostedObject::~HostedObject() {
    destroy();
    delete mPresenceData;
}

void HostedObject::init() {
    mObjectHost->registerHostedObject(getSharedPtr());
}

void HostedObject::destroy()
{
    if (mObjectScript) {
        delete mObjectScript;
        mObjectScript=NULL;
    }

    mPresenceData->clear();
    mObjectHost->unregisterHostedObject(mInternalObjectReference);
}

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


ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space, const ObjectReference& oref)
{
    PresenceDataMap::const_iterator it = mPresenceData->find(SpaceObjectReference(space,oref));
    if (it == mPresenceData->end())
    {
        // SpaceDataMap::const_iterator twoIt = mSpaceData->begin();
        // // for (twoIt = mSpaceData->begin(); twoIt != mSpaceData->end(); ++twoIt)
        // // {
        // //     SpaceObjectReference tmp = twoIt->first;
        // //     std::cout<<"values: "<<tmp<<"\n";
        // // }
        // it = mSpaceData->insert(
        //     SpaceDataMap::value_type( SpaceObjectReference(space,oref), PerPresenceData(this, space,oref) )
        // ).first;
        ProxyManagerPtr returner;
        return returner;

    }
    return it->second.proxyManager;
}


//returns all the spaceobjectreferences associated with the presence with id sporef
void HostedObject::getProxySpaceObjRefs(const SpaceObjectReference& sporef,SpaceObjRefVec& ss) const
{
    PresenceDataMap::iterator smapIter = mPresenceData->find(sporef);


    if (smapIter != mPresenceData->end())
    {
        //means that we actually did have a connection with this sporef
        //load the proxy objects the sporef'd connection has actually seen into
        //ss.
        smapIter->second.proxyManager->getAllObjectReferences(ss);
    }
}


//returns all the spaceobjrefs associated with all presences of this object.
//They are returned in ss.
void HostedObject::getSpaceObjRefs(SpaceObjRefVec& ss) const
{
    if (mPresenceData == NULL)
    {
        std::cout<<"\n\n\nCalling getSpaceObjRefs when not connected to any spaces.  This really shouldn't happen\n\n\n";
        assert(false);
    }

    PresenceDataMap::const_iterator smapIter;
    for (smapIter = mPresenceData->begin(); smapIter != mPresenceData->end(); ++smapIter)
    {
        ss.push_back(SpaceObjectReference(smapIter->second.space,smapIter->second.object));
    }
}



static ProxyObjectPtr nullPtr;
const ProxyObjectPtr &HostedObject::getProxyConst(const SpaceID &space, const ObjectReference& oref) const
{
    PresenceDataMap::const_iterator iter = mPresenceData->find(SpaceObjectReference(space,oref));
    if (iter == mPresenceData->end()) {
        return nullPtr;
    }
    return iter->second.mProxyObject;
}

static ProxyManagerPtr nullManPtr;
ProxyObjectPtr HostedObject::getProxy(const SpaceID& space, const ObjectReference& oref)
{
    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    if (proxy_manager == nullManPtr)
        return nullPtr;

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

void HostedObject::initializeScript(const String& script, const String& args)
{
    if (mObjectScript) {
        SILOG(oh,warn,"[HO] Ignored initializeScript because script already exists for " << getUUID().toString() << "(internal id)");
        return;
    }

    SILOG(oh,debug,"[HO] Creating a script object for " << getUUID().toString() << "(internal id)");

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
        else
        {
            SILOG(oh,debug,"[HO] Failed to create script for " << getUUID().toString() << "(internal id) because incorrect script type");
        }
    }
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getConstructor(script)("");
    if (mgr) {
        SILOG(oh,debug,"[HO] Creating script for " << getUUID().toString() << "(internal id) with args of "<<args);
        mObjectScript = mgr->createObjectScript(this->getSharedPtr(), args);
        mObjectScript->scriptTypeIs(script);
        mObjectScript->scriptOptionsIs(args);
    }
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const UUID&object_uuid_evidence,
        PerPresenceData* ppd)
{
    connect(spaceID, startingLocation, meshBounds, mesh, SolidAngle::Max, object_uuid_evidence,ppd);
}


void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const SolidAngle& queryAngle,
        const UUID&object_uuid_evidence,
        PerPresenceData* ppd)
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
        std::tr1::bind(&HostedObject::handleConnected, this, _1, _2, _3, ppd),
        std::tr1::bind(&HostedObject::handleMigrated, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, this, _1),
        std::tr1::bind(&HostedObject::handleDisconnected, this, _1, _2)
    );

}




void HostedObject::addSimListeners(PerPresenceData& pd, const String& simName,TimeSteppedSimulation*& sim)
{
    SpaceID space = mObjectHost->getDefaultSpace();

    SILOG(cppoh,error,simName);

    HO_LOG(info,String("[OH] Initializing ") + simName);
    sim = SimulationFactory::getSingleton().getConstructor ( simName ) ( mContext, getSharedPtr(), pd.id(), "");
    if (!sim)
    {
        HO_LOG(error,String("Unable to load ") + simName + String(" plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!"));
        std::cerr << "Press enter to continue" << std::endl;
        fgetc(stdin);
        exit(0);
    }
    else
    {
        mObjectHost->addListener(sim);
        HO_LOG(info,String("Successfully initialized ") + simName);
    }
}




void HostedObject::handleConnected(const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info, PerPresenceData* ppd)
{
    // FIXME this never gets cleaned out on disconnect
    mSSTDatagramLayers.push_back(
        BaseDatagramLayerType::createDatagramLayer(
            SpaceObjectReference(space, obj),
            mContext, mDelegateODPService
        )
    );

    // We have to manually do what mContext->mainStrand->wrap( ... ) should be
    // doing because it can't handle > 5 arguments.
    mContext->mainStrand->post(
        std::tr1::bind(&HostedObject::handleConnectedIndirect, this, space, obj, info, ppd)
    );
}


void HostedObject::handleConnectedIndirect(const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info, PerPresenceData* ppd)
{
    if (info.server == NullServerID) {
        HO_LOG(warning,"Failed to connect object (internal:" << mInternalObjectReference.toString() << ") to space " << space);
        return;
    }

    SpaceObjectReference self_objref(space, obj);
    if(mPresenceData->find(self_objref) == mPresenceData->end())
    {
        if (ppd != NULL)
        {
            ppd->populateSpaceObjRef(SpaceObjectReference(space,obj));
            mPresenceData->insert(
                PresenceDataMap::value_type(self_objref, *ppd)
            );
        }
        else
        {
            PerPresenceData toInsert(this,space,obj);
            mPresenceData->insert(
                PresenceDataMap::value_type(self_objref,PerPresenceData(this,space,obj))
            );
        }
    }

    // Convert back to local time
    TimedMotionVector3f local_loc(localTime(space, info.loc.updateTime()), info.loc.value());
    TimedMotionQuaternion local_orient(localTime(space, info.orient.updateTime()), info.orient.value());
    ProxyObjectPtr self_proxy = createProxy(self_objref, self_objref, Transfer::URI(info.mesh), local_loc, local_orient, info.bnds);

    // Use to initialize PerSpaceData
    PresenceDataMap::iterator psd_it = mPresenceData->find(self_objref);
    PerPresenceData& psd = psd_it->second;
    initializePerPresenceData(psd, self_proxy);


    //bind an odp port to listen for the begin scripting signal.  if have
    //receive the scripting signal for the first time, that means that we create
    //a JSObjectScript for this hostedobject
    bindODPPort(space,obj,Services::LISTEN_FOR_SCRIPT_BEGIN);

    notify(&SessionEventListener::onConnected, getSharedPtr(), self_objref);
}

void HostedObject::handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server)
{
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

void HostedObject::initializePerPresenceData(PerPresenceData& psd, ProxyObjectPtr selfproxy) {
    psd.initializeAs(selfproxy);
}

void HostedObject::disconnectFromSpace(const SpaceID &spaceID, const ObjectReference& oref)
{
    PresenceDataMap::iterator where;
    where=mPresenceData->find(SpaceObjectReference(spaceID, oref));
    if (where!=mPresenceData->end()) {
        mPresenceData->erase(where);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" when not connected to it...");
    }
}

void HostedObject::handleDisconnected(const SpaceObjectReference& spaceobj, Disconnect::Code cc) {
    notify(&SessionEventListener::onDisconnected, getSharedPtr(), spaceobj);
    disconnectFromSpace(spaceobj.space(), spaceobj.object());
}



//returns true if this is a script init message.  returns false otherwise
bool HostedObject::handleScriptInitMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
{
    if (dst.port() != Services::LISTEN_FOR_SCRIPT_BEGIN)
        return false;

    //I don't really know what this segment of code does.  I copied it from
    //processRPC
    Sirikata::JS::Protocol::ScriptingInit sMessage;


    bool parsed = sMessage.ParseFromArray(bodyData.data(),bodyData.size());

    if (! parsed)
        return false;

    if (!((sMessage.has_script()) && (sMessage.has_messager())))
    {
        return false;
    }

    String scriptType = sMessage.script();
    String messager   = sMessage.messager();

    if (messager != KnownMessages::INIT_SCRIPT)
        return false;

    if (scriptType == ScriptTypes::JS_SCRIPT_TYPE)
    {
        initializeScript(scriptType,"");
    }

    return true;
}


bool HostedObject::handleEntityCreateMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
{
    //if the message isn't on the create_entity port, then it's good.
    //Otherwise, it's bad.
    if (dst.port() != Services::CREATE_ENTITY)
        return false;

    return true;
}



void HostedObject::receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg) {
    // Convert to ODP runtime format
    ODP::Endpoint src_ep(space, ObjectReference(msg->source_object()), msg->source_port());
    ODP::Endpoint dst_ep(space, ObjectReference(msg->dest_object()), msg->dest_port());

    if (mDelegateODPService->deliver(src_ep, dst_ep, MemoryReference(msg->payload()))) {
        // if this was true, it got delivered

        delete msg;
    }
    else if (handleScriptInitMessage(src_ep,dst_ep,MemoryReference(msg->payload())))
    {
        //if this was true, then that means that it was an init script command,
        //and we dealt with it.
        delete msg;
    }
    else if (handleEntityCreateMessage(src_ep,dst_ep,MemoryReference(msg->payload())))
    {
        //if this was true, then that means that we got a
        //handleEntityCreateMessage, and tried to create a new entity
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

void HostedObject::processLocationUpdate(const SpaceID& space, ProxyObjectPtr proxy_obj, const Sirikata::Protocol::Loc::LocationUpdate& update) {
    uint64 seqno = (update.has_seqno() ? update.seqno() : 0);

    TimedMotionVector3f loc;
    TimedMotionQuaternion orient;
    BoundingSphere3f bounds;
    String mesh;

    TimedMotionVector3f* locptr = NULL;
    TimedMotionQuaternion* orientptr = NULL;
    BoundingSphere3f* boundsptr = NULL;
    String* meshptr = NULL;


    if (update.has_location()) {
        Sirikata::Protocol::TimedMotionVector update_loc = update.location();
        loc = TimedMotionVector3f(localTime(space, update_loc.t()), MotionVector3f(update_loc.position(), update_loc.velocity()));

        CONTEXT_OHTRACE(objectLoc,
            getUUID(),
            update.object(),
            loc
        );

        locptr = &loc;
    }

    if (update.has_orientation()) {
        Sirikata::Protocol::TimedMotionQuaternion update_orient = update.orientation();
        orient = TimedMotionQuaternion(localTime(space, update_orient.t()), MotionQuaternion(update_orient.position(), update_orient.velocity()));
        orientptr = &orient;
    }

    if (update.has_bounds()) {
        bounds = update.bounds();
        boundsptr = &bounds;
    }

    if (update.has_mesh()) {
        std::string mesh = update.mesh();
        meshptr = &mesh;
    }

    processLocationUpdate(space, proxy_obj, seqno, false, locptr, orientptr, boundsptr, meshptr);
}

void HostedObject::processLocationUpdate(const SpaceID& space, ProxyObjectPtr proxy_obj, uint64 seqno, bool predictive, TimedMotionVector3f* loc, TimedMotionQuaternion* orient, BoundingSphere3f* bounds, String* mesh) {

    if (loc)
        proxy_obj->setLocation(*loc, seqno);

    if (orient)
        proxy_obj->setOrientation(*orient, seqno);

    if (bounds)
        proxy_obj->setBounds(*bounds, seqno);

    if (mesh && *mesh != "")
        proxy_obj->setMesh(Transfer::URI(*mesh), seqno);
}

bool HostedObject::handleLocationMessage(const SpaceObjectReference& spaceobj, const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    contents.ParseFromString(frame.payload());

    SpaceID space = spaceobj.space();
    ObjectReference oref = spaceobj.object();
    ProxyManagerPtr proxy_manager = getProxyManager(space, oref);

    if (!proxy_manager)
    {
        return true;
    }

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

        ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());
        SpaceObjectReference observed(spaceobj.space(), ObjectReference(update.object()));
        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(observed);

        if (!proxy_obj) {
            mOrphanLocUpdates.addOrphanUpdate(observed, update);
            continue;
        }

        processLocationUpdate(space, proxy_obj, update);
    }

    return true;
}

bool HostedObject::handleProximityMessage(const SpaceObjectReference& spaceobj, const std::string& payload)
{
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(payload);
    if (!parse_success) return false;

    SpaceID space = spaceobj.space();
    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = contents.update(idx);

        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);

            SpaceObjectReference proximateID(spaceobj.space(), ObjectReference(addition.object()));
            TimedMotionVector3f loc(localTime(space, addition.location().t()), MotionVector3f(addition.location().position(), addition.location().velocity()));

            CONTEXT_OHTRACE(prox,
                getUUID(),
                addition.object(),
                true,
                loc
            );

            TimedMotionQuaternion orient(localTime(space, addition.orientation().t()), MotionQuaternion(addition.orientation().position(), addition.orientation().velocity()));
            BoundingSphere3f bnds = addition.bounds();
            String mesh = (addition.has_mesh() ? addition.mesh() : "");

            ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(),spaceobj.object());
            if (!proxy_manager)
                return true;

            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(proximateID);
            if (!proxy_obj) {
                Transfer::URI meshuri;
                if (addition.has_mesh()) meshuri = Transfer::URI(addition.mesh());

                // FIXME use weak_ptr instead of raw
                proxy_obj = createProxy(proximateID, spaceobj, meshuri, loc, orient, bnds);
            }
            else {
                // Reset so that updates from this new "session" for this proxy
                // get applied
                proxy_obj->reset();
                processLocationUpdate(space, proxy_obj, 0, true, &loc, &orient, &bnds, &mesh);
            }

            // Notify of any out of order loc updates
            OrphanLocUpdateManager::UpdateList orphan_loc_updates = mOrphanLocUpdates.getOrphanUpdates(proximateID);
            for(OrphanLocUpdateManager::UpdateList::iterator orphan_it = orphan_loc_updates.begin(); orphan_it != orphan_loc_updates.end(); orphan_it++)
                processLocationUpdate(spaceobj.space(), proxy_obj, *orphan_it);

            //tells the object script that something that was close has come
            //into view
            if(mObjectScript)
                mObjectScript->notifyProximate(proxy_obj,spaceobj);
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);


            ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());
            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(),
                                                                     ObjectReference(removal.object())));
            if (!proxy_obj) continue;


            if (mObjectScript)
                mObjectScript->notifyProximateGone(proxy_obj,spaceobj);

            // FIXME this is *not* the right way to handle this
            proxy_obj->setMesh(Transfer::URI(""), 0, true);


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


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs)
{
    ProxyObjectPtr returner = buildProxy(objref,owner_objref,meshuri);
    returner->setLocation(tmv, 0);
    returner->setOrientation(tmq, 0);
    returner->setBounds(bs, 0);

    if(meshuri)
        returner->setMesh(meshuri, 0);

    return returner;
}


//should only be called from within createProxy functions.  Otherwise, will not
//initilize position and quaternion correctly
ProxyObjectPtr HostedObject::buildProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri)
{
    ProxyManagerPtr proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());

    if (!proxy_manager)
    {
        mPresenceData->insert(PresenceDataMap::value_type( owner_objref, PerPresenceData(this, owner_objref.space(),owner_objref.object()) ));
        proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());
    }

    ProxyObjectPtr proxy_obj = ProxyObject::construct<ProxyObject> (proxy_manager.get(),objref,getSharedPtr(),owner_objref);

    // The call to createObject must occur before trying to do any other
    // operations so that any listeners will be set up.
    proxy_manager->createObject(proxy_obj);
    return proxy_obj;
}

ProxyManagerPtr HostedObject::getDefaultProxyManager(const SpaceID& space)
{
    std::cout<<"\n\nINCORRECT in getDefaultProxyManager: should try to match object!!!\n\n";
    ObjectReference oref = mPresenceData->begin()->first.object();
    return  getProxyManager(space, oref);
}

ProxyObjectPtr HostedObject::getDefaultProxyObject(const SpaceID& space)
{
    std::cout<<"\n\nINCORRECT in getDefaultProxyObject: should try to match object!!!\n\n";
    ObjectReference oref = mPresenceData->begin()->first.object();
    return  getProxy(space, oref);
}


ProxyManagerPtr HostedObject::presence(const SpaceObjectReference& sor) {
    ProxyManagerPtr proxyManPtr = getProxyManager(sor.space(), sor.object());
    return proxyManPtr;
}

ProxyObjectPtr HostedObject::self(const SpaceObjectReference& sor) {
    ProxyManagerPtr proxy_man = presence(sor);
    if (!proxy_man) return ProxyObjectPtr();
    ProxyObjectPtr proxy_obj = proxy_man->getProxyObject(sor);
    return proxy_obj;
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

bool HostedObject::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload)
{
    assert(source_ep.space() == dest_ep.space());
    return mObjectHost->send(getSharedPtr(), source_ep.space(), source_ep.port(), dest_ep.object().getAsUUID(), dest_ep.port(), payload);
}



void HostedObject::requestLocationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f& loc)
{
    sendLocUpdateRequest(space, oref,&loc, NULL, NULL, NULL);
}

//only update the position of the object, leave the velocity and orientation unaffected
void HostedObject::requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos)
{
    Vector3f curVel = requestCurrentVelocity(space,oref);
    TimedMotionVector3f tmv (currentSpaceTime(space),MotionVector3f(pos,curVel));
    requestLocationUpdate(space,oref,tmv);
}

//only update the velocity of the object, leave the position and the orientation
//unaffected
void HostedObject::requestVelocityUpdate(const SpaceID& space,  const ObjectReference& oref, const Vector3f& vel)
{
    Vector3f curPos = Vector3f(requestCurrentPosition(space,oref));
    TimedMotionVector3f tmv (currentSpaceTime(space),MotionVector3f(curPos,vel));
    requestLocationUpdate(space,oref,tmv);
}

//send a request to update the orientation of this object
void HostedObject::requestOrientationDirectionUpdate(const SpaceID& space, const ObjectReference& oref,const Quaternion& quat)
{
    Quaternion curQuatVel = requestCurrentQuatVel(space,oref);
    TimedMotionQuaternion tmq (Time::local(),MotionQuaternion(quat,curQuatVel));
    requestOrientationUpdate(space,oref, tmq);
}


Quaternion HostedObject::requestCurrentQuatVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getOrientationSpeed();
}


Quaternion HostedObject::requestCurrentOrientation(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    Location curLoc = proxy_obj->extrapolateLocation(Time::local());
    return curLoc.getOrientation();
}

Quaternion HostedObject::requestCurrentOrientationVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getOrientationSpeed();
}

void HostedObject::requestOrientationVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Quaternion& quat)
{
    Quaternion curOrientQuat = requestCurrentOrientation(space,oref);
    TimedMotionQuaternion tmq (Time::local(),MotionQuaternion(curOrientQuat,quat));
    requestOrientationUpdate(space, oref,tmq);
}



//goes into proxymanager and gets out the current location of the presence
//associated with
Vector3d HostedObject::requestCurrentPosition (const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj  = getProxy(space,oref);

    //BFTM_FIXME: need to decide whether want the extrapolated position or last
    //known position.  (Right now, we're going with last known position.)


    Location curLoc = proxy_obj->extrapolateLocation(Time::local());
    Vector3d currentPosition = curLoc.getPosition();
    return currentPosition;
}


//apparently, will always return true now.  even if camera.
bool HostedObject::requestMeshUri(const SpaceID& space, const ObjectReference& oref, Transfer::URI& tUri)
{

    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    ProxyObjectPtr  proxy_obj     = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));

    tUri = proxy_obj->getMesh();
    return true;
}


Vector3f HostedObject::requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return (Vector3f)proxy_obj->getVelocity();
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionQuaternion& orient) {
    sendLocUpdateRequest(space, oref, NULL, &orient, NULL, NULL);
}

BoundingSphere3f HostedObject::requestCurrentBounds(const SpaceID& space,const ObjectReference& oref) {
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getBounds();
}

void HostedObject::requestBoundsUpdate(const SpaceID& space, const ObjectReference& oref, const BoundingSphere3f& bounds) {
    sendLocUpdateRequest(space, oref,NULL, NULL, &bounds, NULL);
}

void HostedObject::requestMeshUpdate(const SpaceID& space, const ObjectReference& oref, const String& mesh)
{
    sendLocUpdateRequest(space, oref, NULL, NULL, NULL, &mesh);
}

void HostedObject::requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, SolidAngle new_angle) {
    Protocol::Prox::QueryRequest request;
    request.set_query_angle(new_angle.asFloat());
    std::string payload = serializePBJMessage(request);

    SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != SSTStreamPtr()) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram(
            (void*)payload.data(), payload.size(),
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY,
            NULL
        );
    }
}

void HostedObject::requestQueryRemoval(const SpaceID& space, const ObjectReference& oref) {
    requestQueryUpdate(space, oref, SolidAngle::Max);
}

void HostedObject::sendLocUpdateRequest(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh) {
    ProxyObjectPtr self_proxy = getProxy(space, oref);
    // Generate and send an update to Loc
    Protocol::Loc::Container container;
    Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
    if (loc != NULL) {
        self_proxy->setLocation(*loc, 0, true);
        Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t( spaceTime(space, loc->updateTime()) );
        requested_loc.set_position(loc->position());
        requested_loc.set_velocity(loc->velocity());
    }
    if (orient != NULL) {
        self_proxy->setOrientation(*orient, 0, true);
        Protocol::ITimedMotionQuaternion requested_orient = loc_request.mutable_orientation();
        requested_orient.set_t( spaceTime(space, orient->updateTime()) );
        requested_orient.set_position(orient->position());
        requested_orient.set_velocity(orient->velocity());
    }
    if (bounds != NULL) {
        self_proxy->setBounds(*bounds, 0, true);
        loc_request.set_bounds(*bounds);
    }
    if (mesh != NULL)
    {
        self_proxy->setMesh(Transfer::URI(*mesh), 0, true);
        loc_request.set_mesh(*mesh);
    }

    std::string payload = serializePBJMessage(container);


    SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != SSTStreamPtr()) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
            OBJECT_PORT_LOCATION, NULL);
    }
}


Location HostedObject::getLocation(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy = getProxy(space, oref);
    assert(proxy);
    Location currentLoc = proxy->globalLocation(currentSpaceTime(space));
    return currentLoc;
}


void HostedObject::persistToFile(std::ofstream& fp)
{
  SpaceObjRefVec ss;

  getSpaceObjRefs(ss);

  SpaceObjRefVec::iterator it = ss.begin();

  for(; it != ss.end(); it++)
  {
      HostedObject::EntityState* es = getEntityState((*it).space(), (*it).object());
      es->persistToFile(fp);
  }
}





void HostedObject::EntityState::persistToFile(std::ofstream& fp)
{
   /* We need to persist the things that are not part of the state. Like the meta state definitions. so,we are writing this file over and over again */

   fp << "\"" << objType << "\"" << "," << "\"" << subType << "\"" << "," << "\"" << name << "\"" << ",";
   /* Persist each field separated by a comma */
   /* persist the position  */

	 fp << pos.x << "," << pos.y << "," << pos.z << ",";


	 /* persist the quaternion.. How do it do it? I don't have x,y,z,w */
	 fp << orient.x << "," << orient.y << "," << orient.z << "," << orient.w << "," ;

	 /* persist the velocity */

	 fp << vel.x << "," << vel.y << "," << vel.z << "," ;

	 /* persist the rotation */

	 fp << rot.x << "," << rot.y << "," << rot.z << ",";

	 /* persist the angular speed*/

	 fp << angular_speed << ",";

	 /*  persist the mesh url*/

	 fp << "\"" << mesh << "\""  << "," ;


	 /* persist the scale */

	 fp << scale << ",";


	 /* persist the object id */

	 fp << objectID << ",";


	 fp << script_type << ",";


   fp << script_opts << std::endl;
}


HostedObject::EntityState* HostedObject::getEntityState(const SpaceID& space, const ObjectReference& oref)
{

    HostedObject::EntityState* es = new HostedObject::EntityState();
    ProxyObjectPtr poptr = getProxy(space, oref);



    Location loc = getLocation(space, oref);
    es->objType = "mesh";
    es->subType = "graphiconly";

    // FIXME : HostedObject does not take the name of the entity into account right now after reading from the scene file.
    es->name = "unknown";
    es->pos = loc.getPosition();
    es->orient = loc.getOrientation();
    es->vel = loc.getVelocity();
    es->rot = loc.getAxisOfRotation();
    es->angular_speed = loc.getAngularSpeed();


    if (poptr == nullPtr)
        assert (false);

    std::cout<<"\n\n";
    std::cout<<poptr->getMesh();
    std::cout<<"\n\n";
    std::cout.flush();

    es->mesh = poptr->getMesh().toString();



    /* Get Scale from the Bounding Sphere. Scale is the radius of this sphere */
    es->scale = poptr->getBounds().radius();
    es->objectID = oref.toString();

    if(mObjectScript)
    {
        es->script_type = mObjectScript->scriptType();
        es->script_opts = mObjectScript->scriptOptions();
    }
    return es;

}





}
