#ifndef SIRIKATA_CDNConfig_hpp__
#define SIRIKATA_CDNConfig_hpp__

#include <core/options/Config.hpp>

namespace Sirikata {
namespace Transfer {
class TransferManager;
}

namespace Task {
class Event;
template <class T> class EventManager;
}

SIRIKATA_EXPORT void initializeProtocols();
SIRIKATA_EXPORT Transfer::TransferManager *initializeTransferManager (const OptionMap& options, Task::EventManager<Task::Event> *eventMgr);
SIRIKATA_EXPORT void destroyTransferManager (Transfer::TransferManager *tm);

}

#endif
