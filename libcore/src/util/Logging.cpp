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

extern "C" {
void *Sirikata_Logging_OptionValue_defaultLevel;
void *Sirikata_Logging_OptionValue_atLeastLevel;
void *Sirikata_Logging_OptionValue_moduleLevel;
}
namespace Sirikata { namespace Logging {
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
