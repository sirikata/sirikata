#include <oh/Platform.hpp>
#include <oh/SimulationFactory.hpp>
#include "OgreSystem.hpp"
static int core_plugin_refcount = 0;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    using namespace Sirikata::Graphics;
    if (core_plugin_refcount==0)
        SimulationFactory::getSingleton().registerConstructor("ogregraphics",
                                                            &OgreSystem::create,
                                                            true);
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
        if (core_plugin_refcount==0)
            SimulationFactory::getSingleton().unregisterConstructor("ogregraphics",true);
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "ogregraphics";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}
