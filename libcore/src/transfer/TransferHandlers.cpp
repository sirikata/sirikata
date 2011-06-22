#include <sirikata/core/transfer/TransferHandlers.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::SharedChunkCache);

namespace Sirikata {
namespace Transfer {

SharedChunkCache& SharedChunkCache::getSingleton() {
    return AutoSingleton<SharedChunkCache>::getSingleton();
}
void SharedChunkCache::destroy() {
    AutoSingleton<SharedChunkCache>::destroy();
}
const unsigned int SharedChunkCache::DISK_LRU_CACHE_SIZE = 1024 * 1024 * 1024; //1GB
const unsigned int SharedChunkCache::MEMORY_LRU_CACHE_SIZE = 1024 * 1024 * 50; //50MB

SharedChunkCache::SharedChunkCache() {
    //Use LRU for eviction
    mDiskCachePolicy = new LRUPolicy(DISK_LRU_CACHE_SIZE);
    mMemoryCachePolicy = new LRUPolicy(MEMORY_LRU_CACHE_SIZE);

    //Make a disk cache as the bottom cache layer
    CacheLayer* diskCache = new DiskCacheLayer(mDiskCachePolicy, "HttpChunkHandlerCache", NULL);
    mCacheLayers.push_back(diskCache);

    //Make a mem cache on top of the disk cache
    CacheLayer* memCache = new MemoryCacheLayer(mMemoryCachePolicy, diskCache);
    mCacheLayers.push_back(memCache);

    //Store top memory cache as the one we'll use
    mCache = memCache;
}

SharedChunkCache::~SharedChunkCache() {
    //Delete all the cache layers we created
    for (std::vector<CacheLayer*>::reverse_iterator it = mCacheLayers.rbegin(); it != mCacheLayers.rend(); it++) {
        delete (*it);
    }
    mCacheLayers.clear();

    //And delete LRU cache policies
    delete mDiskCachePolicy;
    delete mMemoryCachePolicy;
}

CacheLayer* SharedChunkCache::getCache() {
    return mCache;
}

}
}
