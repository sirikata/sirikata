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
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/oh/ObjectQueryProcessor.hpp>

#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/oh/SimulationFactory.hpp>
#include <sirikata/oh/PerPresenceData.hpp>

#include <sirikata/oh/LocUpdate.hpp>
#include <sirikata/oh/ProtocolLocUpdate.hpp>
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"

#include <sirikata/oh/Storage.hpp>

// Transfer mediator added to download objects' Zernike descriptors
#include <sirikata/core/transfer/AggregatedTransferPool.hpp>

#define HO_LOG(lvl,msg) SILOG(ho,lvl,msg);

namespace Sirikata {



HostedObject::HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID & _id)
 : mContext(ctx),
   mID(_id),
   mObjectHost(parent),
   mObjectScript(NULL),
   destroyed(false)
{
    mNumOutstandingConnections=0;
    mDestroyWhenConnected=false;
    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &HostedObject::createDelegateODPPort, this,
            _1, _2, _3
        )
    );

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());
    mTransferPool = mTransferMediator->registerClient<Transfer::AggregatedTransferPool>("HostedObject_"+mID.toString());
}



void HostedObject::killSimulation(
    const SpaceObjectReference& sporef, const String& simName)
{
    if (stopped())
        return;

    PerPresenceData* pd = NULL;
    {
        Mutex::scoped_lock locker(presenceDataMutex);
        PresenceDataMap::iterator psd_it = mPresenceData.find(sporef);
        if (psd_it == mPresenceData.end())
        {
            HO_LOG(error, "Error requesting to stop a "<<        \
                "simulation for a presence that does not exist.");
            return;
        }

        pd = psd_it->second;

        if (pd->sims.find(simName) != pd->sims.end())
        {
            Simulation* simToKill = pd->sims[simName];
            simToKill->stop();
            delete simToKill;
            pd->sims.erase(simName);
        }
        else
            HO_LOG(error,"No simulation with name "<<simName<<" to remove");
    }
}

Simulation* HostedObject::runSimulation(
    const SpaceObjectReference& sporef, const String& simName,
    Network::IOStrandPtr simStrand)
{
    if (stopped()) return NULL;

    PerPresenceData* pd = NULL;
    {
        Mutex::scoped_lock locker(presenceDataMutex);
        PresenceDataMap::iterator psd_it = mPresenceData.find(sporef);
        if (psd_it == mPresenceData.end())
        {
            HO_LOG(error, "Error requesting to run a "<<        \
                "simulation for a presence that does not exist.");
            return NULL;
        }

        pd = psd_it->second;

        if (pd->sims.find(simName) != pd->sims.end()) {
            return pd->sims[simName];
        }
    }

    Simulation* sim = NULL;
    // This is kept outside the lock because the constructor can
    // access the HostedObject and call methods which need the
    // lock.
    HO_LOG(info,String("[OH] Initializing ") + simName);

    try {
        sim = SimulationFactory::getSingleton().getConstructor ( simName ) (
            mContext, static_cast<ConnectionEventProvider*>(mObjectHost),
            getSharedPtr(), sporef,
            getObjectHost()->getSimOptions(simName), simStrand
        );
    } catch(FactoryMissingConstructorException exc) {
        sim = NULL;
    }

    if (!sim) {
        HO_LOG(error, "Unable to load " << simName << " plugin.");
        return NULL;
    }

    HO_LOG(info,String("Successfully initialized ") + simName);
    {
        Mutex::scoped_lock locker(presenceDataMutex);
        pd->sims[simName] = sim;
        sim->start();
    }
    return sim;
}


HostedObject::~HostedObject() {
    destroy(false);

    Mutex::scoped_lock lock(presenceDataMutex);
    for (PresenceDataMap::iterator i=mPresenceData.begin();i!=mPresenceData.end();++i) {
        delete i->second;
    }

    delete mDelegateODPService;
    getObjectHost()->hostedObjectDestroyed(id());
    stop();
}

const UUID& HostedObject::id() const {
    return mID;
}

void HostedObject::start() {
    // Not really any need for the start call -- its always going to be invoked
    // immediately after construction.
}

void HostedObject::stop() {
    if (mObjectScript) {
        // We need to clear out the reference in storage, which will also clear
        // out leases. We do this in here to make sure it happens as we're
        // stopping. Otherwise we might stop everything, cleanup the storage,
        // then want to release buckets as we're doing final object cleanup when
        // it isn't possibly any more. We *also* try to do this during destroy()
        // because destroy() could be called for objects in the middle of run as
        // a result of a kill() scripting call rather than due to system shutdown.
        mObjectHost->getStorage()->releaseBucket(id());

        mObjectScript->stop();
    }
}

bool HostedObject::stopped() const {
    return (mContext->stopped() || destroyed);
}

