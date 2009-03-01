/*  Sirikata Network Utilities
 *  Stream.cpp
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

#include "util/Standard.hh"
#include "Stream.hpp"
namespace Sirikata { namespace Network {
void Stream::ignoreSubstreamCallback(Stream * stream, SetCallbacks&) {
    delete stream;
}
void Stream::ignoreConnectionStatus(Stream::ConnectionStatus status, const std::string&) {
}
void Stream::ignoreBytesReceived(const Chunk&c) {
#if 0
    std::stringstream ss;
    ss<<"ignoring: ";
    for (size_t i=0;i<c.size();++i) {
        ss<<c[i];
    }
    SILOG(tcpsst,debug,ss.str());
#endif
}
unsigned int Stream::StreamID::serialize(uint8 *destination, unsigned int maxsize) const{
    assert (maxsize>=MAX_SERIALIZED_LENGTH);
    assert (mID< (1 <<30));
    uint32 temp=mID;
    if (temp<128){
        destination[0]=temp;
        return 1;
    }
    if (temp<16384){
        destination[0]=(uint8)((temp&127)|128);
        destination[1]=(uint8)(temp/128);
        return 2;
    }
    
    destination[0]=((temp&127)|128);
    destination[1]=(((temp/128)&127)|128);
    temp/=16384;
    destination[2]=(temp&255);
    destination[3]=((temp/256)&255);
    return 4;
}
bool Stream::StreamID::unserialize(const uint8* data, unsigned int &size) {
    if (size==0) return false;
    unsigned int tempvalue=data[0];
    if (tempvalue>=128) {
        if (size<2) return false;
        tempvalue&=127;
        unsigned int tempvalue1=data[1];
        if (tempvalue1>=128) {
            if (size<4) return false;
            size=4;
            tempvalue+=(tempvalue1&127)*128;
            tempvalue1=data[2];
            tempvalue+=(tempvalue1*16384);
            tempvalue1=data[3];
            tempvalue+=(tempvalue1*16384*256);
            mID=tempvalue;
        }else {
            size=2;
            mID=tempvalue|(tempvalue1*128);
        }
    }else {
        size=1;
        mID=tempvalue;
    }
    return true;
}

} }
