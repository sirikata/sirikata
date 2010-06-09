/*  Sirikata
 *  TabularServerIDMap.hpp
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

#ifndef _SIRIKATA_TABULARSERVERID_MAP_HPP_
#define _SIRIKATA_TABULARSERVERID_MAP_HPP_

#include "Server.hpp"
#include "ServerIDMap.hpp"
#include <fstream>
namespace Sirikata {

/* Represents the physical network addresses of servers
 */
class TabularServerIDMap:public ServerIDMap {
    std::tr1::unordered_map<ServerID,Address4> mInternalIDMap;
    std::tr1::unordered_map<Address4,ServerID,Address4::Hasher> mInternalAddressMap;

    std::tr1::unordered_map<ServerID,Address4> mExternalIDMap;
    std::tr1::unordered_map<Address4,ServerID,Address4::Hasher> mExternalAddressMap;
public:
    TabularServerIDMap(std::istream&filestream);
    virtual ~TabularServerIDMap() {}

    virtual ServerID* lookupInternal(const Address4& pos);
    virtual Address4* lookupInternal(const ServerID& obj_id);

    virtual ServerID* lookupExternal(const Address4& pos);
    virtual Address4* lookupExternal(const ServerID& obj_id);
};

} // namespace Sirikata

#endif //_SIRIKATA_SERVER_MAP_HPP_
