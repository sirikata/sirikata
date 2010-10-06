/*  Sirikata
 *  ObjectFactory.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include "ObjectFactory.hpp"
#include <sirikata/oh/ObjectHostContext.hpp>

#include "Object.hpp"
#include <sirikata/core/util/Random.hpp>

#include "RandomMotionPath.hpp"
#include "QuakeMotionPath.hpp"
#include "StaticMotionPath.hpp"
#include "OSegTestMotionPath.hpp"

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/trace/Trace.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata {

static UUID randomUUID() {
    uint8 raw_uuid[UUID::static_size];
    for(uint32 ui = 0; ui < UUID::static_size; ui++)
        raw_uuid[ui] = (uint8)rand() % 256;
    UUID id(raw_uuid, UUID::static_size);
    return id;
}

static UUID pack2UUID(const uint64 packid) {
    uint8 raw_uuid[UUID::static_size];
    const uint8* data_src = (const uint8*)&packid;
    for(uint32 ui = 0; ui < UUID::static_size; ui++)
        raw_uuid[ui] = data_src[ ui % sizeof(uint64) ];
    UUID id(raw_uuid, UUID::static_size);
    return id;
}

ObjectFactory::ObjectFactory(ObjectHostContext* ctx, const BoundingBox3f& region, const Duration& duration, double forceRadius, int forceNumRandomObjects)
 : Service(),
   mContext(ctx),
   mLocalIDSource(0)
{
    // Note: we do random second in order make sure they get later connect times
    generateRandomObjects(region, duration, forceRadius, forceNumRandomObjects);
    if (!forceNumRandomObjects) {
        generatePackObjects(region, duration);
        generateStaticTraceObjects(region, duration);
    }
    setConnectTimes();

    // Possibly dump pack data.
    dumpObjectPack();
}

ObjectFactory::~ObjectFactory() {
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end();) {
        Object* obj = it->second;
        ++it;
        delete obj;
    }

    for(ObjectInputsMap::iterator it = mInputs.begin(); it != mInputs.end();) {
        ObjectInputs* inputs = it->second;
        ++it;
        delete inputs->motion;
        delete inputs;
    }
}

Object*ObjectFactory::getNextObject(const UUID&objid){
    if (mObjects.empty()) return NULL;
    ObjectMap::iterator where=(mObjects.find(objid));
    if (where!=mObjects.end()) {
        ++where;
        if (where!=mObjects.end())
            return where->second;
        return NULL;
    }
    return mObjects.begin()->second;
}

void ObjectFactory::generateRandomObjects(const BoundingBox3f& region, const Duration& duration, double forceRadius, int forceNumRandomObjects) {
    Time start(Time::null());
    Time end = start + duration;
    Vector3f region_extents = region.extents();

    uint32 nobjects              = GetOptionValue<uint32>(OBJECT_NUM_RANDOM);
    if (forceNumRandomObjects)
        nobjects=forceNumRandomObjects;
    if (nobjects == 0) return;
    bool simple                  =   GetOptionValue<bool>(OBJECT_SIMPLE);
    bool only_2d                 =       GetOptionValue<bool>(OBJECT_2D);
    std::string motion_path_type = GetOptionValue<String>(OBJECT_STATIC);
    float driftX                 = GetOptionValue<float>(OBJECT_DRIFT_X);
    float driftY                 = GetOptionValue<float>(OBJECT_DRIFT_Y);
    float driftZ                 = GetOptionValue<float>(OBJECT_DRIFT_Z);

    float percent_queriers       = GetOptionValue<float>(OBJECT_QUERY_FRAC);

    Vector3f driftVecDir(driftX,driftY,driftZ);


    for(uint32 i = 0; i < nobjects; i++) {
        UUID id = randomUUID();

        ObjectInputs* inputs = new ObjectInputs;

        Vector3f startpos = region.min() + Vector3f(randFloat()*region_extents.x, randFloat()*region_extents.y, (only_2d ? 0.5 : randFloat())*region_extents.z);

        double radval = forceRadius;
        if (!radval)
            radval=10;
        float bounds_radius = (simple ? radval : (randFloat()*2*radval));

        inputs->localID = mLocalIDSource++;

        if (motion_path_type == "static")//static
            inputs->motion = new StaticMotionPath(start, startpos);
        else if (motion_path_type == "drift") //drift
        {
          //   inputs->motion = new OSegTestMotionPath(start, end, startpos, 3, Duration::milliseconds((int64)1000), region, zfactor); // FIXME
          inputs->motion = new OSegTestMotionPath(start, end, startpos, 3, Duration::milliseconds((int64)1000), region, 0.5, driftVecDir); // FIXME
        }
        else //random
            inputs->motion = new RandomMotionPath(start, end, startpos, 3, Duration::milliseconds((int64)1000), region, (only_2d ? 0.0 : 1.0)); // FIXME
        inputs->bounds = BoundingSphere3f( Vector3f(0, 0, 0), bounds_radius );
        inputs->registerQuery = (randFloat() <= percent_queriers);
        inputs->queryAngle = SolidAngle(SolidAngle::Max / 900.f); // FIXME how to set this? variability by objects?
        inputs->connectAt = Duration::seconds(0.f);

        inputs->startTimer = Network::IOTimer::create(mContext->ioService);

        mObjectIDs.insert(id);
        mInputs[id] = inputs;
    }
}

/** Object packs are our own custom format for simple, fixed, object tests. The file format is
 *  a simple binary dump of each objects information:
 *
 *    struct ObjectInformation {
 *        uint64 object_id;
 *        double x;
 *        double y;
 *        double z;
 *        double radius;
 *    };
 *
 *  This gives the minimal information for a static object and allows you to seek directly to any
 *  object in the file, making it easy to split the file across multiple object hosts.
 */
