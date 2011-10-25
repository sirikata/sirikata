// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/util/Timer.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/Paths.hpp>
#include <boost/filesystem.hpp>
#include <sirikata/core/transfer/AggregatedTransferPool.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#define LAUNCHER_LOG(lvl, msg) SILOG(launcher, lvl, msg)

using namespace Sirikata;

// ### Utils ###

void InitLauncherOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue("register","false",Sirikata::OptionValueType<bool>(),"If true, registers this binary as handler"))
        .addOption(new OptionValue("unregister","false",Sirikata::OptionValueType<bool>(),"If true, unregisters this binary as handler"))
        .addOption(new OptionValue("uri","",Sirikata::OptionValueType<String>(),"The URI to launch"))
        ;
}

String getExecutableName(String name) {
#if SIRIKATA_DEBUG
    name = name + "_d";
#endif
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    name = name + ".exe";
#endif
    return name;
}

String getExecutablePath(String name) {
    String exe_dir = Path::Get(Path::DIR_EXE);

    name = getExecutableName(name);
#if SIRIKATA_PLATFORM == PLATFORM_MAC
    if (name == "cppoh" || name == "cppoh_d")
        name = name + ".app/Contents/MacOS/" + name;
#endif

    return (boost::filesystem::path(exe_dir) / name).string();
}


#if SIRIKATA_PLATFORM == PLATFORM_LINUX || SIRIKATA_PLATFORM == PLATFORM_MAC
void execCommand(const char* file, const char* const argv[]) {
    pid_t pID = fork();

    if (pID == 0) {
        execvp(file, (char* const*)argv);
    }
    else if (pID < 0) {
        LAUNCHER_LOG(fatal, "Couldn't fork to execute registration commands.");
    }
}
#endif

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
/* Set a registry key. Only creates the key if subkey == NULL. To set the default value, use subkey == "". */
bool setRegKey(HKEY base, const char* key, const char* subkey = NULL, const char* value = NULL) {
    HKEY hkey;
    DWORD disp;
    int status = 0;

    RegOpenKeyEx(base, key, 0, KEY_ALL_ACCESS, &hkey);
    if (!hkey)
        status = RegCreateKeyEx(base, key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &disp);
    if (!status && (subkey != NULL && value != NULL)) RegSetValueEx(hkey, subkey, 0, REG_SZ, (const BYTE *)value, strlen(value)+1);
    RegCloseKey(hkey);
    return (status == 0);
}
/* Delete a registry key. */
bool deleteRegKey(HKEY base, const char* key) {
    int status = 0;
    status = RegDeleteKey(base, key);
    return (status == 0);
}
#endif

// ### Registration ###

bool registerURIHandler() {
    // Note that we actually ask for slauncher's path instead of just using this
    // binary's. This ensures we use the generic (rather than versioned)
    // copy/symlink of the executable. This is just useful to handle upgrades
    // gracefully, i.e. without having to reregister.
    String exe_file = getExecutablePath("slauncher");

#if SIRIKATA_PLATFORM == PLATFORM_LINUX
    String cmd = exe_file + " --uri=%s";
    String needs_terminal =
#if SIRIKATA_DEBUG
        "true"
#else
        "false"
#endif
        ;
    const char* const command_argv[] = { "gconftool-2", "-t", "string", "-s", "/desktop/gnome/url-handlers/sirikata/command", cmd.c_str(), NULL};
    execCommand("gconftool-2", command_argv);

    const char* const needs_terminal_argv[] = { "gconftool-2", "-t", "bool", "-s", "/desktop/gnome/url-handlers/sirikata/needs_terminal", needs_terminal.c_str(), NULL};
    execCommand("gconftool-2", needs_terminal_argv);

    const char* const enabled_argv[] = { "gconftool-2", "-t", "bool", "-s", "/desktop/gnome/url-handlers/sirikata/enabled", "true", NULL};
    execCommand("gconftool-2", enabled_argv);

#elif SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    String exe_name = getExecutableName("slauncher");
    // See http://msdn.microsoft.com/en-us/library/aa767914.aspx for an explanation of what these registry keys do.
    // NOTE: YOU MUST RUN THIS AS ADMINISTRATOR. It'll fail to add the keys if you don't.
    String icon_path = exe_name + ",1";
    String cmd = "\"" + exe_file + "\"" + " " + "\"" + "--uri=%1" + "\"";
    // Apparently windows doesn't like the Unix-style separators in its registry settings
    for(int i = 0; i < cmd.size(); i++)
        if (cmd[i] == '/') cmd[i] = '\\';
    bool success =
        setRegKey(HKEY_CLASSES_ROOT, "sirikata") &&
        setRegKey(HKEY_CLASSES_ROOT, "sirikata", "", "URL:Sirikata Protocol") &&
        setRegKey(HKEY_CLASSES_ROOT, "sirikata", "URL Protocol", "") &&
        setRegKey(HKEY_CLASSES_ROOT, "sirikata\\Default Icon", "", icon_path.c_str()) &&
        setRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell") &&
        setRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell\\open") &&
        setRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell\\open\\command", "", cmd.c_str());
    if (!success)
        LAUNCHER_LOG(error, "Failed to register handler in system registry.");
    return success;
#elif SIRIKATA_PLATFORM == PLATFORM_MAC
    // FIXME
    LAUNCHER_LOG(error, "URI handler registration not supported on this platform.");
    return false;
#endif

    LAUNCHER_LOG(info, "Registered launcher. You may have to restart your browser for this to take effect.");

    return true;
}