void HostedObject::destroy(bool need_self)
{
    // Avoid recursive destruction
    if (destroyed) return;
    if (mNumOutstandingConnections>0) {
        mDestroyWhenConnected=true;
        return;//don't destroy during delicate connection process
    }

    // Make sure that we survive the entire duration of this call. Otherwise all
    // references may be lost, resulting in the destructor getting called
    // (e.g. when the ObjectScript removes all references) and then we return
    // here to do more work and we've already been deleted.
    HostedObjectPtr self_ptr = need_self ? getSharedPtr() : HostedObjectPtr();
    destroyed = true;

    if (mObjectScript) {
        // We need to clear out the reference in storage, which will also clear
        // out leases.
        mObjectHost->getStorage()->releaseBucket(id());
        // Then clear out the script
        delete mObjectScript;
        mObjectScript=NULL;
    }

    //copying the data to a separate map before clearing to avoid deadlock in
    //destructor.

    PresenceDataMap toDeleteFrom;
    {
        Mutex::scoped_lock locker(presenceDataMutex);
        toDeleteFrom.swap(mPresenceData);
    }

    for (PresenceDataMap::iterator iter = toDeleteFrom.begin();
         iter != toDeleteFrom.end(); ++iter)
    {
        // Make sure we explicitly. Other paths don't necessarily do this,
        // e.g. if the call to destroy happens between receiving a connection
        // success and the actual creation of the stream from the space, leaving
        // other components unaware that the connection has been made. Worst
        // case, the object host/session manager ignore the request.
        mObjectHost->disconnectObject(iter->first.space(),iter->first.object());
        delete iter->second;
        // And just clear the ref out from the ObjectHost
        mObjectHost->unregisterHostedObject(iter->first,this);
    }
}

Time HostedObject::spaceTime(const SpaceID& space, const Time& t) {
    return mObjectHost->spaceTime(space, t);
}

Time HostedObject::currentSpaceTime(const SpaceID& space) {
    return mObjectHost->currentSpaceTime(space);
}

Time HostedObject::localTime(const SpaceID& space, const Time& t) {
    return mObjectHost->localTime(space, t);
}

Time HostedObject::currentLocalTime() {
    return mObjectHost->currentLocalTime();
}


ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space, const ObjectReference& oref)
{
    Mutex::scoped_lock lock(presenceDataMutex);
    SpaceObjectReference toFind(space,oref);
    PresenceDataMap::const_iterator it = mPresenceData.find(toFind);
    if (it == mPresenceData.end())
        return ProxyManagerPtr();

    return it->second->proxyManager;
}


//returns all the spaceobjrefs associated with all presences of this object.
//They are returned in ss.
void HostedObject::getSpaceObjRefs(SpaceObjRefVec& ss) const
{
    Mutex::scoped_lock lock( const_cast<Mutex&>(presenceDataMutex) );
    PresenceDataMap::const_iterator smapIter;
    for (smapIter = mPresenceData.begin(); smapIter != mPresenceData.end(); ++smapIter)
        ss.push_back(SpaceObjectReference(smapIter->second->space,smapIter->second->object));
}



static ProxyObjectPtr nullPtr;
static ProxyManagerPtr nullManPtr;

