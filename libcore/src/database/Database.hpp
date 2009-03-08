/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  UUID.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#ifndef _SIRIKATA_DATABASE_HPP_
#define _SIRIKATA_DATABASE_HPP_

namespace Sirikata { namespace Database {
typedef MemoryBuffer Value;
class Key {
    UUID mObject;
    uint32 mField;
public:
    Key(const UUID&obj_uuid, uint32 field)
        :mObject(obj_uuid),mField(field),mPath(path){

    }
    class Hasher{
        const operator()(const Key&key)const {
            return mObject.hash()^std::tr1::hash<uint32>(mField);
        }
    };
    bool operator <(const Key&other) const{
        return mObject==other.mObject?mField<other.mField:mObject<other.mObject;        
    }
    bool operator ==(const Key&other) const{
        return mObject==other.mObject&&mField==other.mField;
    }
};


class KeyValueMap {
    std::map<Key,Value*> mSet;
public:
    ///add key-value mapping to the map
    void insert(const Key&key, const Value&value);
    ///add key-value mapping to the map, ceding ownership of Value to KeyValueMap
    void insert(const Key&key, Value*value);
};
class Database {
public:
    

};

}
}
#endif
