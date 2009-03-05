#include "Network.hpp"
#include "ServerIDMap.hpp"
#include <netdb.h>
namespace CBR{
bool Network::send(const ServerID&sid,const Sirikata::Network::Chunk&c, bool reliable, bool ordered, int priority){
    Address4* addy=mServerIDMap->lookup(sid);
    if (addy) {
        return this->sendTo(*addy,c,reliable,ordered,priority);
    }
    return false;
}




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
}
