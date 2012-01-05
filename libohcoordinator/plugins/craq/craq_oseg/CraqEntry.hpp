/*  Sirikata
 *  CraqEntry.hpp
 *
 *  Copyright (c) 2010, Behram Mistree
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

#ifndef _CRAQ_ENTRY_HPP_
#define _CRAQ_ENTRY_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/ObjectSegmentation.hpp>

namespace Sirikata {
enum {
    CRAQ_SERVER_SIZE             =    10
};
class CraqEntry : public OSegEntry {
public:
    CraqEntry(const OSegEntry& rhs)
     : OSegEntry(rhs) {}
    CraqEntry(uint32 server, float radius)
     : OSegEntry(server, radius) {
    }
    static CraqEntry null() {
        return CraqEntry(NullServerID,0);
    }
    explicit CraqEntry(unsigned char input[CRAQ_SERVER_SIZE])
     : OSegEntry(NullServerID, 0) {
        deserialize(input);
    }
    void serialize(unsigned char output[CRAQ_SERVER_SIZE])const;
    std::string serialize() const{
        char output[CRAQ_SERVER_SIZE];
        serialize((unsigned char*)output);
        return std::string(output,CRAQ_SERVER_SIZE);
    }
    void deserialize(unsigned char input[CRAQ_SERVER_SIZE]);
};
}
#endif
