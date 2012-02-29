/*  Sirikata
 *  BulletPhysicsService.cpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava, Rahul Sheth
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

#include "BulletPhysicsService.hpp"
#include "BulletObject.hpp"
#include "BulletRigidBodyObject.hpp"
#include "BulletCharacterObject.hpp"
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/mesh/CompositeFilter.hpp>

#include "Protocol_Loc.pbj.hpp"

#include <json_spirit/json_spirit.h>

#include <sirikata/core/transfer/AggregatedTransferPool.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata {

namespace {
void bulletPhysicsInternalTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    BulletPhysicsService *bps = static_cast<BulletPhysicsService*>(world->getWorldUserInfo());
    bps->internalTickCallback();
}
}

BulletPhysicsService::BulletPhysicsService(SpaceContext* ctx, LocationUpdatePolicy* update_policy)
 : LocationService(ctx, update_policy),
   mUpdateIteration(0),
   mParsingStrand( ctx->ioService->createStrand("BulletPhysicsService Parsing") )
{

    mBroadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver;
    mDynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, mBroadphase, solver, collisionConfiguration);
    mDynamicsWorld->setInternalTickCallback(bulletPhysicsInternalTickCallback, (void*)this);
    mDynamicsWorld->setGravity(btVector3(0,-9.8,0));

    mLastTime = mContext->simTime();
    mLastDeactivationTime = mContext->simTime();

    mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("any")("");
    try {
        // FIXME these have to be consistent with the ones in OgreRenderer.cpp
        // (actually, with anything on the client, which will soon mean the
        // scripting layer as well) or the simulations won't match
        // up. In this case, we can omit reduce-draw-calls & compute-normals, used by
        // Ogre, because all we actually care about is the adjustment
        // of the mesh to be centered (affecting both the collision
        // mesh and the bounds when used for collision).
        std::vector<String> names_and_args;
        names_and_args.push_back("center"); names_and_args.push_back("");
        mModelFilter = new Mesh::CompositeFilter(names_and_args);
    }
    catch(Mesh::CompositeFilter::Exception e) {
        BULLETLOG(warning,"Couldn't allocate requested model load filter, will not apply filter to loaded models.");
        mModelFilter = NULL;
    }

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());
    mTransferPool = mTransferMediator->registerClient<Transfer::AggregatedTransferPool>("BulletPhysics");

    BULLETLOG(detailed, "Service Loaded");
}

BulletPhysicsService::~BulletPhysicsService() {
    // Note that we should get removal requests for all objects.  Just as a
    // sanity check, we'll make sure we've cleaned everything out at this point.
    while(!mLocations.empty()) {
        cleanupLocationInfo(mLocations.begin()->second);
        mLocations.erase(mLocations.begin());
    }

    delete mDynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete mBroadphase;

    delete mModelFilter;
    delete mModelsSystem;
    delete mParsingStrand;

    BULLETLOG(detailed,"Service Unloaded");
}

bool BulletPhysicsService::contains(const UUID& uuid) const {
    return (mLocations.find(uuid) != mLocations.end());
}

LocationService::TrackingType BulletPhysicsService::type(const UUID& uuid) const {
    LocationMap::const_iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return NotTracking;
    if (it->second.aggregate)
        return Aggregate;
    if (it->second.local)
        return Local;
    return Replica;
}


void BulletPhysicsService::service() {
    //get the time elapsed between calls to this service and
    //move the simulation forward by that amount
    Time now = mContext->simTime();
    Duration delTime = now - mLastTime;
    mLastTime = now;
    float simForwardTime = delTime.toMilliseconds() / 1000.0f;

    // Pre tick
    for(UUIDSet::iterator id_it = mTickObjects.begin(); id_it != mTickObjects.end(); id_it++) {
        const UUID& locobj = *id_it;
        LocationInfo& locinfo = mLocations[locobj];
        assert(locinfo.simObject != NULL);
        locinfo.simObject->preTick(now);
    }
    // Step simulation
    mDynamicsWorld->stepSimulation(simForwardTime);
    // Post tick
    for(UUIDSet::iterator id_it = mTickObjects.begin(); id_it != mTickObjects.end(); id_it++) {
        const UUID& locobj = *id_it;
        LocationInfo& locinfo = mLocations[locobj];
        assert(locinfo.simObject != NULL);
        locinfo.simObject->postTick(now);
    }

    // Check for deactivated objects. Unfortunately there isn't a way to get
    // this information from bullet at the time deactivation, so we need to poll
    // for it
    static Duration deactivation_check_interval(Duration::seconds(1));
    if (now - mLastDeactivationTime > deactivation_check_interval) {
        for(UUIDSet::iterator id_it = mDeactivateableObjects.begin(); id_it != mDeactivateableObjects.end(); id_it++) {
            const UUID& locobj = *id_it;
            LocationInfo& locinfo = mLocations[locobj];
            assert(locinfo.simObject != NULL);
            locinfo.simObject->deactivationTick(now);
        }
        mLastDeactivationTime = now;
    }

    // Process location updates
    for(UUIDSet::iterator i = physicsUpdates.begin(); i != physicsUpdates.end(); i++) {
        LocationMap::iterator it = mLocations.find(*i);
        if(it != mLocations.end())
            notifyLocalLocationUpdated(*i, it->second.aggregate, it->second.props.location() );
    }
    physicsUpdates.clear();

    // See note at declaration of mUpdateIteration. The fastest possible update
    // rate depends on this constant (10) and the LocationService target tick
    // period (10ms) -- so we'll get updates at most every 100ms currently.
    if (mUpdateIteration++ % 10 == 0)
        mUpdatePolicy->service();
}

uint64 BulletPhysicsService::epoch(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.props.maxSeqNo();
}

TimedMotionVector3f BulletPhysicsService::location(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());


    LocationInfo locinfo = it->second;
    return locinfo.props.location();
}

Vector3f BulletPhysicsService::currentPosition(const UUID& uuid) {
    TimedMotionVector3f loc = location(uuid);
    Vector3f returner = loc.extrapolate(mContext->simTime()).position();
    return returner;
}

TimedMotionQuaternion BulletPhysicsService::orientation(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.props.orientation();
}

Quaternion BulletPhysicsService::currentOrientation(const UUID& uuid) {
    TimedMotionQuaternion orient = orientation(uuid);
    return orient.extrapolate(mContext->simTime()).position();
}

BoundingSphere3f BulletPhysicsService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.props.bounds();
}

const String& BulletPhysicsService::mesh(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    locinfo.mesh_copied_str = locinfo.props.mesh().toString();
    return locinfo.mesh_copied_str;
}

const String& BulletPhysicsService::physics(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    locinfo.physics_copied_str = locinfo.props.physics();
    return locinfo.physics_copied_str;
}

bool BulletPhysicsService::isFixed(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    const LocationInfo& locinfo = it->second;
    assert(locinfo.simObject != NULL);
    return locinfo.simObject->fixed();
}

bool BulletPhysicsService::directMotionRequestsEnabled(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    const LocationInfo& locinfo = it->second;
    // If we don't have a simulation object to refer to, it can move freely
    if (locinfo.simObject == NULL) return true;
    // If it's remote, we should always be applying updates
    if (!locinfo.local) return true;
    // Otherwise, defer updates to the simulated object
    return false;
}

void BulletPhysicsService::setLocation(const UUID& uuid, const TimedMotionVector3f& newloc) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    // Note non-epoch (seqno) version because this isn't due to a request.
    locinfo.props.setLocation(newloc);
    notifyLocalLocationUpdated( uuid, locinfo.aggregate, newloc );
}

void BulletPhysicsService::setOrientation(const UUID& uuid, const TimedMotionQuaternion& neworient) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    // Note non-epoch (seqno) version because this isn't due to a request.
    locinfo.props.setOrientation(neworient);
    notifyLocalOrientationUpdated( uuid, locinfo.aggregate, neworient );
}

void BulletPhysicsService::getMesh(const Transfer::URI meshURI, const UUID uuid, MeshdataParsedCallback cb) {
    Transfer::ResourceDownloadTaskPtr dl = Transfer::ResourceDownloadTask::construct(
        Transfer::URI(meshURI), mTransferPool, 1.0,
        // Ideally parsing wouldn't need to be serialized, but something about
        // getting callbacks from multiple threads and parsing simultaneously is
        // causing a crash
        mParsingStrand->wrap(
            std::tr1::bind(&BulletPhysicsService::getMeshCallback, this, _1, _2, _3, cb)
        )
    );
    mMeshDownloads[uuid] = dl;
    dl->start();
}

void BulletPhysicsService::getMeshCallback(Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request, Transfer::DenseDataPtr response, MeshdataParsedCallback cb) {
    // This callback can come in on a separate thread (e.g. from a tranfer
    // thread) so make sure we get it back on the main thread.
    if (request && response) {
        Transfer::ChunkRequestPtr chunkreq = std::tr1::static_pointer_cast<Transfer::ChunkRequest>(request);

        VisualPtr vis = mModelsSystem->load(chunkreq->getMetadata(), chunkreq->getMetadata().getFingerprint(), response);
        // FIXME support more than Meshdata
        MeshdataPtr mesh( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
        if (mesh && mModelFilter) {
            Mesh::MutableFilterDataPtr input_data(new Mesh::FilterData);
            input_data->push_back(mesh);
            Mesh::FilterDataPtr output_data = mModelFilter->apply(input_data);
            assert(output_data->single());
            mesh = std::tr1::dynamic_pointer_cast<Meshdata>(output_data->get());
        }
        mContext->mainStrand->post(std::tr1::bind(cb, mesh), "BulletPhysicsService::getMeshCallback");
    }
    else {
        mContext->mainStrand->post(std::tr1::bind(cb, MeshdataPtr()), "BulletPhysicsService::getMeshCallback");
    }
}

void BulletPhysicsService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh, const String& phy) {
    LocationMap::iterator it = mLocations.find(uuid);

    // Add or update the information to the cache
    if (it == mLocations.end()) {
        mLocations[uuid] = LocationInfo();
        it = mLocations.find(uuid);
    } else {
        // It was already in there as a replica, notify its removal
        assert(it->second.local == false);
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME remote server ID
        notifyReplicaObjectRemoved(uuid);
    }

    LocationInfo& locinfo = it->second;
    locinfo.props.setLocation(loc, 0);
    locinfo.props.setOrientation(orient, 0);
    locinfo.props.setBounds(bnds, 0);
    locinfo.props.setMesh(Transfer::URI(msh), 0);
    locinfo.props.setPhysics(phy, 0);
    locinfo.local = true;
    locinfo.aggregate = false;

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, true, loc);
    notifyLocalObjectAdded(uuid, false, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid));

    updatePhysicsWorld(uuid);
}

void BulletPhysicsService::updatePhysicsWorld(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    LocationInfo& locinfo = it->second;
    const Transfer::URI& msh = locinfo.props.mesh();
    const String& phy = locinfo.props.physics();

    /*BEGIN: Bullet Physics Code*/

    // Default values
    bulletObjTreatment objTreatment = DEFAULT_TREATMENT;
    bulletObjBBox objBBox = DEFAULT_BOUNDS;
    btScalar mass = DEFAULT_MASS;

    if (!phy.empty()) {
        // Parsing stage
        namespace json = json_spirit;
        json::Value settings;
        if (!json::read(phy, settings)) {
            BULLETLOG(error, "Error parsing physics properties: " << phy);
            return;
        }

        String objTreatmentString = settings.getString("treatment", "ignore");
        if (objTreatmentString == "static") objTreatment = BULLET_OBJECT_TREATMENT_STATIC;
        if (objTreatmentString == "dynamic") objTreatment = BULLET_OBJECT_TREATMENT_DYNAMIC;
        if (objTreatmentString == "linear_dynamic") objTreatment = BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC;
        if (objTreatmentString == "vertical_dynamic") objTreatment = BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC;
        if (objTreatmentString == "character") objTreatment = BULLET_OBJECT_TREATMENT_CHARACTER;

        String objBBoxString = settings.getString("bounds", "sphere");
        if (objBBoxString == "box") objBBox = BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT;
        if (objBBoxString == "triangles") objBBox = BULLET_OBJECT_BOUNDS_PER_TRIANGLE;

        mass = settings.getReal("mass", DEFAULT_MASS);
    }

    // Clear out previous state from the simulation.
    if (locinfo.simObject != NULL) {
        locinfo.simObject->unload();
        delete locinfo.simObject;
        locinfo.simObject = NULL;
    }

    // And then proceed to add the new simulated object into bullet
    switch(objTreatment) {
      case BULLET_OBJECT_TREATMENT_IGNORE:
        BULLETLOG(detailed," This mesh will not be added to the bullet world: " << msh);
        locinfo.simObject = NULL;
        return;
      case BULLET_OBJECT_TREATMENT_STATIC:
      case BULLET_OBJECT_TREATMENT_DYNAMIC:
      case BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC:
      case BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC:
        locinfo.simObject = new BulletRigidBodyObject(this, uuid, objTreatment, objBBox, mass);
        break;
      case BULLET_OBJECT_TREATMENT_CHARACTER:
        locinfo.simObject = new BulletCharacterObject(this, uuid, objBBox);
        break;
      default:
        BULLETLOG(detailed,"Error in objTreatment initialization!");
        locinfo.simObject = NULL;
        return;
    }

    // We may need the mesh in order to continue. We need it only if:
    // treatment != ignore (see above check) && bounds != sphere.
    if (locinfo.simObject->bbox() == BULLET_OBJECT_BOUNDS_SPHERE) {
        // Invoke directly since we have all the data we need
        updatePhysicsWorldWithMesh(uuid, MeshdataPtr());
    }
    else {
        getMesh(msh, uuid,
            std::tr1::bind(&BulletPhysicsService::updatePhysicsWorldWithMesh, this, uuid, _1)
        );
    }
}

