#include <util/Platform.hpp>
#include "persistence/ObjectStorage.hpp"
#include "SQLiteObjectStorage.hpp"
#include "persistence/MinitransactionHandlerFactory.hpp"
#include "persistence/ReadWriteHandlerFactory.hpp"
static int core_plugin_refcount = 0;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (core_plugin_refcount==0) {
        MinitransactionHandlerFactory::getSingleton().registerConstructor("sqlite",
                                                                          std::tr1::bind(&SQLiteObjectStorage::create,true,_1),
                                                                          true);
        ReadWriteHandlerFactory::getSingleton().registerConstructor("sqlite",
                                                                    std::tr1::bind(&SQLiteObjectStorage::create,false,_1),
                                                                    true);
    }
    core_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++core_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(core_plugin_refcount>0);
    return --core_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (core_plugin_refcount>0) {
        core_plugin_refcount--;
        assert(core_plugin_refcount==0);
        if (core_plugin_refcount==0) {
            MinitransactionHandlerFactory::getSingleton().unregisterConstructor("sqlite",true);
            ReadWriteHandlerFactory::getSingleton().unregisterConstructor("sqlite",true);
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "sqlite";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}
