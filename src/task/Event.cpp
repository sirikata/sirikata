#include <string>
#include <map>

#include "Event.hpp"

namespace Sirikata {
namespace Task {

/// Stores the map from primary ID to integer--items are never deleted.
typedef std::map<std::string, int> IDMapType;

static IDMapType idMap;
static int max_id = 0;
int IdPair::Primary::getUniqueId(const std::string &id) {
	IDMapType::iterator iter = idMap.find(id);
	if (iter == idMap.end()) {
		iter = idMap.insert(IDMapType::value_type(id, max_id)).first;
		max_id++;
	}
	return (*iter).second;
}

IdPair::Primary::Primary (const std::string &id)
	: mId(getUniqueId(id)) {
}
void Event::operator() (EventHistory){
    //FIXME should this delete the event?
}

}
}