void BulletPhysicsService::updatePhysicsWorldWithMesh(const UUID& uuid, MeshdataPtr retrievedMesh) {
    LocationMap::iterator it = mLocations.find(uuid);
    // It's possible it has already disconnected. TODO(ewencp) we
    // should clear the download instead of waiting for it to finish,
    // but this works for now. Actually, we should also make sure that
    // the data still matches (we could change physics to A, change it
    // to B, have them processed async, finish B, then finish A,
    // resulting in simulation that doesn't match the settings in the
    // locinfo).
    if (it == mLocations.end()) return;

    LocationInfo& locinfo = it->second;

    locinfo.simObject->load(retrievedMesh);
}

// Helper for cleaning up a LocationInfo before removing it
void BulletPhysicsService::cleanupLocationInfo(LocationInfo& locinfo) {
    if (locinfo.simObject != NULL) {
        locinfo.simObject->unload();
        delete locinfo.simObject;
        locinfo.simObject = NULL;
    }
}

void BulletPhysicsService::removeLocalObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == false );

    // FIXME we might want to give a grace period where we generate a replica if one isn't already there,
    // instead of immediately removing all traces of the object.
    // However, this needs to be handled carefully, prefer updates from another server, and expire
    // automatically.

    LocationInfo& locinfo = mLocations[uuid];
    cleanupLocationInfo(locinfo);
    mLocations.erase(uuid);

    // Remove from the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, false, TimedMotionVector3f());
    notifyLocalObjectRemoved(uuid, false);
}

