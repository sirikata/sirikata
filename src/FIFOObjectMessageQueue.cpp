#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "Message.hpp"

namespace CBR {


bool FIFOObjectMessageQueue::send(ObjectToObjectMessage* msg) {
    UUID dest_uuid = msg->destObject();
    ServerID dest_server_id = lookup(dest_uuid);

    Network::Chunk msg_serialized;
    msg->serialize(msg_serialized, 0);

    return mServerMessageQueue->addMessage(dest_server_id, msg_serialized);
}

void FIFOObjectMessageQueue::service(const Time& t){

}

void FIFOObjectMessageQueue::registerClient(UUID sid, float weight) {

}


}
