/*  Sirikata
 *  Platform.hpp
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

#ifndef _EMERSON_PLATFORM_HPP_
#define _EMERSON_PLATFORM_HPP_


#define EMERSON_PLATFORM_WINDOWS 0
#define EMERSON_PLATFORM_LINUX   1
#define EMERSON_PLATFORM_MAC     2


#if defined(__WIN32__) || defined(_WIN32)
// disable type needs to have dll-interface to be used byu clients due to STL member variables which are not public
#pragma warning (disable: 4251)
//disable warning about no suitable definition provided for explicit template instantiation request which seems to have no resolution nor cause any problems
#pragma warning (disable: 4661)
//disable non dll-interface class used as base for dll-interface class when deriving from singleton
#pragma warning (disable : 4275)
#  define EMERSON_PLATFORM EMERSON_PLATFORM_WINDOWS
#elif defined(__APPLE_CC__) || defined(__APPLE__)
#  define EMERSON_PLATFORM EMERSON_PLATFORM_MAC
#  ifndef __MACOSX__
#    define __MACOSX__
#  endif
#else
#  define EMERSON_PLATFORM EMERSON_PLATFORM_LINUX
#endif

namespace Emerson {

// numeric typedefs to get standardized types
typedef unsigned char uchar;
#if EMERSON_PLATFORM == EMERSON_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
typedef __int8 int8;
typedef unsigned __int8 uint8;
typedef __int16 int16;
typedef unsigned __int16 uint16;
typedef __int32 int32;
typedef unsigned __int32 uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;

#else
# include <stdint.h>
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
#endif

} // namespace Emerson

#endif // _EMERSON_PLATFORM_HPP_
