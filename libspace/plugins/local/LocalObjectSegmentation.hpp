/*  Sirikata
 *  LocalObjectSegmentation.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_LOCAL_OBJECT_SEGMENTATION_HPP_
#define _SIRIKATA_LOCAL_OBJECT_SEGMENTATION_HPP_

#include <sirikata/space/ObjectSegmentation.hpp>

namespace Sirikata {

class LocalObjectSegmentation : public ObjectSegmentation {
public:
    LocalObjectSegmentation(SpaceContext* con, Network::IOStrand* o_strand, CoordinateSegmentation* cseg, OSegCache* cache);

    virtual OSegEntry cacheLookup(const UUID& obj_id);
    virtual OSegEntry lookup(const UUID& obj_id);

    virtual void addNewObject(const UUID& obj_id, float radius);
    virtual void addMigratedObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool);
    virtual void removeObject(const UUID& obj_id);

    virtual bool clearToMigrate(const UUID& obj_id);
    virtual void migrateObject(const UUID& obj_id, const OSegEntry& new_server_id);
private:
    CoordinateSegmentation* mCSeg;
    OSegCache* mCache;

    typedef std::tr1::unordered_map<UUID, OSegEntry, UUID::Hasher> OSegMap;
    OSegMap mOSeg;
};

} // namespace Sirikata

#endif //_SIRIKATA_LOCAL_OBJECT_SEGMENTATION_HPP_
