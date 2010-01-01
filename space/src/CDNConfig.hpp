#ifndef SIRIKATA_CDNConfig_hpp__
#define SIRIKATA_CDNConfig_hpp__

#include "Config.hpp"

namespace Sirikata {
namespace Transfer {
class TransferManager;
}

namespace Task {
class Event;
template <class T> class EventManager;
}

void initializeProtocols();
Transfer::TransferManager *initializeTransferManager (const OptionMap& options, Task::EventManager<Task::Event> *eventMgr);
void destroyTransferManager (Transfer::TransferManager *tm);

}

#endif
