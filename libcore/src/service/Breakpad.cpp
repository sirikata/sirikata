/*  Sirikata
 *  Breakpad.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/service/Breakpad.hpp>

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/options/CommonOptions.hpp>

#ifdef HAVE_BREAKPAD
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#include <client/windows/handler/exception_handler.h>
#elif SIRIKATA_PLATFORM == PLATFORM_LINUX
#include <client/linux/handler/exception_handler.h>
#endif
#endif // HAVE_BREAKPAD

namespace Sirikata {
namespace Breakpad {

#ifdef HAVE_BREAKPAD

// Each implementation of ExceptionHandler and the setup are different enough
// that these are worth just completely separating. Each just needs to setup the
// exception handler. Currently, all should set it up to save minidumps to the
// current directory.

#if SIRIKATA_PLATFORM  == PLATFORM_WINDOWS
namespace {

static google_breakpad::ExceptionHandler* breakpad_handler = NULL;
static std::string breakpad_url;

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

bool finishedDump(const wchar_t* dump_path,
    const wchar_t* minidump_id,
    void* context,
    EXCEPTION_POINTERS* exinfo,
    MDRawAssertionInfo* assertion,
    bool succeeded) {
    printf("Finished breakpad dump at %s/%s.dmp: success %d\n", dump_path, minidump_id, succeeded ? 1 : -1);


// Only run the reporter in release mode. This is a decent heuristic --
// generally you'll only run the debug mode when you have a dev environment.
#if SIRIKATA_DEBUG
    return succeeded;
#else
    if (breakpad_url.empty()) return succeeded;

    const char* reporter_name =
#if SIRIKATA_DEBUG
      "crashreporter_d.exe"
#else
      "crashreporter.exe"
#endif
      ;

    STARTUPINFO info={sizeof(info)};
    PROCESS_INFORMATION processInfo;
    std::string cmd = reporter_name + std::string(" ") + breakpad_url + std::string(" ") + wchar_to_string(dump_path) + std::string(" ") + wchar_to_string(minidump_id);
    CreateProcess(reporter_name, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);

    return succeeded;
#endif // SIRIKATA_DEBUG
}
}

void init() {
    if (breakpad_handler != NULL) return;

    // This is needed for CRT to not show dialog for invalid param
    // failures and instead let the code handle it.
    _CrtSetReportMode(_CRT_ASSERT, 0);

    breakpad_url = GetOptionValue<String>(OPT_CRASHREPORT_URL);

    using namespace google_breakpad;
    breakpad_handler = new ExceptionHandler(L".\\",
        NULL,
        finishedDump,
        NULL,
        ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal,
        NULL,
        NULL);
}

#elif SIRIKATA_PLATFORM == PLATFORM_LINUX

namespace {

static google_breakpad::ExceptionHandler* breakpad_handler = NULL;
static std::string breakpad_url;

bool finishedDump(const char* dump_path,
    const char* minidump_id,
    void* context,
    bool succeeded) {
    printf("Finished breakpad dump at %s/%s.dmp: success %d\n", dump_path, minidump_id, succeeded ? 1 : -1);

// Only run the reporter in release mode. This is a decent heuristic --
// generally you'll only run the debug mode when you have a dev environment.
#if SIRIKATA_DEBUG
    return succeeded;
#else
    // If no URL, just finish crashing after the dump.
    if (breakpad_url.empty()) return succeeded;

    // Fork and exec the crashreporter
    pid_t pID = fork();

    if (pID == 0) {
        const char* reporter_name =
#if SIRIKATA_DEBUG
            "crashreporter_d"
#else
            "crashreporter"
#endif
            ;

        execlp(reporter_name, reporter_name, breakpad_url.c_str(), dump_path, minidump_id, (char*)NULL);
        // If crashreporter not in path, try current directory
        execl(reporter_name, reporter_name, breakpad_url.c_str(), dump_path, minidump_id, (char*)NULL);
    }
    else if (pID < 0) {
        printf("Failed to fork crashreporter\n");
    }

    return succeeded;
#endif //SIRIKATA_DEBUG
}
}

void init() {
    if (breakpad_handler != NULL) return;

    breakpad_url = GetOptionValue<String>(OPT_CRASHREPORT_URL);

    using namespace google_breakpad;
    breakpad_handler = new ExceptionHandler("./", NULL, finishedDump, NULL, true);
}

#elif SIRIKATA_PLATFORM == PLATFORM_MAC
// No mac support currently
void init() {
}

#endif

#else //def HAVE_BREAKPAD
// Dummy implementation
void init() {
}
#endif

} // namespace Breakpad
} // namespace Sirikata
