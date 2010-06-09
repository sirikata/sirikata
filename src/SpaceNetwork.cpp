#include "SpaceNetwork.hpp"
#include "Options.hpp"

#include <netdb.h>
namespace Sirikata{

SpaceNetwork::SpaceNetwork(SpaceContext* ctx)
 : mContext(ctx),
   mServerIDMap(NULL)
{
}

SpaceNetwork::~SpaceNetwork() {
}

void SpaceNetwork::start() {
}

void SpaceNetwork::stop() {
}

void SpaceNetwork::setServerIDMap(ServerIDMap* sidmap) {
    mServerIDMap = sidmap;
}

}
