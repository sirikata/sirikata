/*  cbr
 *  main.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "ObjectFactory.hpp"
#include "LocationService.hpp"
#include "ServerMap.hpp"
#include "Proximity.hpp"
#include "Server.hpp"

#include "OracleLocationService.hpp"
#include "UniformServerMap.hpp"
#include "Test.hpp"
int main(int argc, char** argv) {
    using namespace CBR;
    if (argc>1) {
        if (strcmp(argv[1],"-test")==0) {
            if (argc>2) {
                if (strcmp(argv[2],"server")==0) {
                    CBR::testServer(argc>3?argv[3]:"8080",argc>4?argv[4]:"127.0.0.1",argc>5?argv[5]:"8081");
                }else if (strcmp(argv[2],"client")==0) {
                    CBR::testClient(argc>3?argv[3]:"8081",argc>4?argv[4]:"127.0.0.1",argc>5?argv[5]:"8080");
                }
            }
        }
    }
    ObjectFactory* obj_factory = new ObjectFactory(100);
    LocationService* loc_service = new OracleLocationService(obj_factory);
    ServerMap* server_map = new UniformServerMap(
        loc_service,
        BoundingBox3f( Vector3f(0.f, 0.f, 0.f), Vector3f(1.f, 1.f, 1.f) ),
        Vector3ui32(1, 1, 1)
    );
    Proximity* prox = new Proximity();
    Server* server = new Server(1, obj_factory, loc_service, server_map, prox);

    // FIXME this is just for testing.  we should be using a real timer
    for(Time t = Time(0); t < Time(0) + Duration::seconds((uint32)1); t += Duration::milliseconds((uint32)10))
        server->tick(t);

    delete server;
    delete prox;
    delete loc_service;
    delete obj_factory;

    return 0;
}
