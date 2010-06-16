/*  Sirikata
 *  ObjectFactory.hpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/cbrcore/PollingService.hpp>
#include <sirikata/cbrcore/TimeProfiler.hpp>

namespace Sirikata {
class MotionPath;
class Object;
class Trace;
class ObjectHostContext;

/** Generates objects for the simulation.  This class actually has 2 jobs.
 *  First, it generates MotionPaths for every object that will exist in the
 *  system.  Second, when requested, it should generate an Object for performing
 *  full simulation of the object on the local server.
 */
class ObjectFactory : public Service {
    typedef std::set<UUID> ObjectIDSet;
    struct ObjectInputs {
        uint32 localID;
        MotionPath* motion;
        BoundingSphere3f bounds;
        bool registerQuery;
        SolidAngle queryAngle;
        Duration connectAt;

        Network::IOTimerPtr startTimer;
    };
    typedef std::tr1::unordered_map<UUID, ObjectInputs*,UUID::Hasher> ObjectInputsMap;
    typedef std::tr1::unordered_map<UUID, Object*,UUID::Hasher> ObjectMap;

public:
    ObjectFactory(ObjectHostContext* ctx, const BoundingBox3f& region, const Duration& duration);
    ~ObjectFactory();

private:
    Object* object(const UUID& id);

    MotionPath* motion(const UUID& id);
    BoundingSphere3f bounds(const UUID& id);

    virtual void start();
    virtual void stop();

    void generateRandomObjects(const BoundingBox3f& region, const Duration& duration);
    void generatePackObjects(const BoundingBox3f& region, const Duration& duration);
    void generateStaticTraceObjects(const BoundingBox3f& region, const Duration& duration);

    // Generates connection initiation times for all objects *after* they have been created
    void setConnectTimes();
    // If requested, dumps objects to
    void dumpObjectPack() const;

    void handleObjectInit(const UUID& objid);

    bool registerQuery(const UUID& id);
    SolidAngle queryAngle(const UUID& id);


    friend class Object;
    void notifyDestroyed(const UUID& id); // called by objects when they are destroyed

    ObjectHostContext* mContext;
    uint32 mLocalIDSource;
    ObjectIDSet mObjectIDs;
    ObjectInputsMap mInputs;
    ObjectMap mObjects;
}; // class ObjectFactory

} // namespace Sirikata