LocationInfo& BulletPhysicsService::info(const UUID& uuid) {
    assert(mLocations.find(uuid) != mLocations.end());
    return mLocations[uuid];
}

const LocationInfo& BulletPhysicsService::info(const UUID& uuid) const {
    assert(mLocations.find(uuid) != mLocations.end());
    return mLocations.find(uuid)->second;
}

void BulletPhysicsService::addTickObject(const UUID& uuid) {
    mTickObjects.insert(uuid);
}
void BulletPhysicsService::removeTickObject(const UUID& uuid) {
    UUIDSet::iterator dynamic_obj_it = mTickObjects.find(uuid);
    if (dynamic_obj_it != mTickObjects.end())
        mTickObjects.erase(dynamic_obj_it);
}

void BulletPhysicsService::addInternalTickObject(const UUID& uuid) {
    mInternalTickObjects.insert(uuid);
}
void BulletPhysicsService::removeInternalTickObject(const UUID& uuid) {
    UUIDSet::iterator dynamic_obj_it = mInternalTickObjects.find(uuid);
    if (dynamic_obj_it != mInternalTickObjects.end())
        mInternalTickObjects.erase(dynamic_obj_it);
}

void BulletPhysicsService::addDeactivateableObject(const UUID& uuid) {
    mDeactivateableObjects.insert(uuid);
}
void BulletPhysicsService::removeDeactivateableObject(const UUID& uuid) {
    UUIDSet::iterator dynamic_obj_it = mDeactivateableObjects.find(uuid);
    if (dynamic_obj_it != mDeactivateableObjects.end())
        mDeactivateableObjects.erase(dynamic_obj_it);
}


