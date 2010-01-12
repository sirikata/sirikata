#include "Network.hpp"
#include "Options.hpp"

#include <netdb.h>
namespace CBR{

Network::Network(SpaceContext* ctx)
 : mContext(ctx),
   mServerIDMap(NULL)
{
}

Network::~Network() {
}

void Network::start() {
}

void Network::stop() {
}

void Network::setServerIDMap(ServerIDMap* sidmap) {
    mServerIDMap = sidmap;
}

}
