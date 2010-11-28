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

#ifndef _SIRIKATA_MESSAGE_HPP_
#define _SIRIKATA_MESSAGE_HPP_

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {

typedef uint64 UniqueMessageID;

template<typename PBJMessageType>
std::string serializePBJMessage(const PBJMessageType& contents) {
    std::string payload;
    bool serialized_success = contents.SerializeToString(&payload);
    assert(serialized_success);

    return payload;
}

template<typename PBJMessageType>
bool serializePBJMessage(std::string* payload, const PBJMessageType& contents) {
    return contents.SerializeToString(payload);
}

/** Parse a PBJ message from the wire, starting at the given offset from the .
 *  \param contents pointer to the message to parse from the data
 *  \param wire the raw data to parse from, either a Network::Chunk or std::string
 *  \param offset the number of bytes into the data to start parsing
 *  \param length the number of bytes to parse, or -1 to use the entire
 *  \returns true if parsed successfully, false otherwise
 */
template<typename PBJMessageType, typename WireType>
bool parsePBJMessage(PBJMessageType* contents, const WireType& wire, uint32 offset = 0, int32 length = -1) {
    assert(contents != NULL);
    uint32 rlen = (length == -1) ? (wire.size() - offset) : length;
    assert(offset + rlen <= wire.size()); // buffer overrun
    return contents->ParseFromArray((void*)&wire[offset], rlen);
}


#define MESSAGE_ID_SERVER_SHIFT 52
#define MESSAGE_ID_SERVER_BITS 0xFFF0000000000000LL

namespace {

uint64 sIDSource = 0;

uint64 GenerateUniqueID(const ServerID& origin) {
    uint64 id_src = sIDSource++;
    uint64 message_id_server_bits=MESSAGE_ID_SERVER_BITS;
    uint64 server_int = (uint64)origin;
    uint64 server_shifted = server_int << MESSAGE_ID_SERVER_SHIFT;
    assert( (server_shifted & ~message_id_server_bits) == 0 );
    return (server_shifted & message_id_server_bits) | (id_src & ~message_id_server_bits);
}

uint64 GenerateUniqueID(const ObjectHostID& origin) {
    return GenerateUniqueID(origin.id);
}

ServerID GetUniqueIDServerID(uint64 uid) {
    uint64 message_id_server_bits=MESSAGE_ID_SERVER_BITS;
    uint64 server_int = ( uid & message_id_server_bits ) >> MESSAGE_ID_SERVER_SHIFT;
    return (ServerID) server_int;
}

uint64 GetUniqueIDMessageID(uint64 uid) {
    uint64 message_id_server_bits=MESSAGE_ID_SERVER_BITS;
    return ( uid & ~message_id_server_bits );
}

}

#define LOG_INVALID_MESSAGE(module, lvl, msg)                           \
    SILOG(module, lvl, "Error parsing message at " << __FILE__ << ":" << __LINE__ << " Contents: (" << msg.size() << " bytes)"); \
    for(int _i__ = 0; _i__ < (int)msg.size(); _i__++)                   \
        SILOG(module, lvl, "  " << (int)msg[_i__] )

#define LOG_INVALID_MESSAGE_BUFFER(module, lvl, msg_begin, msg_size)    \
    SILOG(module, lvl, "Error parsing message at " << __FILE__ << ":" << __LINE__ << " Contents: (" << msg_size << " bytes)"); \
    for(int _i__ = 0; _i__ < (int)msg_size; _i__++)                     \
        SILOG(module, lvl, "  " << (int)msg_begin[_i__] )

} // namespace Sirikata

#endif //_SIRIKATA_MESSAGE_HPP_