ProxyObjectPtr HostedObject::getProxy(const SpaceID& space, const ObjectReference& oref)
{
    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    if (proxy_manager == nullManPtr)
    {
        SILOG(oh,info, "[HO] In getProxy of HostedObject, have no proxy manager associated with "<<space<<"-"<<oref);
        return nullPtr;
    }

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

void HostedObject::initializeScript(const String& script_type, const String& args, const String& script)
{
    if (stopped()) {
        HO_LOG(warn,"Ignoring script initialization after stop was requested.");
        return;
    }

    if (mObjectScript) {
        HO_LOG(warn,"Ignored initializeScript because script already exists for object");
        return;
    }

    HO_LOG(detailed,"Creating a script object for object");

    if (!ObjectScriptManagerFactory::getSingleton().hasConstructor(script_type)) {
        HO_LOG(debug,"[HO] Failed to create script for object because incorrect script type");
        return;
    }
    ObjectScriptManager *mgr = mObjectHost->getScriptManager(script_type);
    if (mgr) {
        HO_LOG(insane,"[HO] Creating script for object with args of "<<args);
        // First, tell storage that we're active. We only do this here because
        // only scripts use storage -- we don't need to try to activate it until
        // we have an active script
        mObjectHost->getStorage()->leaseBucket(id());
        // Then create the script
        mObjectScript = mgr->createObjectScript(this->getSharedPtr(), args, script);
        mObjectScript->start();
        mObjectScript->scriptTypeIs(script_type);
        mObjectScript->scriptOptionsIs(args);
    }
}

bool HostedObject::downloadZernikeDescriptor(const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& physics,
        const String& query,        
        const ObjectReference& orefID,
        PresenceToken token)
{
  Transfer::TransferRequestPtr req(new Transfer::MetadataRequest( Transfer::URI(mesh), 1.0, std::tr1::bind(
                                       &HostedObject::metadataDownloaded, this, spaceID,
                                       startingLocation,
                                       meshBounds,
                                       mesh,
                                       physics,
                                       query,                                       
                                       orefID,
                                       token, 0,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

  mTransferPool->addRequest(req);

  return true;
}

void HostedObject::metadataDownloaded(const SpaceID&spaceID,
                                    const Location&startingLocation,
                                    const BoundingSphere3f &meshBounds,
                                    const String& mesh,
                                    const String& physics,
                                    const String& query,                                    
                                    const ObjectReference& orefID,
                                    PresenceToken token,
                                    uint8_t retryCount, 
                                    std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                    std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)
{
  if (response != NULL || retryCount >= 3) {
    const Sirikata::Transfer::FileHeaders& headers = response->getHeaders();
    
    String zernike = "";
    if (headers.find("Zernike") != headers.end()) {
      zernike = (headers.find("Zernike"))->second;
    }

    std::tr1::shared_ptr<OHConnectInfo> ocip = std::tr1::shared_ptr<OHConnectInfo>(new OHConnectInfo);
    
    ocip->spaceID=spaceID;
    ocip->startingLocation = startingLocation;
    ocip->meshBounds = meshBounds;
    ocip->mesh = mesh;
    ocip->physics = physics;
    ocip->query =query;
    ocip->orefID = orefID;
    ocip->token = token;
    ocip->zernike = zernike;    
    
    mContext->mainStrand->post(std::tr1::bind(&HostedObject::objectHostConnectIndirect, this, ocip));    
  }
  else if (retryCount < 3) {
    Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(mesh), 1.0, std::tr1::bind(
                                       &HostedObject::metadataDownloaded, this, spaceID,
                                       startingLocation,
                                       meshBounds,
                                       mesh,
                                       physics,
                                       query,                                       
                                       orefID,
                                       token, retryCount+1,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);
  }
}

bool HostedObject::objectHostConnect(const SpaceID spaceID,
        const Location startingLocation,
        const BoundingSphere3f meshBounds,
        const String mesh,
        const String physics,
        const String query,
        const String zernike,        
        const ObjectReference orefID,
        PresenceToken token)
{
  ObjectReference oref = (orefID == ObjectReference::null()) ? ObjectReference(UUID::random()) : orefID;

  SpaceObjectReference connectingSporef (spaceID,oref);

  // Note: we always use Time::null() here.  The server will fill in the
  // appropriate value.  When we get the callback, we can fix this up.
  Time approx_server_time = Time::null();
  if (mObjectHost->connect(
                           getSharedPtr(),
                           connectingSporef, spaceID,
                           TimedMotionVector3f(approx_server_time, MotionVector3f( Vector3f(startingLocation.getPosition()), startingLocation.getVelocity()) ),
                           TimedMotionQuaternion(approx_server_time,MotionQuaternion(startingLocation.getOrientation().normal(),Quaternion(startingLocation.getAxisOfRotation(),startingLocation.getAngularSpeed()))),  //normalize orientations
                           meshBounds,
                           mesh,
                           physics,
                           query,
                           zernike,
                           std::tr1::bind(&HostedObject::handleConnected, getWeakPtr(), _1, _2, _3),
                           std::tr1::bind(&HostedObject::handleMigrated, getWeakPtr(), _1, _2, _3),
                           std::tr1::bind(&HostedObject::handleStreamCreated, getWeakPtr(), _1, _2, token),
                           std::tr1::bind(&HostedObject::handleDisconnected, getWeakPtr(), _1, _2)
                           )) {
    mObjectHost->registerHostedObject(connectingSporef,getSharedPtr());
    return true;
  }else {
    return false;
  }
}

bool HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& physics,
        const String& query,
        const ObjectReference& orefID,
        PresenceToken token) {
    if (stopped()) {
        HO_LOG(warn,"Ignoring HostedObject connection request after system stop requested.");
        return false;
    }

    if (spaceID == SpaceID::null())
        return false;

    // Download the Zernike descriptor from the CDN metadata first. Once that is done,
    // connect() will be invoked on the object host to actually connect the object to the
    // space.

    if (mesh.find("meerkat:") == 0 && GetOptionValue<bool>("specify-zernike-descriptor")) {
      downloadZernikeDescriptor(spaceID,
                              startingLocation,
                              meshBounds,
                              mesh,
                              physics,
                              query,                              
                              orefID,
                              token);    
    
      return false;
    }
    else {
      return objectHostConnect(spaceID, startingLocation, meshBounds, mesh, physics, query, "", orefID, token);
    }
}


void HostedObject::handleConnected(const HostedObjectWPtr& weakSelf, const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info)
{
    HostedObjectPtr self(weakSelf.lock());
    if ((!self)||self->stopped()) {
        HO_LOG(detailed,"Ignoring connection success after system stop requested.");

        return;
    }
    if (info.server == NullServerID)
    {
        HO_LOG(warning,"Earlier failure to connect object:" << obj << " to space " << space);
        return;

    }
    BaseDatagramLayerPtr baseDatagramLayer (self->mContext->sstConnMgr()->createDatagramLayer(
              SpaceObjectReference(space, obj),
              self->mContext, self->mDelegateODPService
           )   );

    // We have to manually do what mContext->mainStrand->wrap( ... ) should be
    // doing because it can't handle > 5 arguments.
    self->mContext->mainStrand->post(
        std::tr1::bind(&HostedObject::handleConnectedIndirect, weakSelf, space, obj, info, baseDatagramLayer),
        "HostedObject::handleConnectedIndirect"
    );
}


void HostedObject::handleConnectedIndirect(const HostedObjectWPtr& weakSelf, const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info, const BaseDatagramLayerPtr& baseDatagramLayer)
{
    if (info.server == NullServerID)
    {
        HO_LOG(warning,"Failed to connect object:" << obj << " to space " << space);
        return;
    }

    HostedObjectPtr self(weakSelf.lock());
    if (!self)
        return;

    SpaceObjectReference self_objref(space, obj);

    {
        Mutex::scoped_lock lock(self->presenceDataMutex);

        if(self->mPresenceData.find(self_objref) == self->mPresenceData.end())
        {
            self->mPresenceData.insert(
                PresenceDataMap::value_type(
                    self_objref,
                    new PerPresenceData(self, space, obj, baseDatagramLayer, info.query)
                )
            );
        }
    }


    // Convert back to local time
    TimedMotionVector3f local_loc(self->localTime(space, info.loc.updateTime()), info.loc.value());
    TimedMotionQuaternion local_orient(self->localTime(space, info.orient.updateTime()), info.orient.value());
    ProxyObjectPtr self_proxy = self->createProxy(self_objref, self_objref, Transfer::URI(info.mesh), local_loc, local_orient, info.bnds, info.physics, info.query, false, 0);


    // Use to initialize PerSpaceData. This just lets the PerPresenceData know
    // there's a self proxy now.
    {
        Mutex::scoped_lock lock(self->presenceDataMutex);
        PresenceDataMap::iterator psd_it = self->mPresenceData.find(self_objref);
        PerPresenceData& psd = *psd_it->second;
        psd.initializeAs(self_proxy);
    }
    HO_LOG(detailed,"Connected object " << obj << " to space " << space << " waiting on notice");

}

void HostedObject::handleMigrated(const HostedObjectWPtr& weakSelf, const SpaceID& space, const ObjectReference& obj, ServerID server)
{
    HostedObjectPtr self(weakSelf.lock());
    if (!self)
        return;
    // When we switch space servers, the ProxyObject's sequence
    // numbers will no longer match because this information isn't
    // moved with the object. Since we shouldn't get more updates from
    // the original server, we reset all the ProxyObjects *owned* by
    // this object to have seqno = 0 so they will start fresh for the
    // new space server.
    ProxyManagerPtr proxy_manager = self->getProxyManager(space, obj);
    if (!proxy_manager) {
        HO_LOG(error, "Got migrated message but don't have a ProxyManager for the object.");
        return;
    }
    proxy_manager->resetAllProxies();
}



void HostedObject::handleStreamCreated(const HostedObjectWPtr& weakSelf, const SpaceObjectReference& spaceobj, SessionManager::ConnectionEvent after, PresenceToken token) {
    HO_LOG(detailed,"Handling new SST stream from space server for " << spaceobj);
    HostedObjectPtr self(weakSelf.lock());
    if (!self)
        return;

    Mutex::scoped_lock lock(self->notifyMutex);
    HO_LOG(detailed,"Notifying of connected object " << spaceobj.object() << " to space " << spaceobj.space());
    if (after == SessionManager::Connected) {
        self->notify(&SessionEventListener::onConnected, self, spaceobj, token);
        if (--self->mNumOutstandingConnections==0&&self->mDestroyWhenConnected) {
            self->mDestroyWhenConnected=false;
            self->destroy(true);
        }

    } else if (after == SessionManager::Migrated)
        self->notify(&SessionEventListener::onMigrated, self, spaceobj, token);
}

void HostedObject::disconnectFromSpace(const SpaceID &spaceID, const ObjectReference& oref)
{
    if (stopped()) {
        HO_LOG(detailed,"Ignoring disconnection request after system stop requested.");
        return;
    }

    SpaceObjectReference sporef(spaceID, oref);
    Mutex::scoped_lock locker(presenceDataMutex);
    PresenceDataMap::iterator where;
    where=mPresenceData.find(sporef);
    if (where!=mPresenceData.end()) {
        // Need to actually send a disconnection request to the space. Note that
        // this occurse *before* getting rid of the other data so callbacks
        // invoked as a result still work.
        mObjectHost->disconnectObject(spaceID,oref);
        delete where->second;
        mPresenceData.erase(where);
        mObjectHost->unregisterHostedObject(sporef, this);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" and object: "<< oref<<" when not connected to it...");
    }
}

void HostedObject::handleDisconnected(
    const HostedObjectWPtr& weakSelf, const SpaceObjectReference& spaceobj,
    Disconnect::Code cc)
{
    HostedObjectPtr self(weakSelf.lock());
    if ((!self)||self->stopped()) {
        HO_LOG(detailed,"Ignoring disconnection callback after system stop requested.");
        return;
    }

    self->mContext->mainStrand->post(
        std::tr1::bind(&HostedObject::iHandleDisconnected,self.get(),
            weakSelf, spaceobj, cc),
        "HostedObject::iHandleDisconnected"
    );
}

void HostedObject::iHandleDisconnected(
    const HostedObjectWPtr& weakSelf, const SpaceObjectReference& spaceobj,
    Disconnect::Code cc)
{
    HostedObjectPtr self(weakSelf.lock());
    if ((!self)||self->stopped()) {
        HO_LOG(detailed,"Ignoring disconnection callback after system stop requested.");
        return;
    }

    Mutex::scoped_lock lock(self->notifyMutex);
    self->notify(&SessionEventListener::onDisconnected, self, spaceobj);

    // Only invoke disconnectFromSpace if we weren't already aware of the
    // disconnection, i.e. if the disconnect was due to the space and we haven't
    // cleaned up yet.
    if (cc == Disconnect::Forced)
        self->disconnectFromSpace(spaceobj.space(), spaceobj.object());
    if (cc == Disconnect::LoginDenied) {
        assert(self->mPresenceData.find(spaceobj)==self->mPresenceData.end());
        self->mObjectHost->unregisterHostedObject(spaceobj, self.get());
        if (--self->mNumOutstandingConnections==0&&self->mDestroyWhenConnected) {
            self->mDestroyWhenConnected=false;
            self->destroy(true);
        }
    }
}


void HostedObject::receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg) {
    if (stopped()) {
        HO_LOG(detailed,"Ignoring received message after system stop requested.");
        return;
    }

    // Convert to ODP runtime format
    ODP::Endpoint src_ep(space, ObjectReference(msg->source_object()), msg->source_port());
    ODP::Endpoint dst_ep(space, ObjectReference(msg->dest_object()), msg->dest_port());

    if (mDelegateODPService->deliver(src_ep, dst_ep, MemoryReference(msg->payload()))) {
        // if this was true, it got delivered
        delete msg;
    }
    else {
        SILOG(cppoh,detailed,"[HO] Undelivered message from " << src_ep << " to " << dst_ep);
        delete msg;
    }

}


void HostedObject::processLocationUpdate(const SpaceObjectReference& sporef, ProxyObjectPtr proxy_obj, const LocUpdate& update) {
    TimedMotionVector3f loc;
    TimedMotionQuaternion orient;
    BoundingSphere3f bounds;
    String mesh;
    String phy;

    TimedMotionVector3f* locptr = NULL;
    uint64 location_seqno = update.location_seqno();
    TimedMotionQuaternion* orientptr = NULL;
    uint64 orient_seqno = update.orientation_seqno();
    BoundingSphere3f* boundsptr = NULL;
    uint64 bounds_seqno = update.bounds_seqno();
    String* meshptr = NULL;
    uint64 mesh_seqno = update.mesh_seqno();
    String* phyptr = NULL;
    uint64 phy_seqno = update.physics_seqno();

    if (update.has_epoch()) {
        // Check if this object is our own presence and update our epoch info if
        // it is.
        Mutex::scoped_lock locker(presenceDataMutex);
        PresenceDataMap::iterator pres_it = mPresenceData.find(sporef);
        if (pres_it != mPresenceData.end()) {
            PerPresenceData* pd = pres_it->second;
            pd->latestReportedEpoch = std::max(pd->latestReportedEpoch, update.epoch());
        }
    }

    if (update.has_location()) {
        loc = update.locationWithLocalTime(this, sporef.space());

        CONTEXT_OHTRACE(objectLoc,
            sporef.object().getAsUUID(),
            //getUUID(),
            update.object().getAsUUID(),
            loc
        );

        locptr = &loc;
    }

    if (update.has_orientation()) {
        orient = update.orientationWithLocalTime(this, sporef.space());
        orientptr = &orient;
    }

    if (update.has_bounds()) {
        bounds = update.bounds();
        boundsptr = &bounds;
    }

    if (update.has_mesh()) {
        mesh = update.mesh();
        meshptr = &mesh;
    }

    if (update.has_physics()) {
        phy = update.physics();
        phyptr = &phy;
    }
    processLocationUpdate(
        sporef.space(), proxy_obj, false,
        locptr, location_seqno, orientptr, orient_seqno,
        boundsptr, bounds_seqno, meshptr, mesh_seqno, phyptr, phy_seqno
    );
}


void HostedObject::processLocationUpdate(
        const SpaceID& space, ProxyObjectPtr proxy_obj, bool predictive,
        TimedMotionVector3f* loc, uint64 loc_seqno,
        TimedMotionQuaternion* orient, uint64 orient_seqno,
        BoundingSphere3f* bounds, uint64 bounds_seqno,
        String* mesh, uint64 mesh_seqno,
        String* phy, uint64 phy_seqno
) {
    if (loc)
        proxy_obj->setLocation(*loc, loc_seqno);

    if (orient)
        proxy_obj->setOrientation(*orient, orient_seqno);

    if (bounds)
        proxy_obj->setBounds(*bounds, bounds_seqno);

    if (mesh)
        proxy_obj->setMesh(Transfer::URI(*mesh), mesh_seqno);

    if (phy && *phy != "")
        proxy_obj->setPhysics(*phy, phy_seqno);
}

void HostedObject::handleLocationUpdate(const SpaceObjectReference& observer, const LocUpdate& lu) {
    ProxyManagerPtr proxy_manager = this->getProxyManager(observer.space(), observer.object());
    if (!proxy_manager) {
        HO_LOG(warn,"Hosted Object received a message for a presence without a proxy manager.");
        return;
    }

    SpaceObjectReference observed(observer.space(), ObjectReference(lu.object()));
    ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(observed);
    assert(proxy_obj);

    this->processLocationUpdate( observer, proxy_obj, lu);
}

void HostedObject::handleProximityUpdate(const SpaceObjectReference& spaceobj, const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    HostedObject* self = this;
    SpaceID space = spaceobj.space();

    ProxyManagerPtr proxy_manager = self->getProxyManager(spaceobj.space(),spaceobj.object());
    if (!proxy_manager) {
        HO_LOG(warn,"Hosted Object received a message for a presence without a proxy manager.");
        return;
    }

    for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
        Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
        ProxProtocolLocUpdate add(addition);

        SpaceObjectReference proximateID(spaceobj.space(), add.object());

        TimedMotionVector3f loc(add.locationWithLocalTime(this, spaceobj.space()));

        CONTEXT_OHTRACE(prox,
            spaceobj.object().getAsUUID(),
            //getUUID(),
            addition.object(),
            true,
            loc
        );

        TimedMotionQuaternion orient(add.orientationWithLocalTime(this, spaceobj.space()));
        BoundingSphere3f bnds = add.bounds();
        String mesh = add.meshOrDefault();
        String phy = add.physicsOrDefault();
        bool isAggregate =
          (addition.type() == Sirikata::Protocol::Prox::ObjectAddition::Aggregate) ?
            true : false;


        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(proximateID);
        if (!proxy_obj) {
            Transfer::URI meshuri;
            if (addition.has_mesh()) meshuri = Transfer::URI(addition.mesh());


            // FIXME use weak_ptr instead of raw
            uint64 proxyAddSeqNo = add.location_seqno();
            assert( add.location_seqno() == add.orientation_seqno() &&
                 add.location_seqno() == add.bounds_seqno() &&
                add.location_seqno() == add.mesh_seqno() &&
                add.location_seqno() == add.physics_seqno());
            proxy_obj = self->createProxy(proximateID, spaceobj, meshuri, loc, orient, bnds, phy, "",
                                          isAggregate, proxyAddSeqNo);
        }
        else {
            // We need to handle optional values properly -- they
            // shouldn't get overwritten.
            String* mesh_ptr = (addition.has_mesh() ? &mesh : NULL);
            String* phy_ptr = (addition.has_physics() ? &phy : NULL);

            self->processLocationUpdate(space, proxy_obj, false,
                &loc, add.location_seqno(),
                &orient, add.orientation_seqno(),
                &bnds, add.bounds_seqno(),
                mesh_ptr, add.mesh_seqno(),
                phy_ptr, add.physics_seqno()
            );
        }

        // Always mark the object as valid (either revalidated, or just
        // valid for the first time)
        if (proxy_obj) proxy_obj->validate();

        //tells the object script that something that was close has come
        //into view
        if(self->mObjectScript)
            self->mObjectScript->notifyProximate(proxy_obj,spaceobj);
    }

    for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
        Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);

        SpaceObjectReference removed_obj_ref(spaceobj.space(),
            ObjectReference(removal.object()));

        bool permanent = (removal.has_type() && (removal.type() == Sirikata::Protocol::Prox::ObjectRemoval::Permanent));

        if (removed_obj_ref == spaceobj) {
            // We want to ignore removal of ourself -- we should
            // always be in our result set, and we don't want to
            // delete our own proxy.

            SILOG(oh,detailed,"Ignoring self removal from proximity results.");
        }
        else {
            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(removed_obj_ref);

            if (proxy_obj) {
                // NOTE: We *don't* reset the proxy object
                // here. Resetting it puts the seqnos back at 0,
                // but if we get an addition while still on this
                // space server, we actually want the old ones to
                // stay in place, in case of unordered prox/loc
                // updates. Resetting only happens when we move
                // across space servers (see handleMigrated).
                proxy_manager->destroyObject(proxy_obj);

                if (self->mObjectScript)
                    self->mObjectScript->notifyProximateGone(proxy_obj,spaceobj);

                proxy_obj->invalidate(permanent);
            }
        }

        CONTEXT_OHTRACE(prox,
            spaceobj.object().getAsUUID(),
            //getUUID(),
            removal.object(),
            false,
            TimedMotionVector3f()
        );
    }

}

ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs, const String& phy, const String& query, bool isAggregate, uint64 seqNo)
{
    ProxyManagerPtr proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());
    Mutex::scoped_lock lock(presenceDataMutex);
    if (!proxy_manager)
    {
        mPresenceData.insert(
            PresenceDataMap::value_type(
                owner_objref,
                new PerPresenceData(getSharedPtr(), owner_objref.space(),owner_objref.object(), BaseDatagramLayerPtr(), query)
            )
        );
        proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());
    }

    ProxyObjectPtr proxy_obj = proxy_manager->createObject(objref, tmv, tmq, bs, meshuri, phy,
                                                           isAggregate, seqNo);

    return proxy_obj;
}


