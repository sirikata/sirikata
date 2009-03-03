#include "Test.hpp"
#include "RaknetNetwork.hpp"
#include <arpa/inet.h>
namespace CBR {

void testAny(const char * listenport, const char* hostname, const char* port, bool server) {
    RaknetNetwork rn;
    rn.listen(listenport);
    bool canSend=!server;
    unsigned int mine=0;
    unsigned int theirs=0;
    Sirikata::Network::Chunk toSend(4);
    unsigned int offset=15;
    unsigned int maxval=65637;
    while (mine<maxval||theirs<maxval) {
        Sirikata::Network::Chunk *c;
        while ((c=rn.receiveOne())) {
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
            rn.sendTo(Address4(Sirikata::Network::Address(hostname,port)),toSend,true,true,0);
        }
    }
}
void testServer(const char * listenport, const char* hostname, const char* port){

    testAny(listenport,hostname,port,true);    
}
void testClient(const char * listenport, const char* hostname, const char* port){
    testAny(listenport,hostname,port,false);    

}


}
