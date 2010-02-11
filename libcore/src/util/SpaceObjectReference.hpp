/*  Sirikata
 *  SpaceObjectReference.hpp
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
#ifndef _SPACE_OBJECT_REFERENCE_HPP_
#define _SPACE_OBJECT_REFERENCE_HPP_

#include "SpaceID.hpp"
#include "ObjectReference.hpp"

namespace Sirikata {

typedef uint32 MessagePort;

/** A reference to an object in a space.  This can be used as
 *  an address to send a message to an object via a space.
 *  SpaceObjectReferences are specific to spaces, i.e. they are not
 *  guaranteed to work in spaces other than the one they originated
 *  in.
 */
class SpaceObjectReference : TotallyOrdered<SpaceObjectReference>{
  public:
    SpaceObjectReference(){}
    SpaceObjectReference(const SpaceID&sid,
                         const ObjectReference&oref):mSpace(sid),
                                                     mObject(oref) {}
    explicit SpaceObjectReference(const String& humanReadable){
        String::size_type where=humanReadable.find(":");
        if (where==String::npos) {
            mObject=ObjectReference::null();
            mSpace=SpaceID::null();
            throw std::invalid_argument("Unable to find colon separator in SpaceObjectReference");
        }else {
            mObject=ObjectReference(humanReadable.substr(0,where));
            mSpace=SpaceID(humanReadable.substr(where+1));
        }
    }
    explicit SpaceObjectReference(const void * data,
                                  unsigned int size)
        :mSpace(UUID::Data::construct(reinterpret_cast<const byte*>(data),
                                     reinterpret_cast<const byte*>(data)+SpaceID::static_size)),
       mObject(UUID::Data::construct(reinterpret_cast<const byte*>(data)+SpaceID::static_size,
                                     reinterpret_cast<const byte*>(data)+SpaceID::static_size)){}

    Array<byte,SpaceID::static_size+ObjectReference::static_size> toRawBytes() const {
        Array<byte,SpaceID::static_size+ObjectReference::static_size> retval;
        std::memcpy(retval.begin(),space().toRawBytes().begin(),SpaceID::static_size);
        std::memcpy(retval.begin()+SpaceID::static_size,object().toRawBytes().begin(),ObjectReference::static_size);
        return retval;
    }

    String toRawHexData() const
    { return mSpace.toRawHexData()+mObject.toRawHexData(); }

    static const SpaceObjectReference &null(){
        static SpaceObjectReference retval(SpaceID::null(),
                                           ObjectReference::null());
        return retval;
    }

    String toString() const{
        return object().toString()+':'+space().toString();
    }
    unsigned int hash() const{
        return object().hash()^space().hash();
    }
    class Hasher{public:
        size_t operator() (const SpaceObjectReference&uuid) const {
            return uuid.hash();
        }
    };

    const ObjectReference&object() const {
        return mObject;
    }
    const SpaceID&space() const {
        return mSpace;
    }

    bool operator==(const SpaceObjectReference& rhs) const{
        return space()==rhs.space()&&object()==rhs.object();
    }
    bool operator<(const SpaceObjectReference& rhs) const{
        if (space()==rhs.space()) return object()<rhs.object();
        return space()<rhs.space();
    }

  private:

    SpaceID mSpace;
    ObjectReference mObject;

};

inline std::ostream &operator << (std::ostream &os, const SpaceObjectReference&sor) {
    os << sor.space() << ":" << sor.object();
    return os;
}

} // namespace Sirikata

#endif //_OBJECT_REFERENCE_HPP_
