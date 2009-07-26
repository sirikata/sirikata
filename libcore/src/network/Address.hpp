/*  Sirikata Network Utilities
 *  Address.hpp
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
#ifndef _NETWORK_ADDRESS_HPP_
#define _NETWORK_ADDRESS_HPP_
namespace Sirikata { namespace Network {
/**
 * Provides a protocol-independent address layer for connecting to a target
 * Takes a name and a service (for TCP that would be computer ip address and numeric port
 */
class Address {
    String mName;
    String mService;
public:
    static Any lexical_cast(const std::string&value) {
        std::string::size_type where=value.rfind(':');
        if (where==std::string::npos) {
            throw std::invalid_argument("Address does not contain a port");
        }
        Address retval(value.substr(0,where),
                       value.substr(where+1));
        return retval;
    }
    const String &getHostName()const {
        return mName;
    }
    const String &getService()const {
        return mService;
    }
    static Address null() {
        return Address(String(),String());
    }
    bool operator==(const Address&other) const{
        return other.mName==mName&&other.mService==mService;
    }
    bool operator<(const Address&other) const{
        return other.mName==mName?mService<other.mService:mName<other.mName;
    }
    Address(const String& name, 
            const String& service) {
        mName=name;
        mService=service;
    }
    class Hasher {public:
        size_t operator()(const Address&addy)const {
            return std::tr1::hash<std::string>()(addy.mName)^std::tr1::hash<std::string>()(addy.mService);
        }
    };
};
} }
#endif
