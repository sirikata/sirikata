/*  Sirikata Utilities -- Sirikata Logging Utility
 *  Logging.hpp
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
namespace Sirikata {
class OptionValue;
namespace Logging {
enum LOGGING_LEVEL {
    fatal=1,
    error=8,
    warning=64,
    info=512,
    debug=4096,
    insane=32768
};
extern OptionValue* defaultLevel;
extern OptionValue* atLeastLevel;
extern OptionValue* moduleLevel;
} }
#if 1
# ifdef DEBUG_ALL
#  define SILOGP(module,lvl) true
# else
#  define SILOGP(module,lvl) \
    (Sirikata::Logging::defaultLevel->as<Sirikata::Logging::LOGGING_LEVEL>()>=Sirikata::Logging::lvl&& \
        ( (Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().find(#module)==Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().end() && \
           Sirikata::Logging::defaultLevel->as<Sirikata::Logging::LOGGING_LEVEL>()>=(Sirikata::Logging::lvl)) \
          || (Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().find(#module)!=Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().end() && \
              Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >()[#module]>=Sirikata::Logging::lvl)))
# endif
# define SILOGNOCR(module,lvl,value) \
    if (SILOGP(module,lvl)) \
        std::cerr << value
# define SILOG(module,lvl,value) SILOGNOCR(module,lvl,value) << std::endl
#else
# define SILOGP(module,lvl) false
# define SILOGNOCR(module,lvl,value)
# define SILOG(module,lvl,value)
#endif