ProxyManagerPtr HostedObject::presence(const SpaceObjectReference& sor)
{
    return getProxyManager(sor.space(), sor.object());
}

SequencedPresencePropertiesPtr HostedObject::presenceRequestedLocation(const SpaceObjectReference& sor) {
    Mutex::scoped_lock lock(presenceDataMutex);
    PresenceDataMap::const_iterator it = mPresenceData.find(sor);
    if (it == mPresenceData.end())
        return SequencedPresencePropertiesPtr();

    return it->second->requestLoc;
}

uint64 HostedObject::presenceLatestEpoch(const SpaceObjectReference& sor) {
    Mutex::scoped_lock lock(presenceDataMutex);
    PresenceDataMap::const_iterator it = mPresenceData.find(sor);
    if (it == mPresenceData.end())
        return 0;

    return it->second->latestReportedEpoch;
}

ProxyObjectPtr HostedObject::self(const SpaceObjectReference& sor)
{
    ProxyManagerPtr proxy_man = presence(sor);
    if (!proxy_man) return ProxyObjectPtr();
    ProxyObjectPtr proxy_obj = proxy_man->getProxyObject(sor);



    return proxy_obj;
}


// ODP::Service Interface
ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port)
{
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(space, objref, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor, ODP::PortID port) {
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(sor, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref) {
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(space, objref);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor) {
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(sor);
}

ODP::PortID HostedObject::unusedODPPort(const SpaceID& space, const ObjectReference& objref) {
    if (stopped()) return 0;
    return mDelegateODPService->unusedODPPort(space, objref);
}

ODP::PortID HostedObject::unusedODPPort(const SpaceObjectReference& sor) {
    if (stopped()) return 0;
    return mDelegateODPService->unusedODPPort(sor);
}

void HostedObject::registerDefaultODPHandler(const ODP::Service::MessageHandler& cb) {
    if (stopped()) return;
    mDelegateODPService->registerDefaultODPHandler(cb);
}

ODPSST::Stream::Ptr HostedObject::getSpaceStream(const SpaceObjectReference& sor) {
    return mObjectHost->getSpaceStream(sor.space(), sor.object());
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
    SpaceObjectReference sporef(source_ep.space(), source_ep.object());
    if (stopped()) return false;

    return mObjectHost->send(sporef,source_ep.space(), source_ep.port(), dest_ep.object().getAsUUID(),dest_ep.port(), payload);
}



void HostedObject::requestLocationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f& loc)
{
    updateLocUpdateRequest(space, oref,&loc, NULL, NULL, NULL, NULL);
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionQuaternion& orient) {
    updateLocUpdateRequest(space, oref, NULL, &orient, NULL, NULL, NULL);
}


void HostedObject::requestBoundsUpdate(const SpaceID& space, const ObjectReference& oref, const BoundingSphere3f& bounds) {
    updateLocUpdateRequest(space, oref,NULL, NULL, &bounds, NULL, NULL);
}

void HostedObject::requestMeshUpdate(const SpaceID& space, const ObjectReference& oref, const String& mesh)
{
    updateLocUpdateRequest(space, oref, NULL, NULL, NULL, &mesh, NULL);
}

String HostedObject::requestQuery(const SpaceID& space, const ObjectReference& oref)
{
    Mutex::scoped_lock lock(presenceDataMutex);
    PresenceDataMap::iterator iter = mPresenceData.find(SpaceObjectReference(space,oref));
    if (iter == mPresenceData.end())
    {
        SILOG(cppoh, error, "Error in cppoh, requesting solid angle for presence that doesn't exist in your presence map.  Returning max solid angle instead.");
        static String empty_query("");
        return empty_query;
    }
    return iter->second->query;
}

void HostedObject::requestPhysicsUpdate(const SpaceID& space, const ObjectReference& oref, const String& phy)
{
    updateLocUpdateRequest(space, oref, NULL, NULL, NULL, NULL, &phy);
}


void HostedObject::requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, const String& new_query) {
    if (stopped()) {
        HO_LOG(detailed,"Ignoring query update request after system stop.");
        return;
    }

    SpaceObjectReference sporef(space,oref);
    Mutex::scoped_lock lock(presenceDataMutex);
    PresenceDataMap::iterator pdmIter = mPresenceData.find(sporef);
    if (pdmIter != mPresenceData.end()) {
        pdmIter->second->query = new_query;
    }
    else {
        SILOG(cppoh,error,"Error in cppoh, requesting solid angle update for presence that doesn't exist in your presence map.");
    }

    mObjectHost->getQueryProcessor()->updateQuery(getSharedPtr(), sporef, new_query);
}