void BulletPhysicsService::addUpdate(const UUID& uuid) {
    physicsUpdates.insert(uuid);
}

void BulletPhysicsService::updateObjectFromDeactivation(const UUID& uuid) {
    if (isFixed(uuid)) return;
    if (location(uuid).velocity() == Vector3f(0.f, 0.f, 0.f) &&
        orientation(uuid).velocity() == Quaternion::identity())
        return;

    Time t = context()->simTime();
    TimedMotionVector3f newLocation(
        t,
        MotionVector3f(
            currentPosition(uuid),
            Vector3f(0.f, 0.f, 0.f)
        )
    );
    setLocation(uuid, newLocation);
    BULLETLOG(insane, "Updating " << uuid.toString() << " to stopped.");

    TimedMotionQuaternion newOrientation(
        t,
        MotionQuaternion(
            currentOrientation(uuid),
            Quaternion::identity()
        )
    );
    setOrientation(uuid, newOrientation);

    physicsUpdates.insert(uuid);
}

void BulletPhysicsService::internalTickCallback() {
    Time t = mContext->simTime();
    for(UUIDSet::iterator id_it = mInternalTickObjects.begin(); id_it != mInternalTickObjects.end(); id_it++) {
        LocationInfo& locinfo = mLocations[*id_it];
        assert(locinfo.simObject != NULL);
        locinfo.simObject->internalTick(t);
    }
}

