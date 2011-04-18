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

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#include<shellapi.h>
#endif

size_t writehandler(void*ptr, size_t size, size_t nmemb, std::string* userdata) {
    *userdata += std::string((const char*)ptr, size*nmemb);
    return size*nmemb;
}

int main(int argc, char** argv) {
    // crashreporter report_url minidumppath minidumpfile version git_hash
    assert(argc == 6);
    // The file is called minidumppath/minidumpfile.dmp

    const char* report_url = argv[1];
    std::string dumppath = std::string(argv[2]) + std::string("/");
    std::string dumpfilename = std::string(argv[3]) + std::string(".dmp");
    std::string fulldumpfile = dumppath + dumpfilename;
    std::string sirikata_version = std::string(argv[4]);
    std::string sirikata_git_hash = std::string(argv[5]);

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

        curl_easy_setopt(curl, CURLOPT_URL, report_url);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultStr);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writehandler);
        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl_formfree(formpost);

        if (!resultStr.empty()) {
#if SIRIKATA_PLATFORM == PLATFORM_LINUX
            execlp("xdg-open", "xdg-open", resultStr.c_str(), (char*)NULL);
#elif SIRIKATA_PLATFORM == PLATFORM_WINDOWS
	    ShellExecute(NULL, "open", resultStr.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
        }
    }

    return 0;
}
