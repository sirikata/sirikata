#include "oh/Platform.hpp"
#include "MonoObject.hpp"
#include "MonoClass.hpp"
#include "MonoAssembly.hpp"
#include "MonoDomain.hpp"
#include "MonoUtil.hpp"
#include "MonoException.hpp"
#include "MonoSystem.hpp"
namespace Mono {

#include <mono/jit/jit.h>
#include <mono/utils/mono-logger.h>
#if (MERU_PLATFORM == MERU_PLATFORM_WINDOWS) || (MONO_VERSION_MAJOR < 1) || (MONO_VERSION_MAJOR == 1 && ((MONO_VERSION_MINOR < 9) || (MONO_VERSION_MINOR == 9 && MONO_VERSION_MICRO == 0)))
#ifdef G_HAVE_ISO_VARARGS
G_END_DECLS
#endif
#endif
#include <mono/metadata/mono-gc.h>

namespace Mono {

extern void reportPinnedObjects();

//#####################################################################
// Function assembly_loaded
//#####################################################################
void assembly_loaded(MonoAssembly* assembly, gpointer user_data) {
    assert(assembly != NULL);
    Assembly ass(assembly);
    std::vector<Assembly>* assemblies = (std::vector<Assembly>*)user_data;
    assemblies->push_back(ass);
    printf("Assembly loaded: %s\n", ass.fullname().c_str());
}

//#####################################################################
// Function Mono
//#####################################################################
MonoSystem::MonoSystem()
    : mDomain(NULL)
{
    mono_install_assembly_load_hook(assembly_loaded, &mAssemblies);

    mWorkDir = "/home/daniel/sirikata/";//Sirikata::getCWD();

    mDomain = Domain(mono_jit_init_version("Root Domain", "v2.0.50727"));

    assert(!mDomain.null());
    Domain::setRoot(mDomain);

    //mono_trace_set_level(G_LOG_LEVEL_INFO);
    //mono_trace_set_level(G_LOG_LEVEL_DEBUG);
}

//#####################################################################
// Function ~Mono
//#####################################################################
MonoSystem::~MonoSystem() {
    reportPinnedObjects();

    mDomain.fireProcessExit();
    mono_jit_cleanup(mDomain.domain());
}

//#####################################################################
// Function createDomain
//#####################################################################
Domain MonoSystem::createDomain() {
    NOT_IMPLEMENTED(mono);
    return Domain::root();//(mono_domain_create());
}

//#####################################################################
// Function listAssembliesHelper
//#####################################################################
static void listAssembliesHelper(MonoAssembly* assembly, MonoSystem* this_ptr) {
    MonoImage* image = mono_assembly_get_image(assembly);
    printf("Assembly Image: %s\n", mono_image_get_name(image));
}

//#####################################################################
// Function listAssemblies
//#####################################################################
void MonoSystem::listAssemblies() {
    mono_assembly_foreach((GFunc)listAssembliesHelper, this);
}

//#####################################################################
// Function getClass
//#####################################################################
Class MonoSystem::getClass(const Meru::String& name_space, const Meru::String& klass) {
    // Check all loaded assemblies
    for(unsigned int i = 0; i < mAssemblies.size(); i++) {
        // FIXME we should probably have a way to just check and not have to deal with all these exceptions...
        try {
            Class found_klass = mAssemblies[i].getClass(name_space, klass);
            return found_klass;
        }
        catch (Exception& e) {
            // do nothing, just means it wasn't in this assembly
        }
    }

    // If not found, throw
    throw Exception::ClassNotFound(name_space + "." + klass, "(Any)");
}

//#####################################################################
// Function loadAssembly
//#####################################################################
bool MonoSystem::loadAssembly(const Meru::String& name) const {
    MonoAssemblyName aname;
    bool parsed = mono_assembly_name_parse(name.c_str(), &aname);
    if (!parsed) return false;

    MonoImageOpenStatus image_open_status;
    MonoAssembly* assembly = mono_assembly_load(&aname, mWorkDir.c_str(), &image_open_status);
    mono_assembly_name_free(&aname);

    return (assembly != NULL);
}

//#####################################################################
// Function loadAssembly
//#####################################################################
bool MonoSystem::loadAssembly(const Meru::String& name, const Meru::String& dir) const {
    MonoAssemblyName aname;
    bool parsed = mono_assembly_name_parse(name.c_str(), &aname);
    if (!parsed) return false;

    MonoImageOpenStatus image_open_status;
    MonoAssembly* assembly = mono_assembly_load(&aname, dir.c_str(), &image_open_status);
    mono_assembly_name_free(&aname);

    return (assembly != NULL);
}

//#####################################################################
// Function loadAssembly
//#####################################################################
bool MonoSystem::loadMemoryAssembly(char* membuf, unsigned int len) const {
    MonoAssembly *ass;
    MonoImageOpenStatus status = MONO_IMAGE_OK;
    MonoImage *image = mono_image_open_from_data_full(membuf, len, TRUE, &status, false);

    if (!image) { // bad image format
        printf("Error loading image from memory: %d\n", status);
        return false;
    }

    ass = mono_assembly_load_from_full (image, "", &status, false);

    if (!ass) {
        printf("Error loading assembly from memory: %d\n", status); // FIXME real error reporting
        // bad image format?
        mono_image_close (image);
        return false;
    }

    return true;
}

//#####################################################################
// Function GC
//#####################################################################
void MonoSystem::GC() const {
    mono_gc_max_generation();
}

} // namespace Mono

