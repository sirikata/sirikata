#include <oh/Platform.hpp>
#include <util/Platform.hpp>

#include "CDNConfig.hpp"

#include <task/EventManager.hpp>

#include <transfer/EventTransferManager.hpp>
#include <transfer/LRUPolicy.hpp>
#include <transfer/DiskCacheLayer.hpp>
#include <transfer/MemoryCacheLayer.hpp>
#include <transfer/NetworkCacheLayer.hpp>
#include <transfer/HTTPDownloadHandler.hpp>
#include <transfer/HTTPUploadHandler.hpp>
#include <transfer/HTTPFormUploadHandler.hpp>
#include <transfer/FileProtocolHandler.hpp>
#include <transfer/CachedNameLookupManager.hpp>
#include <transfer/CachedServiceLookup.hpp>
#include <transfer/ServiceManager.hpp>

namespace Sirikata {

using namespace Transfer;
using Task::GenEventManager;

cache_usize_type parseSize(const std::string &sizeStr) {
    std::istringstream stream (sizeStr);
    char units;
    double amount(0);
    stream >> amount >> units;
    units = tolower(units);
    switch (units) {
    case 'k':
        return cache_usize_type(amount * Transfer::kilobyte);
    case 'm':
        return cache_usize_type(amount * Transfer::megabyte);
    case 'g':
        return cache_usize_type(amount * Transfer::gigabyte);
    case 't':
        return cache_usize_type(amount * Transfer::terabyte);
    }
    return cache_usize_type(amount);
}

template <class CreatedType>
class OptionFactory {
public:
    typedef CreatedType* (*FactoryFunc) (const OptionMap&);
    typedef std::map<std::string, FactoryFunc> FactoryMap;
    FactoryMap factories;
    OptionFactory(void (*initializer) (OptionFactory<CreatedType> &)) {
        initializer(*this);
    }
    CreatedType *operator()(const OptionMap &options) {
        typename FactoryMap::const_iterator iter = factories.find(options.getValue());
        if (iter == factories.end())
            return NULL;
        return ((*iter).second)(options);
    }
    void insert(const std::string &name, FactoryFunc func) {
        factories.insert(typename FactoryMap::value_type(name,func));
    }
};

CachePolicy *createLRUPolicy(const OptionMap &options) {
    return new LRUPolicy(parseSize(options["size"].getValue()));
}

void initializePolicy(OptionFactory<CachePolicy> &factories) {
    factories.insert("LRU",&createLRUPolicy);
}
OptionFactory<CachePolicy> CreatePolicy(&initializePolicy);

CacheLayer *createMemoryCache(const OptionMap &options) {
    CachePolicy *policy = CreatePolicy(options["policy"]);
    if (!policy)
        return NULL;
    return new MemoryCacheLayer(policy, NULL);
}
CacheLayer *createDiskCache(const OptionMap &options) {
    CachePolicy *policy = CreatePolicy(options["policy"]);
    if (!policy)
        return NULL;
    return new DiskCacheLayer(policy, options["directory"].getValue(), NULL);
}
CacheLayer *createNetworkCache(const OptionMap &options); // Defined below.


void initializeLayer(OptionFactory<CacheLayer> &factories) {
    factories.insert("Memory",&createMemoryCache);
    factories.insert("Disk",&createDiskCache);
    factories.insert("Network",&createNetworkCache);
}
OptionFactory<CacheLayer> CreateCache(&initializeLayer);


Transfer::ProtocolRegistry<Transfer::DownloadHandler> downloadProtocolRegistry;
Transfer::ProtocolRegistry<Transfer::NameLookupHandler> nameProtocolRegistry;
Transfer::ProtocolRegistry<Transfer::UploadHandler> uploadProtocolRegistry;
Transfer::ProtocolRegistry<Transfer::NameUploadHandler> nameUploadProtocolRegistry;
void initializeProtocols() {
            std::tr1::shared_ptr<Transfer::HTTPDownloadHandler> httpHandler(new Transfer::HTTPDownloadHandler);
            std::tr1::shared_ptr<Transfer::HTTPUploadHandler> httpRestUploadHandler(new Transfer::HTTPUploadHandler);
            std::tr1::shared_ptr<Transfer::HTTPFormUploadHandler> httpFormUploadHandler(new Transfer::HTTPFormUploadHandler);
            std::tr1::shared_ptr<Transfer::FileProtocolHandler> fileHandler(new Transfer::FileProtocolHandler);
            std::tr1::shared_ptr<Transfer::FileNameHandler> fileNameHandler(new Transfer::FileNameHandler(fileHandler));
            downloadProtocolRegistry.setHandler("http", httpHandler);
            nameProtocolRegistry.setHandler("http", httpHandler);
            uploadProtocolRegistry.setHandler("http", httpFormUploadHandler);
            nameUploadProtocolRegistry.setHandler("http", httpFormUploadHandler);

            downloadProtocolRegistry.setHandler("rest", httpHandler);
            nameProtocolRegistry.setHandler("rest", httpHandler);
            uploadProtocolRegistry.setHandler("rest", httpRestUploadHandler);
            nameUploadProtocolRegistry.setHandler("rest", httpRestUploadHandler);

            downloadProtocolRegistry.setHandler("file", fileHandler);
            nameProtocolRegistry.setHandler("file", fileNameHandler);
            uploadProtocolRegistry.setHandler("file", fileHandler);
            nameUploadProtocolRegistry.setHandler("file", fileNameHandler);
}


void insertOneService(const OptionMap &urioption, Transfer::ListOfServices *services) {
            Transfer::ServiceParams params;
            for (OptionMap::const_iterator paramiter = urioption.begin(); paramiter != urioption.end(); ++paramiter) {
                params.insert(Transfer::ServiceParams::value_type((*paramiter).first, (*paramiter).second->getValue()));
            }
            services->push_back(Transfer::ListOfServices::value_type(
                                Transfer::URIContext(urioption.getValue()),
                                params));
}
void insertServices(const OptionMap &options, Transfer::CachedServiceLookup *serviceCache) {
    for (OptionMap::const_iterator iter = options.begin(); iter != options.end(); ++iter) {
        Transfer::ListOfServices *services = new Transfer::ListOfServices;
        if ((*iter).second->getValue().empty()) {
            for (OptionMap::const_iterator uriiter = (*iter).second->begin(); uriiter != (*iter).second->end(); ++uriiter) {
                insertOneService(*(*uriiter).second, services);
            }
        } else {
            insertOneService(*((*iter).second), services);
        }
        serviceCache->addToCache(Transfer::URIContext((*iter).first), Transfer::ListOfServicesPtr(services));
    }
}

CacheLayer *createNetworkCache(const OptionMap &options) {
    Transfer::CachedServiceLookup *services = new Transfer::CachedServiceLookup;
    Transfer::ServiceManager<Transfer::DownloadHandler> *downServ (
                new Transfer::ServiceManager<Transfer::DownloadHandler>(
                    services,
                    &downloadProtocolRegistry));
    insertServices(options["services"], services);
    return new NetworkCacheLayer(NULL, downServ);
}

TransferManager *initializeTransferManager (const OptionMap& options, GenEventManager *eventMgr) {
    OptionMap &cache = options["cache"];
    CacheLayer *lastCacheLayer = NULL;
    CacheLayer *firstCacheLayer;
    for (OptionMap::const_iterator iter = cache.begin(); iter != cache.end(); ++iter) {
        CacheLayer *newCacheLayer = CreateCache(*((*iter).second));
        if (lastCacheLayer) {
            lastCacheLayer->setNext(newCacheLayer);
        } else {
            firstCacheLayer = newCacheLayer;
        }
        lastCacheLayer = newCacheLayer;
    }
    Transfer::CachedServiceLookup*downServiceMap,*nameServiceMap,*upServiceMap,*upnameServiceMap;
    Transfer::ServiceManager<Transfer::DownloadHandler> *downServ =
                new Transfer::ServiceManager<Transfer::DownloadHandler>(
                    downServiceMap=new Transfer::CachedServiceLookup,
                    &downloadProtocolRegistry);
            Transfer::ServiceManager<Transfer::NameLookupHandler> *nameServ =
                new Transfer::ServiceManager<Transfer::NameLookupHandler>(
                    nameServiceMap=new Transfer::CachedServiceLookup,
                    &nameProtocolRegistry);
    Transfer::ServiceManager<Transfer::UploadHandler> *uploadServ =
                new Transfer::ServiceManager<Transfer::UploadHandler>(
                    upServiceMap=new Transfer::CachedServiceLookup,
                    &uploadProtocolRegistry);
            Transfer::ServiceManager<Transfer::NameUploadHandler> *upnameServ =
                new Transfer::ServiceManager<Transfer::NameUploadHandler>(
                    upnameServiceMap=new Transfer::CachedServiceLookup,
                    &nameUploadProtocolRegistry);

            insertServices(options["download"], downServiceMap);
            insertServices(options["namelookup"], nameServiceMap);
            insertServices(options["upload"], upServiceMap);
            insertServices(options["nameupload"], upnameServiceMap);

        return new Transfer::EventTransferManager(
                firstCacheLayer,
                new Transfer::CachedNameLookupManager(nameServ, downServ),
                eventMgr,
				downServ,
                upnameServ,
                uploadServ
            );
}

}
