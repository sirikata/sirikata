#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "Message.hpp"

namespace CBR {


bool FIFOObjectMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj){
    return mServerMessageQueue->addMessage(destinationServer,msg);
}
void FIFOObjectMessageQueue::service(const Time& t){

}

void FIFOObjectMessageQueue::registerClient(UUID sid, float weight) {

}


}
