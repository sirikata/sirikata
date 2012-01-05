/*  Sirikata
 *  main.cpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

// NOTE: This is only to determine platform. Don't use libcore types.
#include <sirikata/core/util/Platform.hpp>

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <string>

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#include<shellapi.h>
#endif

#ifdef HAVE_BREAKPAD
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#include <client/windows/crash_generation/crash_generation_server.h>
#include <client/windows/crash_generation/client_info.h>
#endif
#endif

size_t writehandler(void*ptr, size_t size, size_t nmemb, std::string* userdata) {
    *userdata += std::string((const char*)ptr, size*nmemb);
    return size*nmemb;
}

void reportCrash(const std::string& report_url, const std::string&dumpfilename,  const std::string& fulldumpfile, const std::string& sirikata_version, const std::string& sirikata_git_hash) {
    CURL* curl;
    CURLcode res;

    curl_httppost* formpost = NULL;
    curl_httppost* lastptr = NULL;

    curl_global_init(CURL_GLOBAL_ALL);

    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "dump",
        CURLFORM_FILE, fulldumpfile.c_str(),
        CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "dumpname",
        CURLFORM_COPYCONTENTS, dumpfilename.c_str(),
        CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "version",
        CURLFORM_COPYCONTENTS, sirikata_version.c_str(),
        CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "githash",
        CURLFORM_COPYCONTENTS, sirikata_git_hash.c_str(),
        CURLFORM_END);

    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "submit",
        CURLFORM_COPYCONTENTS, "send",
        CURLFORM_END);

    curl = curl_easy_init();
    if (curl) {
        std::string resultStr;

        curl_easy_setopt(curl, CURLOPT_URL, report_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultStr);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writehandler);
        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl_formfree(formpost);

        if (!resultStr.empty()) {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
            execlp("xdg-open", "xdg-open", resultStr.c_str(), (char*)NULL);
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
	    ShellExecute(NULL, "open", resultStr.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
        }
    }
}

// All crash generation server code is only relevant with breakpad
#ifdef HAVE_BREAKPAD

// Currently only enabled for windows
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS


std::string wchar_to_string(const wchar_t* orig) {
  size_t origsize = wcslen(orig) + 1;
  const size_t newsize = origsize;
  size_t convertedChars = 0;
  char* nstring = new char[newsize];
  wcstombs_s(&convertedChars, nstring, origsize, orig, _TRUNCATE);
  std::string res(nstring);
  delete nstring;
  return res;
}

std::string wstring_to_string(const std::wstring& orig) {
    return wchar_to_string(orig.c_str());
}

// Convert to a wchar_t*
std::wstring str_to_wstring(const std::string& orig) {
    size_t origsize = orig.size() + 1;
    const size_t newsize = origsize;
    size_t convertedChars = 0;
    wchar_t* wcstring = new wchar_t[newsize];
    mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
    std::wstring res(wcstring);
    delete wcstring;
    return res;
}

static const wchar_t kPipeName[] = L"\\\\.\\pipe\\SirikataCrashServices";

static google_breakpad::CrashGenerationServer* breakpad_server = NULL;
static int n_connected = 0; // Current total connections
static int n_sessions = 0; // Sessions seen so far
static int n_work = 0;
static std::string g_report_url;
static std::string g_dump_path;

void OnClientConnected(void* context,
    const google_breakpad::ClientInfo* client_info) {
    //std::cout << "Client connected, pid = "  << client_info->pid() <<  std::endl;
    // FIXME thread safety
    n_connected++;
    n_sessions++;
}

void OnClientExited(void* context,
    const google_breakpad::ClientInfo* client_info) {
    //std::cout << "Client exited" << std::endl;

    // FIXME thread safety
    n_connected--;
}

void OnClientDumpRequest(void* context,
    const google_breakpad::ClientInfo* client_info,
    const std::wstring* file_path) {

    using namespace google_breakpad;

    n_work++;

    if (file_path == NULL || client_info == NULL) {
        n_work--;
        return;
    }

    //std::wcout << L"Client dump request: " << *file_path << std::endl;

    // Decode client info
    std::wstring version, githash;
    CustomClientInfo custom_info = client_info->GetCustomInfo();
    for(int i = 0; i < custom_info.count; i++) {
        if (std::wstring(custom_info.entries[i].name) == L"version")
            version = std::wstring(custom_info.entries[i].value);
        else if (std::wstring(custom_info.entries[i].name) == L"githash")
            githash = std::wstring(custom_info.entries[i].value);
    }

    // Finally do the report
    // In this case we just get the full path, so we need to strip off
    // the dump name
    std::string fulldumpfile = wstring_to_string(*file_path);
    std::string dumpfile = fulldumpfile.substr(fulldumpfile.rfind('\\')+1);
    reportCrash(
        g_report_url,
        dumpfile,
        fulldumpfile,
        wstring_to_string(version),
        wstring_to_string(githash)
    );

    n_work--;
}

#endif // SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#endif // HAVE_BREAKPAD

int main(int argc, char** argv) {
    // We can run crashreporter in one of two ways. First, as just the
    // crashreporter, triggered by in-process minidump generation.  In this case
    // we run with the command line:
    // crashreporter report_url minidumppath minidumpfile version git_hash
    if (argc == 6) {
        // The file is called minidumppath/minidumpfile.dmp
        std::string report_url = std::string(argv[1]);
        std::string dumppath = std::string(argv[2]) + std::string("/");
        std::string dumpfilename = std::string(argv[3]) + std::string(".dmp");
        std::string fulldumpfile = dumppath + dumpfilename;
        std::string sirikata_version = std::string(argv[4]);
        std::string sirikata_git_hash = std::string(argv[5]);
        reportCrash(report_url, dumpfilename, fulldumpfile, sirikata_version, sirikata_git_hash);
        return 0;
    }
#ifdef HAVE_BREAKPAD // All crash generation server code is only relevant with breakpad
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS // windows only OOP right now
    else {
        // Otherwise, we're in out of process minidump generation + crashreport
        // service mode: we setup to listen for crashes, help the other process
        // get a dump, and finally file the reports ourselves. In this
        // case, we invoke with the following signature:
        // crashreporter report_url minidumppath
        // The remaining values are provided by breakpad or through
        // CustomClientInfo
        assert(argc == 3);
        std::string report_url = std::string(argv[1]);
        std::string dumppath = std::string(argv[2]) + std::string("/");
        g_report_url = report_url;
        g_dump_path = dumppath;

        using namespace google_breakpad;
        const wchar_t* pipe_name = kPipeName;
        std::wstring wdumppath = str_to_wstring(dumppath);
        breakpad_server = new CrashGenerationServer(
            pipe_name, NULL,
            OnClientConnected, NULL,
            OnClientDumpRequest, NULL,
            OnClientExited, NULL,
            true, &wdumppath
        );

        if (!breakpad_server->Start()) {
            printf("crashreporter couldn't start crash generation server. Exiting\n");
            return -1;
        }

        // "Processing" loop, server manages everything in the
        // background. The condition for exit is a bit confusing.
        // When we start things up, we need to wait for a connection,
        // in which case the *normal* exit condition -- no connections
        // and no on-going work -- will be true. Therefore, we also
        // keep track of total sessions seen and number of times we've
        // run through the loop and found no clients or work, allowing
        // us to exit if we don't get clients in a reasonable time
        // period, but exit promptly if we've already seen clients and
        // they are just gone.
        int n_empty_its = 0;
        while(true) {
            if (n_connected == 0 && n_work == 0) {
                n_empty_its++;
                // Arbitrary 15 second wait for client that triggered us
                if ((n_sessions == 0 && n_empty_its > 15) || n_sessions > 0)
                    break;
            }
            Sleep(1000);
        }
    }
#endif // WINDOWS
#endif // HAVE_BREAKPAD

    return 0;
}
