/*  Sirikata
 *  Address4.cpp
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

#include <sirikata/cbrcore/Address4.hpp>

#include <netdb.h>
namespace Sirikata{

Address4 Address4::Null = Address4(0,0);

Address4::Address4(const Sirikata::Network::Address&a){
    hostent*addrs=gethostbyname(a.getHostName().c_str());
    if (addrs) {
        if (addrs->h_addr_list[0]) {
            memset(&this->ip,0,sizeof(this->ip));
            if (addrs->h_length<=(int)sizeof(this->ip)) {
                memcpy(&this->ip,addrs->h_addr_list[0],addrs->h_length);
            }else {
                fprintf (stderr,"Error translating address %s: please specify with dot notation \n",a.getHostName().c_str());
            }
        }
    }
    this->port=atoi(a.getService().c_str());
}
using namespace Sirikata::Network;
Address convertAddress4ToSirikata(const Address4&addy) {
    std::stringstream port;
    port << addy.getPort();
    std::stringstream hostname;
    uint32 mynum=addy.ip;
    unsigned char bleh[4];
    memcpy(bleh,&mynum,4);

    hostname << (unsigned int)bleh[0]<<'.'<<(unsigned int)bleh[1]<<'.'<<(unsigned int)bleh[2]<<'.'<<(unsigned int)bleh[3];

    return Address(hostname.str(),port.str());
}

}
