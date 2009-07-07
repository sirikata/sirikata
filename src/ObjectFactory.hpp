/*  cbr
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "Utility.hpp"
#include "Duration.hpp"
#include "Server.hpp"

namespace CBR {
class ObjectMessageQueue;
class MotionPath;
class Object;

/** Generates objects for the simulation.  This class actually has 2 jobs.
 *  First, it generates MotionPaths for every object that will exist in the
 *  system.  Second, when requested, it should generate an Object for performing
 *  full simulation of the object on the local server.
 */
class ObjectFactory {
    typedef std::set<UUID> ObjectIDSet;
    struct ObjectInputs {
        MotionPath* motion;
        BoundingSphere3f bounds;
        float proximityRadius; // XXX FIXME
    };
    typedef std::map<UUID, ObjectInputs*> ObjectInputsMap;
    typedef std::map<UUID, Object*> ObjectMap;
public:
    typedef ObjectIDSet::iterator iterator;
    typedef ObjectIDSet::const_iterator const_iterator;
    float getProximityRadius(UUID obj) {
        return mInputs[obj]->proximityRadius;
    }
    ObjectFactory(uint32 count, const BoundingBox3f& region, const Duration& duration);
    ~ObjectFactory();

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

    MotionPath* motion(const UUID& id);
    BoundingSphere3f bounds(const UUID& id);
    float proximityRadius(const UUID& id); // FIXME
    Object* object(const UUID& id, const ServerID& server);
    void destroyObject(const UUID& id);

    void setObjectMessageQueue(ObjectMessageQueue* sq);

private:
    ObjectIDSet mObjectIDs;
    ObjectInputsMap mInputs;
    ObjectMap mObjects;
    ObjectMessageQueue* mObjectMessageQueue;
}; // class ObjectFactory

} // namespace CBR
