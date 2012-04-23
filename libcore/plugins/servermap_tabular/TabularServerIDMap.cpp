/*  Sirikata
 *  TabularServerIDMap.cpp
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

#include <sirikata/core/network/Address4.hpp>
#include "TabularServerIDMap.hpp"
#include <sirikata/core/util/Random.hpp>

namespace Sirikata {


TabularServerIDMap::TabularServerIDMap(Context* ctx, std::istream&filestream)
 : ServerIDMap(ctx)
{
    int count=1;
    while(!filestream.bad() && !filestream.fail() && !filestream.eof()) {
        char ip[1025];
        ip[1024]='\0';
        char service[1025];
        service[1024]='\0';
        char ohservice[1025];
        ohservice[1024]='\0';
        filestream.getline(ip,1024,':');
        if (filestream.eof()) break;
        filestream.getline(service,1024,':');
        if (filestream.eof()) break;
        filestream.getline(ohservice,1024,'\n');

        Address4 internal_addy(Sirikata::Network::Address(ip,service));
        mInternalIDMap[count] = internal_addy;

        Address4 external_addy(Sirikata::Network::Address(ip,ohservice));
        mExternalIDMap[count] = external_addy;

	count++;
    }
}

Address4 TabularServerIDMap::lookupInternal(const ServerID& server_id){
    if (mInternalIDMap.find(server_id)!=mInternalIDMap.end())
        return mInternalIDMap.find(server_id)->second;
    return Address4::Null;
}

void TabularServerIDMap::lookupInternal(const ServerID& sid, Address4LookupCallback cb) {
    mContext->ioService->post(std::tr1::bind(cb, sid, lookupInternal(sid)), "TabularServerIDMap::lookupInternal");
}

Address4 TabularServerIDMap::lookupExternal(const ServerID& server_id){
    if (mExternalIDMap.find(server_id)!=mExternalIDMap.end())
        return mExternalIDMap.find(server_id)->second;
    return Address4::Null;
}

void TabularServerIDMap::lookupExternal(const ServerID& sid, Address4LookupCallback cb) {
    mContext->ioService->post(std::tr1::bind(cb, sid, lookupExternal(sid)), "TabularServerIDMap::lookupExternal");
}

void TabularServerIDMap::lookupRandomExternal(Address4LookupCallback cb) {
    int32 selected_server = randInt<int32>(0, mExternalIDMap.size()-1);
    ServerToAddressMap::iterator it = mExternalIDMap.begin();
    for(int32 i = 0; i < selected_server; i++) it++;
    mContext->ioService->post(std::tr1::bind(cb, it->first, it->second), "TabularServerIDMap::lookupExternal");
}

}//end namespace sirikata