bool unregisterURIHandler() {
    // See note in registerURIHandler
    String exe_file = getExecutablePath("slauncher");

#if SIRIKATA_PLATFORM == PLATFORM_LINUX
    const char* const enabled_argv[] = { "gconftool-2", "-t", "bool", "-s", "/desktop/gnome/url-handlers/sirikata/enabled", "false", NULL};
    execCommand("gconftool-2", enabled_argv);
#elif SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    bool success = 
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell\\open\\command") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell\\open") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\Default Icon") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata");
    LAUNCHER_LOG(error, "Failed to unregister handler in system registry.");
    return success;
#elif SIRIKATA_PLATFORM == PLATFORM_MAC
    // FIXME
#endif

    return false;
}


// ### Downloading config and launching code ###

// Work to keep the system busy when other stuff, e.g. downloads, are going on
// away from the main IOService
Network::IOWork* ioWork = NULL;
void startWork(Network::IOService* ios) {
    ioWork = new Network::IOWork(*ios, "slauncher");
}
void stopWork() {
    delete ioWork;
    ioWork = NULL;
}


Transfer::ResourceDownloadTaskPtr rdl;
void finishLaunchURI(Transfer::ChunkRequestPtr req, Transfer::DenseDataPtr data, int* retval);
bool startLaunchURI(String uri_str, Transfer::TransferPoolPtr transferPool, int* retval) {
    Transfer::URI uri(uri_str);
    if (uri.scheme() != "sirikata") {
        LAUNCHER_LOG(error, "slauncher only supports sirikata URIs");
        return false;
    }

    // Format for the URI: sirikata:real_config_url
    String config_uri_str = uri.schemeSpecificPart();
    // We assume the second part is a URI that we support
    Transfer::URI config_uri(config_uri_str);
    if (config_uri.empty()) {
        LAUNCHER_LOG(error, "Invalid config URL in slauncher URI.");
        return false;
    }

    LAUNCHER_LOG(detailed, "Loading from " << config_uri_str);

    rdl =
        Transfer::ResourceDownloadTask::construct(
            config_uri, transferPool, 1.0,
            std::tr1::bind(finishLaunchURI, std::tr1::placeholders::_1, std::tr1::placeholders::_2, retval)
        );
    rdl->start();

    return true;
}

void eventLoopExit(int* retval, int val) {
    stopWork();
    *retval = val;
}