void BulletPhysicsService::addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh, const String& phy) {
    // Aggregates get randomly assigned IDs -- if there's a conflict either we
    // got a true conflict (incredibly unlikely) or somebody (prox/query
    // handler) screwed up.
    assert(mLocations.find(uuid) == mLocations.end());

    mLocations[uuid] = LocationInfo();
    LocationMap::iterator it = mLocations.find(uuid);

    LocationInfo& locinfo = it->second;
    locinfo.props.setLocation(loc, 0);
    locinfo.props.setOrientation(orient, 0);
    locinfo.props.setBounds(bnds, 0);
    locinfo.props.setMesh(Transfer::URI(msh), 0);
    locinfo.props.setPhysics(phy, 0);
    locinfo.local = true;
    locinfo.aggregate = true;

    // Add to the list of local objects
    notifyLocalObjectAdded(uuid, true, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid));
    updatePhysicsWorld(uuid);
}

void BulletPhysicsService::removeLocalAggregateObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == true );

    LocationInfo& locinfo = mLocations[uuid];
    cleanupLocationInfo(locinfo);
    mLocations.erase(uuid);

    notifyLocalObjectRemoved(uuid, true);
}

void BulletPhysicsService::updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setLocation(newval, 0); // no epochs for aggregates
    notifyLocalLocationUpdated( uuid, true, newval );
}
void BulletPhysicsService::updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setOrientation(newval, 0); // no epochs for aggregates
    notifyLocalOrientationUpdated( uuid, true, newval );
}
void BulletPhysicsService::updateLocalAggregateBounds(const UUID& uuid, const BoundingSphere3f& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    BoundingSphere3f oldval = loc_it->second.props.bounds();
    loc_it->second.props.setBounds(newval, 0); // no epochs for aggregates
    notifyLocalBoundsUpdated( uuid, true, newval );
    if (oldval != newval) updatePhysicsWorld(uuid);
}
void BulletPhysicsService::updateLocalAggregateMesh(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    String oldval = loc_it->second.props.mesh().toString();
    loc_it->second.props.setMesh(Transfer::URI(newval), 0); // no epochs for aggregates
    notifyLocalMeshUpdated( uuid, true, newval );
    if (oldval != newval) updatePhysicsWorld(uuid);
}
void BulletPhysicsService::updateLocalAggregatePhysics(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    String oldval = loc_it->second.props.physics();
    loc_it->second.props.setPhysics(newval, 0); // no epochs for aggregates
    notifyLocalPhysicsUpdated( uuid, true, newval );
    if (oldval != newval) updatePhysicsWorld(uuid);
}

