/*  Sirikata
 *  Test.cpp
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

#include "Test.hpp"
#include "TCPSpaceNetwork.hpp"
#include "TabularServerIDMap.hpp"
#include "Statistics.hpp"
#include <arpa/inet.h>
namespace Sirikata {

void testAny(const char * listenport, const char* hostname, const char* port, bool server) {
    std::stringstream strst( std::string(hostname) + ":" + std::string(listenport) + std::string("\n") );
    Trace trace("test.trace");

    IOService* ios = IOServiceFactory::makeIOService();
    IOStrand* mainStrand = ios->createStrand();



    SpaceContext ctx(0, ios, mainStrand, Time::null(), Time::null(), &trace, Duration::seconds(0));
    TCPNetwork rn(&ctx);
    rn.listen(Address4(Sirikata::Network::Address("localhost", "6666")), NULL);
    bool canSend=!server;
    unsigned int mine=0;
    unsigned int theirs=0;
    Sirikata::Network::Chunk toSend(4);
    unsigned int offset=15;
    unsigned int maxval=65637;
    while (mine<maxval||theirs<maxval) {
        Sirikata::Network::Chunk *c;
        while ((c=rn.receiveOne(Address4::Null, 1000000))) {
            canSend=true;
            unsigned int network;
            if (c->size()>=4) {
                memcpy(&network,&*c->begin(),4);
                network=ntohl(network);
                assert(network==theirs);
                theirs+=offset;
            }
        }
        if (canSend) {
            unsigned int network=htonl(mine);
            mine+=offset;
            memcpy(&*toSend.begin(),&network,4);
            rn.send(Address4(Sirikata::Network::Address(hostname,port)),toSend);
        }
    }
    sleep (1);
    printf("Passed\n");
}
void testServer(const char * listenport, const char* hostname, const char* port){

    testAny(listenport,hostname,port,true);
}
void testClient(const char * listenport, const char* hostname, const char* port){
    testAny(listenport,hostname,port,false);

}


}
