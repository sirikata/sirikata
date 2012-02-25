// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/util/Timer.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/transfer/URL.hpp>
#include <sirikata/core/util/Paths.hpp>
#include <boost/filesystem.hpp>
#include <sirikata/core/transfer/AggregatedTransferPool.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

#include <json_spirit/json_spirit.h>
#include <boost/foreach.hpp>

#define LAUNCHER_LOG(lvl, msg) SILOG(launcher, lvl, msg)

using namespace Sirikata;

Context* gContext = NULL;
Transfer::TransferPoolPtr gTransferPool;

// ### Utils ###

void InitLauncherOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue("register","false",Sirikata::OptionValueType<bool>(),"If true, registers this binary as handler"))
        .addOption(new OptionValue("unregister","false",Sirikata::OptionValueType<bool>(),"If true, unregisters this binary as handler"))
        .addOption(new OptionValue("uri","",Sirikata::OptionValueType<String>(),"The URI to launch"))
        .addOption(new OptionValue("debug","false",Sirikata::OptionValueType<bool>(),"If true, try to run under the debugger"))
        ;
}

String getExecutableName(String name) {
#if SIRIKATA_DEBUG
    name = name + "_d";
#endif
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
    name = name + ".exe";
#endif
    return name;
}

String getExecutablePath(String name) {
    String exe_dir = Path::Get(Path::DIR_EXE);

    name = getExecutableName(name);
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
    if (name == "cppoh" || name == "cppoh_d")
        name = name + ".app/Contents/MacOS/" + name;
#endif

    return (boost::filesystem::path(exe_dir) / name).string();
}


#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
void execCommand(const char* file, const char* const argv[], bool do_fork = true)
{
    pid_t pID = 0;
    if (do_fork)
        pID = fork();

    if (pID == 0) {

        // setsid() decouples this process from the parent, ensuring that the
        // exit of the parent doesn't kill the child process by accident
        // (e.g. by causing the parent terminal to exit).
        setsid();
        execvp(file, (char* const*)argv);
    }
    else if (pID < 0) {
        LAUNCHER_LOG(fatal, "Couldn't fork to execute registration commands.");
    }
}
#endif

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
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

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX

    // We register in two ways. The first way is for Firefox/Gnome and just adds
    // the appropriate command for the uri type to gconf. xdg-utils, it seems,
    // *sometimes* picks up this one as well.

    // Note that in both cases, we use --uri sirikata:foobar rather than
    // --uri=sirikata:foobar. Sometimes the launcher (e.g. xdg-open) insists on
    // splitting the url portion into a separate argument, even if we didn't
    // leave a space, so we are forced to use this format.
    String cmd = exe_file + " --uri %s";
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

    // The second registration is through xdg-utils. We generate a .desktop
    // entry for the current user (putting it in their
    // ~/.local/share/applications). This should work across a bunch of
    // browsers, work from the command line, and is also used as a fallback in
    // some cases.
    // TODO(ewencp) we could probably improve this by detecting root user and
    // installing globally in that case

    {
        FILE* desktop_fp = fopen("/tmp/sirikata-slauncher.desktop", "w");
        if (!desktop_fp) {
            LAUNCHER_LOG(error, "Couldn't create temporary .desktop file.");
        }
        else {
            fprintf(desktop_fp, "[Desktop Entry]\n");
            fprintf(desktop_fp, "Name=Sirikata Launcher\n");
            fprintf(desktop_fp, "Comment=Launcher for Sirikata Virtual Worlds\n");
            String desktop_cmd = exe_file + " '--uri %u'";
            fprintf(desktop_fp, "Exec=%s\n", desktop_cmd.c_str());
            fprintf(desktop_fp, "Terminal=%s\n", needs_terminal.c_str());
            fprintf(desktop_fp, "Type=Application\n");
            fprintf(desktop_fp, "Categories=Network\n");
            fprintf(desktop_fp, "MimeType=x-scheme-handler/sirikata\n");
            fclose(desktop_fp);

            const char* const desktop_install_argv[] = { "xdg-desktop-menu", "install", "/tmp/sirikata-slauncher.desktop", NULL};
            execCommand("xdg-desktop-menu", desktop_install_argv);
        }
    }


#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
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
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
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

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
    const char* const enabled_argv[] = { "gconftool-2", "-t", "bool", "-s", "/desktop/gnome/url-handlers/sirikata/enabled", "false", NULL};
    execCommand("gconftool-2", enabled_argv);
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
    bool success =
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell\\open\\command") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell\\open") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\shell") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata\\Default Icon") &&
        deleteRegKey(HKEY_CLASSES_ROOT, "sirikata");
    LAUNCHER_LOG(error, "Failed to unregister handler in system registry.");
    return success;
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
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