void BulletPhysicsService::addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh, const String& phy) {
    // FIXME we should do checks on timestamps to decide which setting is "more" sane
    LocationMap::iterator it = mLocations.find(uuid);

    if (it != mLocations.end()) {
        // It already exists. If its local, ignore the update. If its another replica, somethings out of sync, but perform the update anyway
        LocationInfo& locinfo = it->second;
        if (!locinfo.local) {
            locinfo.props.reset();
            locinfo.props.setLocation(loc, 0);
            locinfo.props.setOrientation(orient, 0);
            locinfo.props.setBounds(bnds, 0);
            locinfo.props.setMesh(Transfer::URI(msh), 0);
            locinfo.props.setPhysics(phy, 0);
            //local = false
            // FIXME should we notify location and bounds updated info?
            updatePhysicsWorld(uuid);
        }
        // else ignore
    }
    else {
        // Its a new replica, just insert it
        LocationInfo locinfo;
        locinfo.props.setLocation(loc, 0);
        locinfo.props.setOrientation(orient, 0);
        locinfo.props.setBounds(bnds, 0);
        locinfo.props.setMesh(Transfer::URI(msh), 0);
        locinfo.props.setPhysics(phy, 0);
        locinfo.local = false;
        locinfo.aggregate = false;
        mLocations[uuid] = locinfo;

        // We only run this notification when the object actually is new
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, true, loc); // FIXME add remote server ID
        notifyReplicaObjectAdded(uuid, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid));
        updatePhysicsWorld(uuid);
    }
}

void BulletPhysicsService::removeReplicaObject(const Time& t, const UUID& uuid) {
    // FIXME we should maintain some time information and check t against it to make sure this is sane

    LocationMap::iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return;

    // If the object is marked as local, this is out of date information.  Just ignore it.
    LocationInfo& locinfo = it->second;
    if (locinfo.local)
        return;

    // Otherwise, remove and notify
    cleanupLocationInfo(locinfo);
    mLocations.erase(it);
    CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME add remote server ID
    notifyReplicaObjectRemoved(uuid);
}


