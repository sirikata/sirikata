#include <oh/Platform.hpp>
#include "util/SpaceID.hpp"
#include "oh/SpaceTimeOffsetManager.hpp"
#include "util/Time.hpp"
#include <boost/thread.hpp>
AUTO_SINGLETON_INSTANCE(Sirikata::SpaceTimeOffsetManager);
namespace Sirikata {
#if BOOST_VERSION >= 103600
typedef boost::shared_mutex SharedMutex;
typedef boost::shared_lock<boost::shared_mutex> SharedLock;
typedef boost::unique_lock<boost::shared_mutex> UniqueLock;
#else
typedef boost::mutex SharedMutex;
typedef boost::unique_lock<boost::mutex> UniqueLock;
typedef boost::unique_lock<boost::mutex> SharedLock;
#endif
namespace {
SharedMutex sSpaceIdDurationMutex;
std::tr1::unordered_map<SpaceID,Duration,SpaceID::Hasher> sSpaceIdDuration;
Duration nilDuration(Duration::seconds(0));;
}

SpaceTimeOffsetManager::SpaceTimeOffsetManager(){}
SpaceTimeOffsetManager::~SpaceTimeOffsetManager(){}
void SpaceTimeOffsetManager::destroy() {
    AutoSingleton<SpaceTimeOffsetManager>::destroy();
}
SpaceTimeOffsetManager& SpaceTimeOffsetManager::getSingleton() {
    return AutoSingleton<SpaceTimeOffsetManager>::getSingleton();
}
const Duration& SpaceTimeOffsetManager::getSpaceTimeOffset(const SpaceID& sid)const {
    SharedLock lok(sSpaceIdDurationMutex);
    std::tr1::unordered_map<SpaceID,Duration>::iterator where=sSpaceIdDuration.find(sid);
    if (where==sSpaceIdDuration.end()) {
        return nilDuration;
    }
    return where->second;
}
void SpaceTimeOffsetManager::setSpaceTimeOffset(const SpaceID& sid, const Duration&dur) {
    UniqueLock lok(sSpaceIdDurationMutex);
    sSpaceIdDuration[sid]=dur;
}

}
