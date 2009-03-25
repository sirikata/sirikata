#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
#include "ServerIDMap.hpp"
#include "Statistics.hpp"

namespace CBR{
class ServerMessageQueue {
public:
    ServerMessageQueue(Network* net, const ServerID& sid, ServerIDMap* sidmap, Trace* trace)
     : mNetwork(net),
       mSourceServer(sid),
       mServerIDMap(sidmap),
       mTrace(trace)
    {
        // start the network listening
        Address4* listen_addy = mServerIDMap->lookup(mSourceServer);
        assert(listen_addy != NULL);
        net->listen(*listen_addy);
    }

    virtual ~ServerMessageQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out) = 0;
    virtual void service(const Time& t)=0;

    virtual void setServerWeight(ServerID sid, float weight) = 0;

protected:
    Network* mNetwork;
    ServerID mSourceServer;
    ServerIDMap* mServerIDMap;
    Trace* mTrace;
};
}
#endif