void BulletPhysicsService::receiveMessage(Message* msg) {
    assert(msg->dest_port() == SERVER_PORT_LOCATION);
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    bool parsed = parsePBJMessage(&contents, msg->payload());

    if (parsed) {
        for(int32 idx = 0; idx < contents.update_size(); idx++) {
            Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

            // Its possible we'll get an out of date update. We only use this update
            // if (a) we have this object marked as a replica object and (b) we don't
            // have this object marked as a local object
            if (type(update.object()) != Replica)
                continue;

            LocationMap::iterator loc_it = mLocations.find( update.object() );
            // We can safely make this assertion right now because space server
            // to space server loc and prox are on the same reliable channel. If
            // this goes away then a) we can't make this assertion and b) we
            // need an OrphanLocUpdateManager to save updates where this
            // condition is false so they can be applied once the prox update
            // arrives.
            assert(loc_it != mLocations.end());

            // Many different changes could require updating the
            // physics simulation, this tracks if we need to and the
            // update is performed at the end.
            bool updatePhysics = false;

            // Whether we should directly apply requests to move the object
            bool apply_direct = directMotionRequestsEnabled(update.object());

            uint64 epoch = 0;
            if (update.has_epoch())
                epoch = update.epoch();

            if (update.has_location()) {
                // Remote objects are always updated, forced if necessary.
                TimedMotionVector3f newloc(
                    update.location().t(),
                    MotionVector3f( update.location().position(), update.location().velocity() )
                );

                if (apply_direct) {
                    loc_it->second.props.setLocation(newloc, epoch);
                }
                else {
                    assert(loc_it->second.simObject != NULL);
                    loc_it->second.simObject->applyForcedLocation(newloc, epoch);
                }

                notifyReplicaLocationUpdated( update.object(), loc_it->second.props.location() );
                CONTEXT_SPACETRACE(serverLoc, msg->source_server(), mContext->id(), update.object(), loc_it->second.props.location() );
            }

            if (update.has_orientation()) {
                // Remote objects are always updated, forced if necessary.
                TimedMotionQuaternion neworient(
                    update.orientation().t(),
                    MotionQuaternion( update.orientation().position(), update.orientation().velocity() )
                );

                if (apply_direct) {
                    loc_it->second.props.setOrientation(neworient, epoch);
                }
                else {
                    assert(loc_it->second.simObject != NULL);
                    loc_it->second.simObject->applyForcedOrientation(neworient, epoch);
                }

                notifyReplicaOrientationUpdated( update.object(), loc_it->second.props.orientation());
            }

            if (update.has_bounds()) {
                BoundingSphere3f oldbounds = loc_it->second.props.bounds();
                BoundingSphere3f newbounds = update.bounds();
                loc_it->second.props.setBounds(newbounds, epoch);
                notifyReplicaBoundsUpdated( update.object(), loc_it->second.props.bounds() );
                if (oldbounds != newbounds) updatePhysics = true;
            }

            if (update.has_mesh()) {
                Transfer::URI oldmesh = loc_it->second.props.mesh();
                Transfer::URI newmesh(update.mesh());
                loc_it->second.props.setMesh(newmesh, epoch);
                notifyReplicaMeshUpdated( update.object(), loc_it->second.props.mesh().toString() );
                if (oldmesh != newmesh) updatePhysics = true;
            }

            if (update.has_physics()) {
                String oldphy = loc_it->second.props.physics();
                String newphy = update.physics();
                loc_it->second.props.setPhysics(newphy, epoch);
                notifyReplicaPhysicsUpdated( update.object(), loc_it->second.props.physics());
                if (oldphy != newphy) updatePhysics = true;
            }

            if (updatePhysics)
                updatePhysicsWorld(update.object());
        }
    }

    delete msg;
}

