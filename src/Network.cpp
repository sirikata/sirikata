#include "Network.hpp"
#include "ServerIDMap.hpp"
#include <netdb.h>
namespace CBR{

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

Network::Network(SpaceContext* ctx)
 : mContext(ctx)
{
    mProfiler = mContext->profiler->addStage("Network");
}

}
