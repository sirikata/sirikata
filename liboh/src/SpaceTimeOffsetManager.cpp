/*  Sirikata
 *  SpaceTimeOffsetManager.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/SpaceID.hpp>
#include <sirikata/oh/SpaceTimeOffsetManager.hpp>
#include <sirikata/core/util/Time.hpp>
#include <boost/thread.hpp>

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
Time SpaceTimeOffsetManager::now(const SpaceID& sid) {
    return Time::now(getSpaceTimeOffset(sid));
}
const Duration& SpaceTimeOffsetManager::getSpaceTimeOffset(const SpaceID& sid) {
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
AUTO_SINGLETON_INSTANCE(Sirikata::SpaceTimeOffsetManager);