void finishLaunchURI(Transfer::URI config_uri, Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr req, Transfer::DenseDataPtr data, int* retval);
void finishDownloadResource(const String& data_path, Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr req, Transfer::DenseDataPtr data, int* retval);
void doExecApp(int* retval);

Transfer::ResourceDownloadTaskPtr rdl;
bool startLaunchURI(String uri_str, int* retval) {
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
            config_uri, gTransferPool, 1.0,
            gContext->mainStrand->wrap(std::tr1::bind(finishLaunchURI, config_uri, std::tr1::placeholders::_1, std::tr1::placeholders::_2, std::tr1::placeholders::_3, retval))
        );
    rdl->start();

    return true;
}

void eventLoopExit(int* retval, int val) {
    stopWork();
    *retval = val;
}

String app;
String appDir;
String binary;
std::vector<String> binaryArgs;
String appDirPath() {
    return (boost::filesystem::path(Path::Get(Path::DIR_USER_HIDDEN)) / appDir).string();
}

typedef std::map<String, Transfer::ResourceDownloadTaskPtr> ResourceDownloadMap;
ResourceDownloadMap resourceDownloads;

void finishLaunchURI(Transfer::URI config_uri, Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr req, Transfer::DenseDataPtr data, int* retval) {
    // Fire off the request for
    if (!data || data->size() == 0) {
        LAUNCHER_LOG(error, "Failed to download config");
        eventLoopExit(retval, -1);
        return;
    }

    LAUNCHER_LOG(detailed, "Downloaded config, size " << data->size());

    // Parse the data
    namespace json = json_spirit;
    json::Value config;
    if (!json::read(data->asString(), config)) {
        LAUNCHER_LOG(error, "Invalid JSON configuration data.");
        eventLoopExit(retval, -1);
        return;
    }

    try {
        app = config.getString("app.name");
        appDir = config.getString("app.directory");
        // First, we need to know what app we're executing
        binary = config.getString("binary.name");
        // We also take a dictionary of arguments to add to the command
        // line
        BOOST_FOREACH(json::Object::value_type &v, config.getObject("binary.args")) {
            if (!v.second.isString()) {
                LAUNCHER_LOG(error, "Non-string binary.args value");
                eventLoopExit(retval, -1);
                return;
            }
            binaryArgs.push_back( String("--") + v.first + "=" + v.second.getString() );
        }
    }
    catch(const json::Value::PathError& path_exc) {
        LAUNCHER_LOG(error, "Required configuration properties not found.");
        eventLoopExit(retval, -1);
        return;
    }

    // Make sure our app path has been created
    boost::filesystem::path app_dir_path(appDirPath());
    if (!boost::filesystem::exists(app_dir_path)) {
        if (!boost::filesystem::create_directory(app_dir_path)) {
            LAUNCHER_LOG(error, "Application directory didn't exist and failed to create it.");
            eventLoopExit(retval, -1);
            return;
        }
    }
    LAUNCHER_LOG(info, "Using appliction directory: " << appDirPath());

    // This is not required data
    if (!config.contains("app.files")) {
        // If we don't have extra files, start the app now
        if (resourceDownloads.empty())
            doExecApp(retval);
    }
    const json::Value& app_files = config.get("app.files");
    if (!app_files.isObject()) {
        LAUNCHER_LOG(error, "app.files was specified, but doesn't contain a map of files.");
        eventLoopExit(retval, -1);
        return;
    }

    // We allow syncing of data that's required for the app
    BOOST_FOREACH(const json::Object::value_type &v, app_files.getObject()) {
        String data_path(v.first);

        if (!v.second.isString()) {
            LAUNCHER_LOG(error, "Non-string filename in app.files.");
            eventLoopExit(retval, -1);
            return;
        }

        // Handle relative URLs carefullly.
        // By default, we'll just try handling
        Transfer::URI data_uri(v.second.getString());
        // And we'll only override it with a relative one if the relative
        // one can be decoded as a URL.
        Transfer::URL config_url(config_uri);
        if (!config_url.empty()) {
            // Constructor figures out absolute/relative, and just fails if
            // it can't construct a valid URL.
            Transfer::URL deriv_url(config_url.context(), v.second.getString());
            if (!deriv_url.empty())
                data_uri = Transfer::URI(deriv_url.toString());
        }

        LAUNCHER_LOG(detailed, "Download resource " << data_path << " from " << data_uri);
        Transfer::ResourceDownloadTaskPtr dl =
            Transfer::ResourceDownloadTask::construct(
                data_uri, gTransferPool, 1.0,
                gContext->mainStrand->wrap(
                    std::tr1::bind(finishDownloadResource,
                        data_path,
                        std::tr1::placeholders::_1, std::tr1::placeholders::_2,
                        std::tr1::placeholders::_3, retval
                    )
                )
            );
        dl->start();
        resourceDownloads[data_path] = dl;
    }
}