bool BulletPhysicsService::locationUpdate(UUID source, void* buffer, uint32 length) {
    Sirikata::Protocol::Loc::Container loc_container;
    bool parse_success = loc_container.ParseFromString( String((char*) buffer, length) );
    if (!parse_success) {
        LOG_INVALID_MESSAGE_BUFFER(standardloc, error, ((char*)buffer), length);
        return false;
    }

    if (loc_container.has_update_request()) {
        Sirikata::Protocol::Loc::LocationUpdateRequest request = loc_container.update_request();

        TrackingType obj_type = type(source);
        if (obj_type == Local) {
            LocationMap::iterator loc_it = mLocations.find( source );
            assert(loc_it != mLocations.end());

            // Many different changes could require updating the
            // physics simulation, this tracks if we need to and the
            // update is performed at the end.
            bool updatePhysics = false;

            // Whether we should directly apply requests to move the object
            bool apply_direct = directMotionRequestsEnabled(source);

            uint64 epoch = 0;
            if (request.has_epoch())
                epoch = request.epoch();

            if (request.has_location()) {
                // When requested from an object, we only try to apply the update
                TimedMotionVector3f newloc(
                    request.location().t(),
                    MotionVector3f( request.location().position(), request.location().velocity() )
                );

                bool updated = false;
                if (apply_direct) {
                    loc_it->second.props.setLocation(newloc, epoch);
                    updated = true;
                }
                else {
                    assert(loc_it->second.simObject != NULL);
                    updated = loc_it->second.simObject->applyRequestedLocation(newloc, epoch);
                }

                // We always force an update, even if we don't apply the
                // change. This ensures that the client is notified of
                // the existing value with the current epoch,
                // indicating that the update was received but not applied.
                // TODO(ewencp) we accomplish this more efficiently by
                // having a notifyLocalEpochUpdated.
                notifyLocalLocationUpdated( source, loc_it->second.aggregate, loc_it->second.props.location() );
                if (updated) {
                    CONTEXT_SPACETRACE(serverLoc, mContext->id(), mContext->id(), source, loc_it->second.props.location() );
                }
            }

            if (request.has_orientation()) {
                // When requested from an object, we only try to apply the update
                TimedMotionQuaternion neworient(
                    request.orientation().t(),
                    MotionQuaternion( request.orientation().position(), request.orientation().velocity() )
                );

                // See above note about always sending an update, even
                // if nothing changed.
                if (apply_direct) {
                    loc_it->second.props.setOrientation(neworient, epoch);
                }
                else {
                    assert(loc_it->second.simObject != NULL);
                    loc_it->second.simObject->applyRequestedOrientation(neworient, epoch);
                }

                notifyLocalOrientationUpdated( source, loc_it->second.aggregate, loc_it->second.props.orientation() );
            }

            if (request.has_bounds()) {
                BoundingSphere3f oldbounds = loc_it->second.props.bounds();
                BoundingSphere3f newbounds = request.bounds();
                loc_it->second.props.setBounds(newbounds, epoch);
                notifyLocalBoundsUpdated( source, loc_it->second.aggregate, loc_it->second.props.bounds() );
                if (oldbounds != newbounds) updatePhysics = true;
            }

            if (request.has_mesh()) {
                Transfer::URI oldmesh = loc_it->second.props.mesh();
                Transfer::URI newmesh(request.mesh());
                loc_it->second.props.setMesh(newmesh, epoch);
                notifyLocalMeshUpdated( source, loc_it->second.aggregate, loc_it->second.props.mesh().toString() );
                if (oldmesh != newmesh) updatePhysics = true;
            }

            if (request.has_physics()) {
                String oldphy = loc_it->second.props.physics();
                String newphy = request.physics();
                loc_it->second.props.setPhysics(newphy, epoch);
                notifyLocalPhysicsUpdated( source, loc_it->second.aggregate, loc_it->second.props.physics() );
                if (oldphy != newphy) updatePhysics = true;
            }

            if (updatePhysics)
                updatePhysicsWorld(source);
        }
        else {
            // Warn about update to non-local object
        }
    }
    return true;
}



} // namespace Sirikata
