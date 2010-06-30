/*  Sirikata
 *  Address4.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#ifndef _SIRIKATA_ADDRESS4_HPP_
#define _SIRIKATA_ADDRESS4_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Address.hpp>

namespace Sirikata {

class SIRIKATA_EXPORT Address4 {
public:
    unsigned int ip;
    unsigned short port;
    class Hasher{
    public:
        size_t operator() (const Address4&addy)const {
            return std::tr1::hash<unsigned int>()(addy.ip^addy.port);
        }
    };
    Address4() {
        ip=port=0;
    }
    Address4(const Sirikata::Network::Address&a);
    Address4(unsigned int ip, unsigned short prt) {
        this->ip=ip;
        this->port=prt;
    }
    bool operator ==(const Address4&other)const {
        return ip==other.ip&&port==other.port;
    }
    bool operator!=(const Address4& other) const {
        return ip != other.ip || port != other.port;
    }
    bool operator<(const Address4& other) const {
        return (ip < other.ip) || (ip == other.ip && port < other.port);
    }
    uint16 getPort() const {
        return port;
    }

    static Address4 Null;
};

Sirikata::Network::Address SIRIKATA_EXPORT convertAddress4ToSirikata(const Address4&addy);

inline size_t hash_value(const Address4&addy) {
    return std::tr1::hash<unsigned int>()(addy.ip)^std::tr1::hash<unsigned short>()(addy.port);
}

}
#endif //_SIRIKATA_ADDRESS4_HPP_
