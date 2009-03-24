#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "Statistics.hpp"

namespace CBR{
class ServerMessageQueue {
public:
    ServerMessageQueue(Network* net, const ServerID& sid, Trace* trace)
     : mNetwork(net),
       mSourceServer(sid),
       mTrace(trace)
    {}

    virtual ~ServerMessageQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual Network::Chunk* receive() = 0;
    virtual void service(const Time& t)=0;

    virtual void setServerWeight(ServerID sid, float weight) = 0;

protected:
    Network* mNetwork;
    ServerID mSourceServer;
    Trace* mTrace;
};
}
#endif
