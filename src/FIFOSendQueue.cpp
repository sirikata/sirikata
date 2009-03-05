#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOSendQueue.hpp"
namespace CBR {

bool FIFOSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    mQueue.push(std::pair<ServerID,Network::Chunk>(destinationServer,msg));
    return true;
}
bool FIFOSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj){
    mQueue.push(std::pair<ServerID,Network::Chunk>(destinationServer,msg));        
    return true;
}
void FIFOSendQueue::service(){
    while(mQueue.size()) {
        bool ok=mNetwork->send(mQueue.front().first,mQueue.front().second,false,true,1);
        assert(ok&&"Network Send Failed");
        mQueue.pop();
    }
}



}
