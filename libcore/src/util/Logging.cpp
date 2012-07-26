/*  Sirikata Utilities -- Sirikata Logging Utility
 *  Logging.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include <sirikata/core/options/Options.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#include <io.h>
#define fileno_platform _fileno
#define dup2_platform _dup2
#else
#define fileno_platform fileno
#define dup2_platform dup2
#endif

extern "C" {
void *Sirikata_Logging_OptionValue_defaultLevel;
void *Sirikata_Logging_OptionValue_atLeastLevel;
void *Sirikata_Logging_OptionValue_moduleLevel;
}
namespace Sirikata { namespace Logging {

extern "C" {
    // Redirect for just SILOG
    std::ostream* SirikataLogStream = &std::cerr;
}

namespace {
// Redirect for entire stdout, stderr. By default we don't do any redirection.
FILE* SirikataLogFP = NULL;

class RedirectBuf : public std::streambuf {
public:
    RedirectBuf() {
        setp(0, 0);
    }

    virtual int_type overflow(int_type c = traits_type::eof()) {
        return fputc(c, stdout) == EOF ? traits_type::eof() : c;
    }

    virtual int sync() {
	return fflush(stdout);
    }
};

RedirectBuf* SirikataRedirectCPPOut = NULL;
std::streambuf* orig_cout_buf = NULL;
std::streambuf* orig_cerr_buf = NULL;
}
void setOutputFP(FILE* fp) {
    int stdout_fileno = fileno_platform(stdout);
    dup2(fileno_platform(fp), stdout_fileno);

    int stderr_fileno = fileno_platform(stderr);
    dup2(fileno_platform(stderr), stderr_fileno);

    // In some places, we don't get stdout == cout and stderr == cerr, so we
    // need to redirect those as well.

    // set std::cout to use my custom streambuf
    SirikataRedirectCPPOut = new RedirectBuf();
    orig_cout_buf = std::cout.rdbuf(SirikataRedirectCPPOut);
    orig_cerr_buf = std::cerr.rdbuf(SirikataRedirectCPPOut);
}

void setLogStream(std::ostream* logfs) {
    SirikataLogStream = logfs;
}

void finishLog() {
    SirikataLogStream->flush();
    if (SirikataLogStream != &std::cerr) {
        delete SirikataLogStream;
        SirikataLogStream = &std::cerr;
    }

    if (SirikataLogFP != NULL) {
        fflush(SirikataLogFP);
        fclose(SirikataLogFP);
    }

    if (SirikataRedirectCPPOut) {
        // make sure to restore the original so we don't get a crash on close!
        std::cout.rdbuf(orig_cout_buf);
        std::cerr.rdbuf(orig_cerr_buf);
        delete SirikataRedirectCPPOut;
    }
}

typedef std::tr1::unordered_map<const char*, String> CapsNameMap;
static CapsNameMap LogModuleStrings;

typedef boost::shared_mutex SharedMutex;
typedef boost::upgrade_lock<SharedMutex> UpgradeLock;
typedef boost::upgrade_to_unique_lock<SharedMutex> UpgradedLock;
SharedMutex LogModuleStringsMutex;

const String& LogModuleString(const char* base) {
    UpgradeLock upgrade_lock(LogModuleStringsMutex);

    CapsNameMap::iterator it = LogModuleStrings.find(base);

    if (it == LogModuleStrings.end()) {
        String base_str(base);
        boost::to_upper(base_str);

        // Cap/extend
#define LOGGING_MAX_MODULE_LENGTH 10
        if (base_str.size() > LOGGING_MAX_MODULE_LENGTH)
            base_str = base_str.substr(0, LOGGING_MAX_MODULE_LENGTH);
        if (base_str.size() < LOGGING_MAX_MODULE_LENGTH)
            base_str = String(LOGGING_MAX_MODULE_LENGTH-base_str.size(), ' ') + base_str;

        UpgradedLock upgraded_lock(upgrade_lock);
        std::pair<CapsNameMap::iterator, bool> inserted = LogModuleStrings.insert(std::make_pair(base, base_str));
        return (inserted.first)->second;
    }

    return it->second;
}

const char* LogLevelString(LOGGING_LEVEL lvl, const char* lvl_as_string) {
    switch(lvl) {
        // Note these are all setup to be aligned/the same length. The default
        // case, which represents an unexpected level, will break this padding.
      case fatal:    return "FATAL   "; break;
      case error:    return "ERROR   "; break;
      case warning:  return "WARNING "; break;
      case info:     return "INFO    "; break;
      case debug:    return "DEBUG   "; break;
      case detailed: return "DETAILED"; break;
      case insane:   return "INSANE  "; break;
      default:       return lvl_as_string; break;
    }
}

class LogLevelParser {public:
    static LOGGING_LEVEL lex_cast(const std::string&value) {
        if (value=="warning")
            return warning;
        if (value=="info")
            return info;
        if (value=="error")
            return error;
        if (value=="fatal")
            return fatal;
        if (value=="debug")
            return debug;
        if (value=="detailed")
            return detailed;
        return insane;
    }
    Any operator()(const std::string&value) {
        return lex_cast(value);
    }
};
class LogLevelMapParser {public:
    Any operator()(const std::string&cvalue) {
        std::tr1::unordered_map<std::string,LOGGING_LEVEL> retval;
        std::string value=cvalue;
        while (true) {
            std::string::size_type where=value.find_first_of(",");
            std::string comma;
            if (where!=std::string::npos) {
                comma=value.substr(0,where);
                value=value.substr(where+1);
            }else {
                comma=value;
            }
            std::string::size_type whereequal=comma.find_first_of("=");
            if (whereequal!=std::string::npos) {
                retval[comma.substr(0,whereequal)]=LogLevelParser::lex_cast(comma.substr(whereequal+1));
            }
            if (where==std::string::npos) {
                break;
            }
        }
        return retval;
    }
};

InitializeGlobalOptions o("",
                    Sirikata_Logging_OptionValue_defaultLevel=new OptionValue("loglevel",
#ifdef NDEBUG
                                                 "info",
#else
                                                 "debug",
#endif
                                                 "Sets the default level for logging when no per-module override",
                                                 LogLevelParser()),
                    Sirikata_Logging_OptionValue_atLeastLevel=new OptionValue("maxloglevel",
#ifdef NDEBUG
                                                 "info",
#else
                                                 "insane",
#endif
                                                 "Sets the maximum logging level any module may be set to",
                                                 LogLevelParser()),
                    Sirikata_Logging_OptionValue_moduleLevel=new OptionValue("moduleloglevel",
                                                "",
                                                "Sets a per-module logging level: should be formatted <module>=debug,<othermodule>=info...",
                                                LogLevelMapParser()),
                     NULL);

std::tr1::unordered_map<std::string,LOGGING_LEVEL> module_level;

} }
