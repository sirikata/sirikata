/*  Sirikata
 *  ObjectReference.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _OBJECT_REFERENCE_HPP_
#define _OBJECT_REFERENCE_HPP_

#include "UUID.hpp"

namespace Sirikata {
class ObjectReference;
} // namespace Sirikata

namespace Mono {
struct CSharpObjectReference;
void ConvertObjectReference(const Sirikata::ObjectReference& in, CSharpObjectReference* out);
} // namespace Mono

namespace Sirikata {

/** A reference to an object in a space.  This can be used as
 *  an address to send a message to an object via a space.
 *  ObjectReferences are specific to spaces, i.e. they are not
 *  guaranteed to work in spaces other than the one they originated
 *  in.
 */
class ObjectReference : TotallyOrdered<ObjectReference>{
  public:
    enum {static_size=UUID::static_size};
    ObjectReference(){}
    explicit ObjectReference(const UUID& id):mID(id){}
    explicit ObjectReference(const String& str):mID(str,UUID::HumanReadable()){}
    explicit ObjectReference(const UUID::Data &data):mID(data){}

    UUID::Data toRawBytes() const
    { return mID.getArray(); }

    String toRawHexData() const
    { return mID.rawHexData(); }

    static const ObjectReference &null(){
        static ObjectReference retval(UUID::null());
        return retval;
    }
    static const ObjectReference &spaceServiceID() {
        return null();
    }
    String toString() const{
        return mID.readableHexData();
    }
    class Hasher {public:
        size_t operator()(const ObjectReference&objr) const {
             return objr.hash();
        }
    };
    size_t hash() const{
        return mID.hash();
    }

    /** Get the %UUID of the object referred to. This is not necessarily space-specific, it is just the value used to
     *  construct this reference. Note the contrast with getAsUUID().
     */
    const UUID& getObjectUUID() const{
        return mID;
    }

    /** Get this object reference in the form of an %UUID. This value is space-specific and used in contexts where objects
     *  need to be addressed by some form of %UUID, e.g. for message routing. Note the contrast with getObjectUUID().
     */
    UUID getAsUUID() const {
        return mID;
    }

    bool operator==(const ObjectReference& rhs) const{
        return mID==rhs.mID;
    }
    bool operator<(const ObjectReference& rhs) const{
        return mID<rhs.mID;
    }

  private:

    friend void Mono::ConvertObjectReference(const Sirikata::ObjectReference& in, CSharpObjectReference* out);

    UUID mID;

};

inline std::ostream &operator << (std::ostream &os, const ObjectReference&objRef) {
    os << objRef.toString();
    return os;
}

} // namespace Sirikata

#endif //_OBJECT_REFERENCE_HPP_