void finishDownloadResource(const String& data_path, Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr req, Transfer::DenseDataPtr data, int* retval) {
    if (!data || data->size() == 0) {
        LAUNCHER_LOG(error, "Failed to download data file: " << data_path);
        eventLoopExit(retval, -1);
        return;
    }

    // Store to disk
    boost::filesystem::path app_data_path(appDirPath());
    app_data_path /= data_path;

    // Make sure the directory exists. If we had sirikata/bin/demo/ as
    // the app data path and foo/bar/bin.db as the file, we need to
    // make sure sirikata/bin/demo/foo/bar/ exists, which is the
    // parent of the full data file path. We know from earlier that
    // we've already got sirikata/bin/demo/.
    if (!boost::filesystem::exists(app_data_path.parent_path()) &&
        !boost::filesystem::create_directories(app_data_path.parent_path())) {
        LAUNCHER_LOG(error, "Couldn't create data directory: " << app_data_path.parent_path().string());
        eventLoopExit(retval, -1);
        return;
    }

    String app_data_path_str = app_data_path.string();
    FILE* fp = fopen(app_data_path_str.c_str(), "wb");
    if (fp == NULL) {
        LAUNCHER_LOG(error, "Couldn't open file for writing: " << app_data_path_str);
        eventLoopExit(retval, -1);
    }
    fwrite(data->begin(), 1, data->size(), fp);
    fclose(fp);

    // Clear out record, and if we're ready launch the app
    resourceDownloads.erase(data_path);
    if (resourceDownloads.empty())
        doExecApp(retval);
}

void doExecApp(int* retval) {
    LAUNCHER_LOG(detailed, "Got config and all resources, executing application.");

    String appCurrDir = appDirPath();
    Path::Set(Path::DIR_CURRENT, appCurrDir);

    // We want to add support for syncing additional content (maybe
    // something like app.data = url for archive) which will require
    // additional downloading, but for now we just invoke the command
    // here.
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
    String appExe = getExecutablePath(binary);
    binaryArgs.insert(binaryArgs.begin(), appExe);

    bool do_fork = true;
    bool do_debug = GetOptionValue<bool>("debug");
    if (do_debug) {
        // We need to prefix gdb --args and change the app name to
        // gdb. Also don't fork so we can actually use gdb
        binaryArgs.insert(binaryArgs.begin(), "--args");
        binaryArgs.insert(binaryArgs.begin(), "gdb");
        appExe = "gdb";
        do_fork = false;
    }

    // Generate a version of the args that can be used by exec
    const char** execArgs = new const char*[binaryArgs.size()+1];
    for(uint32 i = 0; i < binaryArgs.size(); i++)
        execArgs[i] = binaryArgs[i].c_str();
    execArgs[binaryArgs.size()] = NULL;
    execCommand(appExe.c_str(), execArgs, do_fork);
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
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
    CreateProcess(appExe.c_str(), (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, appCurrDir.c_str(), &info, &processInfo);
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

    Network::IOService* ios = new Network::IOService("slauncher");
    Network::IOStrand* iostrand = ios->createStrand("slauncher Main");
    startWork(ios);

    Trace::Trace* trace = new Trace::Trace("slauncher.log");
    Time epoch = Timer::now();

    gContext = new Context("slauncher", ios, iostrand, trace, epoch);

    gTransferPool = Transfer::TransferMediator::getSingleton().registerClient<Transfer::AggregatedTransferPool>("slauncher");

    // Start loading
    int retval = 0;
    bool launched = startLaunchURI(uri, &retval);
    // Only start the process if we
    if (!launched) return -1;

    // Run
    gContext->add(gContext);
    gContext->run(1);

    // Cleanup
    gContext->cleanup();
    trace->prepareShutdown();
    delete gContext;
    delete trace;

    delete iostrand;
    delete ios;

    return retval;
}
