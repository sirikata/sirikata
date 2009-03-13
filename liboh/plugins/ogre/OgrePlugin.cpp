#include <oh/Platform.hpp>
#include <oh/GraphicsFactory.hpp>
#include "OgreSystem.hpp"
static int core_plugin_refcount = 0;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    using namespace Sirikata::Graphics;
    if (core_plugin_refcount==0)
        GraphicsFactory::getSingleton().registerConstructor("ogregraphics",
                                                            &OgreSystem::create,
                                                            true);
    core_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    core_plugin_refcount--;
    if (core_plugin_refcount==0)
        GraphicsFactory::getSingleton().unregisterConstructor("ogregraphics",true);
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "ogregraphics";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}
