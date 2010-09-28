/*  Sirikata
 *  PBJDebug.cpp
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

#include <sirikata/core/network/PBJDebug.hpp>

#include "Protocol_Empty.pbj.hpp"

namespace Sirikata {

namespace {
bool printPBJMessage(const std::string& payload, const std::string& indent) {
    const char* indent_c = indent.c_str();
    Sirikata::Protocol::Empty msg;
    if (!msg.ParseFromArray(&payload[0], payload.size()))
        return false;

    printf("%s %d fields\n", indent_c, msg.unknown_fields().field_count());

    for(int i = 0; i < msg.unknown_fields().field_count(); i++) {
#define __FIELD(i) msg.unknown_fields().field(i)
        switch(__FIELD(i).type()) {
          case ::google::protobuf::UnknownField::TYPE_VARINT:
            printf("%s %d varint %ld\n", indent_c, __FIELD(i).number(), __FIELD(i).varint());
            break;
          case ::google::protobuf::UnknownField::TYPE_FIXED32:
            printf("%s %d fixed32 %d\n", indent_c, __FIELD(i).number(), __FIELD(i).fixed32());
            break;
          case ::google::protobuf::UnknownField::TYPE_FIXED64:
            printf("%s %d fixed64 %ld\n", indent_c, __FIELD(i).number(), __FIELD(i).fixed64());
            break;
          case ::google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED:
            printf("%s %d length %d\n", indent_c, __FIELD(i).number(), (int) __FIELD(i).length_delimited().size());
            printPBJMessage(__FIELD(i).length_delimited(), indent + " ");
            break;
          case ::google::protobuf::UnknownField::TYPE_GROUP:
            printf("%s %d group\n", indent_c, __FIELD(i).number());
            // FIXME recurse
            break;
          default:
            printf("%s %d unknown\n", indent_c, __FIELD(i).number());
            break;
        };
    }
    return true;
}
}

bool printPBJMessageString(const std::string& msg) {
    bool parsed = printPBJMessage(msg, "");
    if (!parsed)
        printf("Parsing PBJ failed.\n");
    return parsed;
}

bool printPBJMessageArray(const Sirikata::Network::Chunk& msg_ary) {
    std::string msg((const char*)&(msg_ary[0]), msg_ary.size());
    return printPBJMessageString(msg);
}


} // namespace Sirikata