void finishLaunchURI(Transfer::ChunkRequestPtr req, Transfer::DenseDataPtr data, int* retval) {
    // Fire off the request for
    if (!data || data->size() == 0) {
        LAUNCHER_LOG(error, "Failed to download config");
        eventLoopExit(retval, -1);
        return;
    }

    LAUNCHER_LOG(detailed, "Downloaded config, size " << data->size());

    // Parse the data
    using namespace boost::property_tree;
    ptree pt;
    try {
        std::stringstream data_json(data->asString());
        read_json(data_json, pt);
    }
    catch(json_parser::json_parser_error exc) {
        LAUNCHER_LOG(error, "Invalid JSON configuration data.");
        eventLoopExit(retval, -1);
        return;
    }


    String app;
    String appDir;
    String binary;
    std::vector<String> binaryArgs;
    try {
        app = pt.get<String>("app.name");
        appDir = pt.get<String>("app.directory");
        // First, we need to know what app we're executing
        binary = pt.get<String>("binary.name");
        // We also take a dictionary of arguments to add to the command
        // line
        BOOST_FOREACH(ptree::value_type &v,
            pt.get_child("binary.args"))
            binaryArgs.push_back( String("--") + v.first + "=" + v.second.data() );
    }
    catch(ptree_bad_data exc) {
        LAUNCHER_LOG(error, "Required configuration properties not found.");
        eventLoopExit(retval, -1);
        return;
    }

    // We want to add support for syncing additional content (maybe
    // something like app.data = url for archive) which will require
    // additional downloading, but for now we just invoke the command
    // here.
#if SIRIKATA_PLATFORM == PLATFORM_LINUX || SIRIKATA_PLATFORM == PLATFORM_MAC
    // FIXME -- set current dir to bin dir, but should set it to something app-specific
    Path::Set(Path::DIR_CURRENT, Path::Get(Path::DIR_EXE));

    String appExe = getExecutablePath(binary);
    binaryArgs.insert(binaryArgs.begin(), appExe);
    const char** execArgs = new const char*[binaryArgs.size()+1];
    for(uint32 i = 0; i < binaryArgs.size(); i++)
        execArgs[i] = binaryArgs[i].c_str();
    execArgs[binaryArgs.size()] = NULL;
    execCommand(appExe.c_str(), execArgs);
#elif SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    // FIXME -- set current dir to bin dir, but should set it to something app-specific
    Path::Set(Path::DIR_CURRENT, boost::filesystem::path(Path::Get(Path::DIR_EXE)).parent_path().string());

    String appExe = getExecutablePath(binary);
    binaryArgs.insert(binaryArgs.begin(), appExe);
    String cmd = "";
    for(int i = 0; i < binaryArgs.size(); i++)
        cmd = cmd + " " + binaryArgs[i];
    /* Helpful for debugging
    int msgboxID = MessageBox(
        NULL,
        cmd.c_str(),
        "slauncher",
        MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
    );
    */
    STARTUPINFO info={sizeof(info)};
    PROCESS_INFORMATION processInfo;
    CreateProcess(appExe.c_str(), (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
#endif
    eventLoopExit(retval, 0);
}

// ### Driver ###

int main(int argc, char** argv) {

    // Init and parse options

    InitOptions();
    InitLauncherOptions();
    ParseOptions(argc, argv);

    // Special simple case for registering handlers

    bool registerHandler = GetOptionValue<bool>("register");
    bool unregisterHandler = GetOptionValue<bool>("unregister");
    String uri = GetOptionValue<String>("uri");

    // Make default action be registration
    if (registerHandler || (!unregisterHandler && uri.empty()))
        return (registerURIHandler() ? 0 : -1);

    if (unregisterHandler)
        return (unregisterURIHandler() ? 0 : -1);

    // Basic app setup

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* iostrand = ios->createStrand();
    startWork(ios);

    Trace::Trace* trace = new Trace::Trace("slauncher.log");
    Time epoch = Timer::now();

    Context* ctx = new Context("slauncher", ios, iostrand, trace, epoch);

    Transfer::TransferPoolPtr transferPool = Transfer::TransferMediator::getSingleton().registerClient<Transfer::AggregatedTransferPool>("slauncher");

    // Start loading
    int retval = 0;
    bool launched = startLaunchURI(uri, transferPool, &retval);
    // Only start the process if we
    if (!launched) return -1;

    // Run
    ctx->add(ctx);
    ctx->run(1);

    // Cleanup
    ctx->cleanup();
    trace->prepareShutdown();
    delete ctx;
    delete trace;

    delete iostrand;
    Network::IOServiceFactory::destroyIOService(ios);

    return retval;
}