void HostedObject::requestQueryRemoval(const SpaceID& space, const ObjectReference& oref) {
    requestQueryUpdate(space, oref, "");
}

void HostedObject::updateLocUpdateRequest(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh, const String* const phy) {

    if (stopped()) {
        HO_LOG(detailed,"Ignoring loc update request after system stop.");
        return;
    }

    {
        // Scope this lock since sendLocUpdateRequest will acquire
        // lock itself
        Mutex::scoped_lock locker(presenceDataMutex);
        assert(mPresenceData.find(SpaceObjectReference(space, oref)) != mPresenceData.end());
        PerPresenceData& pd = *(mPresenceData.find(SpaceObjectReference(space, oref)))->second;

        // These set values directly, the epoch/seqno values will be
        // updated when the request is sent
        if (loc != NULL) { pd.requestLoc->setLocation(*loc); pd.updateFields |= PerPresenceData::LOC_FIELD_LOC; }
        if (orient != NULL) { pd.requestLoc->setOrientation(*orient); pd.updateFields |= PerPresenceData::LOC_FIELD_ORIENTATION; }
        if (bounds != NULL) { pd.requestLoc->setBounds(*bounds); pd.updateFields |= PerPresenceData::LOC_FIELD_BOUNDS; }
        if (mesh != NULL) { pd.requestLoc->setMesh(Transfer::URI(*mesh)); pd.updateFields |= PerPresenceData::LOC_FIELD_MESH; }
        if (phy != NULL) { pd.requestLoc->setPhysics(*phy); pd.updateFields |= PerPresenceData::LOC_FIELD_PHYSICS; }

        // Cancel the re-request timer if it was active
        pd.rerequestTimer->cancel();
    }

    sendLocUpdateRequest(space, oref);
}