void ObjectFactory::generatePackObjects(const BoundingBox3f& region, const Duration& duration) {
    bool dump_pack = GetOptionValue<bool>(OBJECT_PACK_DUMP);
    if (dump_pack) return; // If we're dumping objects, don't load
                           // from a pack

    Time start(Time::null());
    Time end = start + duration;
    Vector3f region_extents = region.extents();

    uint32 nobjects = GetOptionValue<uint32>(OBJECT_PACK_NUM);
    if (nobjects == 0) return;
    String pack_filename = GetOptionValue<String>(OBJECT_PACK);
    assert(!pack_filename.empty());

    String pack_dir = GetOptionValue<String>(OBJECT_PACK_DIR);
    pack_filename = pack_dir + pack_filename;

    FILE* pack_file = fopen(pack_filename.c_str(), "rb");
    if (pack_file == NULL) {
        SILOG(objectfactory,error,"Couldn't open object pack file, not generating any objects.");
        assert(false);
        return;
    }

    // First offset ourselves into the file
    uint32 pack_offset = GetOptionValue<uint32>(OBJECT_PACK_OFFSET);

    uint32 obj_pack_size =
        8 + // objid
        8 + // radius
        8 + // x
        8 + // y
        8 + // z
        0;
    int seek_success = fseek( pack_file, obj_pack_size * pack_offset, SEEK_SET );
    if (seek_success != 0) {
        SILOG(objectfactory,error,"Couldn't seek to appropriate offset in object pack file.");
        fclose(pack_file);
        return;
    }

    for(uint32 i = 0; i < nobjects; i++) {
        ObjectInputs* inputs = new ObjectInputs;

        uint64 pack_objid = 0;
        double x = 0, y = 0, z = 0, rad = 0;
        fread( &pack_objid, sizeof(uint64), 1, pack_file );
        fread( &x, sizeof(double), 1, pack_file );
        fread( &y, sizeof(double), 1, pack_file );
        fread( &z, sizeof(double), 1, pack_file );
        fread( &rad, sizeof(double), 1, pack_file );

        UUID id = pack2UUID(pack_objid);

        Vector3f startpos((float)x, (float)y, (float)z);
        float bounds_radius = (float)rad;

        inputs->localID = mLocalIDSource++;
        inputs->motion = new StaticMotionPath(start, startpos);
        inputs->bounds = BoundingSphere3f( Vector3f(0, 0, 0), bounds_radius );
        inputs->registerQuery = false;
        inputs->queryAngle = SolidAngle::Max;
        inputs->connectAt = Duration::seconds(0.f);

        inputs->startTimer = Network::IOTimer::create(mContext->ioService);

        mObjectIDs.insert(id);
        mInputs[id] = inputs;
    }

    fclose(pack_file);
}

