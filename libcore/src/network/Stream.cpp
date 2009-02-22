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
    fprintf( stderr, "ignoring: ");
    for (size_t i=0;i<c.size();++i) {
        fprintf(stderr,"%c",c[i]);
    }
    fprintf(stderr,"\n");
#endif
}
unsigned int Stream::StreamID::serialize(uint8 *destination, unsigned int maxsize) const{
    if (mID<128){
        if (maxsize<1) return 1;
        destination[0]=mID;
        return 1;
    }else if (mID<127*256+255) {
        if (maxsize<2) return 2;
        destination[1]=mID%256;
        destination[0]=(mID/256)+128;
        return 2;
    }else {
        if (maxsize<6) return 6;
        destination[0]=255;
        destination[1]=255;
        destination[5]=mID%256;
        destination[4]=(mID/256)%256;
        destination[3]=(mID/256/256)%256;
        destination[2]=(mID/256/256/256)%256;
        return 6;
    }
}
bool Stream::StreamID::unserialize(const uint8* data, unsigned int &size) {
    if (size==0) return false;
    unsigned int tempvalue=data[0];
    if (tempvalue>=128) {
        if (size<2) return false;
        tempvalue-=128;
        tempvalue*=256;
        tempvalue+=data[1];
        if (tempvalue==127*256+255) {
            if (size<6) return false;
            size=6;
            tempvalue=data[2];
            tempvalue*=256;
            tempvalue+=data[3];
            tempvalue*=256;
            tempvalue+=data[4];
            tempvalue*=256;
            tempvalue+=data[5];
            mID=tempvalue;
        }else{
            size=2;
            mID=tempvalue;
        }
    }else {
        mID=tempvalue;
        size=1;
    }
    return true;
}

} }