namespace {
void discardChildStream(int success, SST::Stream<SpaceObjectReference>::Ptr sptr) {
    if (success != SST_IMPL_SUCCESS) return;
    sptr->close(false);
}
}


void HostedObject::sendLocUpdateRequest(const SpaceID& space, const ObjectReference& oref) {
    // Up here to avoid recursive lock
    ProxyObjectPtr self_proxy = getProxy(space, oref);

    Mutex::scoped_lock locker(presenceDataMutex);
    assert(mPresenceData.find(SpaceObjectReference(space, oref)) != mPresenceData.end());
    PerPresenceData& pd = *(mPresenceData.find(SpaceObjectReference(space, oref)))->second;

    if (!self_proxy)
    {
        HO_LOG(warn,"Requesting sendLocUpdateRequest for missing self proxy.  Doing nothing.");
        return;
    }

    assert(pd.updateFields != PerPresenceData::LOC_FIELD_NONE);
    // Generate and send an update to Loc
    Protocol::Loc::Container container;
    Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
    uint64 epoch = pd.requestEpoch++;
    loc_request.set_epoch(epoch);
    if (pd.updateFields & PerPresenceData::LOC_FIELD_LOC) {
        Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t( spaceTime(space, pd.requestLoc->location().updateTime()) );
        requested_loc.set_position(pd.requestLoc->location().position());
        requested_loc.set_velocity(pd.requestLoc->location().velocity());
        // Save value but bump the epoch
        pd.requestLoc->setLocation(pd.requestLoc->location(), epoch);
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_ORIENTATION) {
        Protocol::ITimedMotionQuaternion requested_orient = loc_request.mutable_orientation();
        requested_orient.set_t( spaceTime(space, pd.requestLoc->orientation().updateTime()) );
        //Normalize positions, which only make sense as unit quaternions.
        requested_orient.set_position(pd.requestLoc->orientation().position().normal());
        requested_orient.set_velocity(pd.requestLoc->orientation().velocity());
        // Save value but bump the epoch
        pd.requestLoc->setOrientation(pd.requestLoc->orientation(), epoch);
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_BOUNDS) {
        loc_request.set_bounds(pd.requestLoc->bounds());
        // Save value but bump the epoch
        pd.requestLoc->setBounds(pd.requestLoc->bounds(), epoch);
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_MESH) {
        loc_request.set_mesh(pd.requestLoc->mesh().toString());
        // Save value but bump the epoch
        pd.requestLoc->setMesh(pd.requestLoc->mesh(), epoch);
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_PHYSICS) {
        loc_request.set_physics(pd.requestLoc->physics());
        // Save value but bump the epoch
        pd.requestLoc->setPhysics(pd.requestLoc->physics(), epoch);
    }

    std::string payload = serializePBJMessage(container);


    bool send_succeeded = false;
    SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, oref);
    if (spaceStream) {
        spaceStream->createChildStream(
            std::tr1::bind(discardChildStream, _1, _2),
            (void*)payload.data(), payload.size(),
            OBJECT_PORT_LOCATION, OBJECT_PORT_LOCATION
        );
        send_succeeded = true;
    }

    if (send_succeeded) {
        pd.updateFields = PerPresenceData::LOC_FIELD_NONE;
    }
    else {
        // Set up retry timer. Just rerun this method, but add no new
        // update fields.
        pd.rerequestTimer->wait(
            Duration::milliseconds((int64)10),
            std::tr1::bind(&HostedObject::sendLocUpdateRequest, this, space, oref)
        );
    }
}



void HostedObject::commandPresences(
    const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid)
{
    Command::Result result = Command::EmptyResult();
    // Make sure we return the objects key set even if there are none
    result.put( String("presences"), Command::Object());
    Command::Object& presences_map = result.getObject("presences");

    {
        Mutex::scoped_lock locker(presenceDataMutex);
        for(PresenceDataMap::const_iterator presit = mPresenceData.begin(); presit != mPresenceData.end(); presit++) {
            Command::Object presdata;
            // Should fill in basic presence info but it's a pain to serialize
            // here (loc, orientation, etc).
            presdata["mesh"] = presit->second->requestLoc->mesh().toString();
            presdata["physics"] = presit->second->requestLoc->physics();
            Command::Object proxy_data;
            proxy_data["alive"] = presit->second->proxyManager->size();
            proxy_data["active"] = presit->second->proxyManager->activeSize();
            presdata["proxies"] = proxy_data;
            presences_map[presit->first.toString()] = presdata;
        }
    }

    cmdr->result(cmdid, result);
}

}
