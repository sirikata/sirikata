/*  Sirikata
 *  Message.hpp
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

#ifndef _SIRIKATA_OBJECT_MESSAGE_HPP_
#define _SIRIKATA_OBJECT_MESSAGE_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>

#include "Message.hpp"
#include "Protocol_ObjectMessage.pbj.hpp"


namespace Sirikata {

typedef uint16 ObjectMessagePort;

// List of well known object ports.
#define OBJECT_PORT_SESSION       1
#define OBJECT_PORT_PROXIMITY     2
#define OBJECT_PORT_LOCATION      3
#define OBJECT_PORT_TIMESYNC      4
#define OBJECT_SPACE_PORT         253
#define OBJECT_PORT_PING          254

#define MESSAGE_ID_SERVER_SHIFT 52
#define MESSAGE_ID_SERVER_BITS 0xFFF0000000000000LL

SIRIKATA_FUNCTION_EXPORT Sirikata::Protocol::Object::ObjectMessage* createObjectMessage(ServerID source_server, const SpaceObjectReference& sporef_src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload);

SIRIKATA_FUNCTION_EXPORT Sirikata::Protocol::Object::ObjectMessage* createObjectMessage(ServerID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload);


// Wrapper class for Protocol::Object::Message which provides it some missing methods
// that are useful, e.g. size().
class SIRIKATA_EXPORT ObjectMessage : public Sirikata::Protocol::Object::ObjectMessage {
public:
    uint32 size() {
        return this->ByteSize();
    }

    void serialize(std::string* result) const {
        serializePBJMessage(result, *this);
    }

    std::string* serialize() const {
        return new std::string( serializePBJMessage(*this) );
    };
}; // class ObjectMessage

// FIXME get rid of this
SIRIKATA_FUNCTION_EXPORT void createObjectHostMessage(ObjectHostID source_server, const SpaceObjectReference& sporef_src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload, ObjectMessage* result);

SIRIKATA_FUNCTION_EXPORT void createObjectHostMessage(ObjectHostID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload, ObjectMessage* result);


} // namespace Sirikata

#endif //_SIRIKATA_OBJECT_MESSAGE_HPP_