/** This is another simple format. This is purely textual, so easier to generate
 *  from scripting languages.  It contains a list of motion updates. Currently
 *  we just load the first (by time) update for each object. Objects are static,
 *  and we use OBJECT_SL_NUM parameter to control how many we add. Note,
 *  however, that we load them sorted by an extra parameter, distance from
 *  OBJECT_SL_CENTER so we add them with increasing distance. We reuse the
 *  OBJECT_PACK_DIR to specify where to find the files.
 */
struct SLEntry {
    UUID uuid;
    Vector3f pos;
    float rad;
    uint32 t;
};
typedef std::map<uint32, SLEntry> ObjectUpdateList; // Want ordered so we can pick out the first
typedef std::map<UUID, ObjectUpdateList> TraceObjectMap;
struct SortObjectUpdateListByDist {
    SortObjectUpdateListByDist(Vector3f orig) : o(orig) {}

    bool operator()(const ObjectUpdateList& lhs, const ObjectUpdateList& rhs) {
        return ( (lhs.begin()->second.pos-o).lengthSquared() < (rhs.begin()->second.pos-o).lengthSquared());
    }

    Vector3f o;
};

void ObjectFactory::generateStaticTraceObjects(const BoundingBox3f& region, const Duration& duration) {
    Time start(Time::null());
    Time end = start + duration;
    Vector3f region_extents = region.extents();

    uint32 nobjects = GetOptionValue<uint32>(OBJECT_SL_NUM);
    if (nobjects == 0) return;
    String pack_filename = GetOptionValue<String>(OBJECT_SL_FILE);
    assert(!pack_filename.empty());
    String pack_dir = GetOptionValue<String>(OBJECT_PACK_DIR);
    pack_filename = pack_dir + pack_filename;

    Vector3f sim_center = GetOptionValue<Vector3f>(OBJECT_SL_CENTER);

    // First, load in all the objects.
    FILE* pack_file = fopen(pack_filename.c_str(), "rb");
    if (pack_file == NULL) {
        SILOG(objectfactory,error,"Couldn't open object pack file, not generating any objects.");
        assert(false);
        return;
    }

    TraceObjectMap trace_objects;
    // parse each line: uuid x y z t rad
    SLEntry ent;
    char uuid[256];
    while( !feof(pack_file) && fscanf(pack_file, "%s %f %f %f %d %f", uuid, &ent.pos.x, &ent.pos.y, &ent.pos.z, &ent.t, &ent.rad) ) {
        ent.uuid = UUID(std::string(uuid), UUID::HumanReadable());

        TraceObjectMap::iterator obj_it = trace_objects.find(ent.uuid);
        if (obj_it == trace_objects.end()) {
            trace_objects[ent.uuid] = ObjectUpdateList();
            obj_it = trace_objects.find(ent.uuid);
        }
        obj_it->second[ent.t] = ent;
    }
    fclose(pack_file);

    // Now get a version sorted by distance from
    typedef std::set<ObjectUpdateList, SortObjectUpdateListByDist> ObjectsByDistanceList;
    ObjectsByDistanceList objs_by_dist = ObjectsByDistanceList( SortObjectUpdateListByDist(sim_center) );
    for(TraceObjectMap::iterator obj_it = trace_objects.begin(); obj_it != trace_objects.end(); obj_it++)
        objs_by_dist.insert(obj_it->second);

    // Finally, for the number of objects requested, insert the data
    ObjectsByDistanceList::iterator obj_it = objs_by_dist.begin();
    for(uint32 i = 0; i < nobjects; i++, obj_it++) {
        ObjectInputs* inputs = new ObjectInputs;
        SLEntry first = (obj_it->begin())->second;

        inputs->localID = mLocalIDSource++;
        inputs->motion = new StaticMotionPath(Time::microseconds(first.t*1000), first.pos);
        inputs->bounds = BoundingSphere3f( Vector3f(0, 0, 0), first.rad );
        inputs->registerQuery = false;
        inputs->queryAngle = SolidAngle::Max;
        inputs->connectAt = Duration::seconds(0.f);

        inputs->startTimer = Network::IOTimer::create(mContext->ioService);

        mObjectIDs.insert(first.uuid);
        mInputs[first.uuid] = inputs;
    }
}

