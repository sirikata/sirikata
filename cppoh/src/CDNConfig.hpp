#ifndef SIRIKATA_CDNConfig_hpp__
#define SIRIKATA_CDNConfig_hpp__

namespace Sirikata {

class OptionMap;
typedef std::tr1::shared_ptr<OptionMap> OptionMapPtr;

struct OptionDoesNotExist : public std::runtime_error {
    OptionDoesNotExist(const std::string &optionName)
        : std::runtime_error("Option '" + optionName + "' does not exist!") {
    }
};

class OptionMap : public std::map<std::string, OptionMapPtr>{
private:
    std::string value;
public:
    OptionMap(std::string value) : value(value) {
    }
    OptionMap() {}

    const std::string & getValue() const {
        return value;
    }
    const OptionMapPtr &put(const std::string &key, OptionMapPtr value) {
        iterator iter = insert(OptionMap::value_type(key, value)).first;
        return (*iter).second;
    }
    const OptionMapPtr &get (const std::string &key) const {
        static OptionMapPtr nulloptionmapptr;
        const_iterator iter = find(key);
        if (iter != end()) {
            return (*iter).second;
        } else {
            return nulloptionmapptr;
        }
    }
    OptionMap &operator[] (const std::string &key) const {
        const OptionMapPtr &ptr = get(key);
        if (!ptr) {
            throw OptionDoesNotExist(key);
        }
        return *ptr;
    }
};
// Can be used to dump from file.
std::ostream &operator<< (std::ostream &os, const OptionMap &om);

void parseConfig(
        const std::string &input,
        const OptionMapPtr &globalvariables, //< Identical to output
        const OptionMapPtr &options, //< output
        std::string::size_type &pos //< reference to variable initialized to 0.
);

namespace Transfer {
class TransferManager;
}

namespace Task {
class Event;
template <class T> class EventManager;
}

void initializeProtocols();
Transfer::TransferManager *initializeTransferManager (const OptionMap& options, Task::EventManager<Task::Event> *eventMgr);

}

#endif