void ObjectFactory::dumpObjectPack() const {
    bool dump_pack = GetOptionValue<bool>(OBJECT_PACK_DUMP);
    if (!dump_pack) return;

    String pack_filename = GetOptionValue<String>(OBJECT_PACK);
    assert(!pack_filename.empty());

    FILE* pack_file = fopen(pack_filename.c_str(), "wb");
    if (pack_file == NULL) {
        SILOG(objectfactory,error,"Couldn't open object pack file, not dumping any objects.");
        assert(false);
        return;
    }

    uint64 pack_id_gen = 1;
    for(ObjectInputsMap::const_iterator it = mInputs.begin(); it != mInputs.end(); it++) {
        uint64 pack_objid = pack_id_gen++;
        ObjectInputs* inputs = it->second;

        fwrite( &pack_objid, sizeof(uint64), 1, pack_file );
        Vector3f startpos = inputs->motion->initial().position();
        float64 x = startpos.x, y = startpos.y, z = startpos.z;
        fwrite( &(x), sizeof(x), 1, pack_file );
        fwrite( &(y), sizeof(x), 1, pack_file );
        fwrite( &(z), sizeof(x), 1, pack_file );
        float64 rad = inputs->bounds.radius();
        fwrite( &rad, sizeof(rad), 1, pack_file );
    }

    fclose(pack_file);
}

void ObjectFactory::setConnectTimes() {
    Duration total_duration = GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);

    for(ObjectInputsMap::iterator it = mInputs.begin(); it != mInputs.end(); it++) {
        ObjectInputs* inputs = it->second;
        inputs->connectAt = total_duration * ((float)inputs->localID/(float)mLocalIDSource);
    }
}

MotionPath* ObjectFactory::motion(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->motion;
}

BoundingSphere3f ObjectFactory::bounds(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->bounds;
}

bool ObjectFactory::registerQuery(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->registerQuery;
}

SolidAngle ObjectFactory::queryAngle(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->queryAngle;
}

Object* ObjectFactory::object(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );

    ObjectMap::iterator it = mObjects.find(id);
    if (it != mObjects.end()) return it->second;

    Object* new_obj = new Object(this, id, motion(id), bounds(id), registerQuery(id), queryAngle(id), mContext);
    mObjects[id] = new_obj;
    return new_obj;
}


void ObjectFactory::start() {
    for(ObjectInputsMap::iterator it = mInputs.begin(); it != mInputs.end(); it++) {
        UUID objid = it->first;
        ObjectInputs* inputs = it->second;

        inputs->startTimer->wait(
            inputs->connectAt,
            mContext->mainStrand->wrap( std::tr1::bind(&ObjectFactory::handleObjectInit, this, objid) )
        );
    }
}

void ObjectFactory::handleObjectInit(const UUID& objid) {
    Object* obj = object(objid);
    obj->start();
}

void ObjectFactory::stop() {
    // Stop the timers which start new objects
    for(ObjectInputsMap::iterator it = mInputs.begin(); it != mInputs.end(); it++) {
        ObjectInputs* inputs = it->second;
        inputs->startTimer->cancel();
    }

    // Stop all objects
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        Object* obj = it->second;
        obj->stop();
    }
}

void ObjectFactory::notifyDestroyed(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mObjects.find(id) != mObjects.end() );

    mObjects.erase(id);
}

} // namespace Sirikata
