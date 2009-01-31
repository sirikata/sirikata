/*  Sirikata -- Platform Dependent Definitions
 *  Platform.hpp
 *
 *  Copyright (c) 2008, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_PLATFORM_HPP_
#define _SIRIKATA_PLATFORM_HPP_


#define PLATFORM_WINDOWS 0
#define PLATFORM_MAC     1
#define PLATFORM_LINUX   2


#if defined(__WIN32__) || defined(_WIN32)
#  define SIRIKATA_PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE_CC__) || defined(__APPLE__)
#  define SIRIKATA_PLATFORM PLATFORM_MAC
#  ifndef __MACOSX__
#    define __MACOSX__
#  endif
#else
#  define SIRIKATA_PLATFORM PLATFORM_LINUX
#endif


#ifndef SIRIKATA_EXPORT
# if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#   if defined(STATIC_LINKED)
#     define SIRIKATA_EXPORT
#   else
#     if defined(SIRIKATA_BUILD)
#       define SIRIKATA_EXPORT __declspec(dllexport)
#     else
#       define SIRIKATA_EXPORT __declspec(dllimport)
#     endif
#   endif
# else
#   if defined(__GNUC__) && defined(GCC_HASVISIBILITY)
#     define SIRIKATA_EXPORT __attribute__ ((visibility("default")))
#   else
#     define SIRIKATA_EXPORT
#   endif
# endif
#endif

#ifndef SIRIKATA_EXPORT_C
# define SIRIKATA_EXPORT_C extern "C" SIRIKATA_EXPORT
#endif

# ifdef __GLIBC__
#  include <endian.h>
#  define SIRIKATA_LITTLE_ENDIAN __LITTLE_ENDIAN
#  define SIRIKATA_BIG_ENDIAN    __BIG_ENDIAN
#  define SIRIKATA_BYTE_ORDER    __BYTE_ORDER
# elif defined(__APPLE__) || defined(MACOSX) || defined(BSD) || defined(__FreeBSD__)
#  include<machine/endian.h>
#  ifdef BYTE_ORDER
#   define SIRIKATA_LITTLE_ENDIAN LITTLE_ENDIAN
#   define SIRIKATA_BIG_ENDIAN    BIG_ENDIAN
#   define SIRIKATA_BYTE_ORDER    BYTE_ORDER
#  else 
    #error "MACINTOSH DOES NOT DEFINE ENDIANNESS"    
#  endif
# else
#  define SIRIKATA_LITTLE_ENDIAN 1234
#  define SIRIKATA_BIG_ENDIAN    4321
#  ifdef _BIG_ENDIAN
#   define SIRIKATA_BYTE_ORDER SIRIKATA_BIG_ENDIAN
#   ifdef _LITTLE_ENDIAN
#    error "BOTH little and big endian defined"
#   endif
#  else
#   ifdef _LITTLE_ENDIAN
#    define SIRIKATA_BYTE_ORDER SIRIKATA_LITTLE_ENDIAN
#   elif defined(__sparc) || defined(__sparc__) \
      || defined(_POWER) || defined(__powerpc__) \
      || defined(__ppc__) || defined(__hpux) \
      || defined(_MIPSEB) || defined(_POWER) \
      || defined(__s390__)
#    define SIRIKATA_BYTE_ORDER SIRIKATA_BIG_ENDIAN
#   elif defined(__i386__) || defined(__alpha__) \
   ||    defined(__ia64) || defined(__ia64__) \
   ||    defined(_M_IX86) || defined(_M_IA64) \
   ||    defined(_M_ALPHA) || defined(__amd64) \
   ||    defined(__amd64__) || defined(_M_AMD64) \
   ||    defined(__x86_64) || defined(__x86_64__) \
   ||    defined(_M_X64)
#    define SIRIKATA_BYTE_ORDER SIRIKATA_LITTLE_ENDIAN
#   else
#    error "Not a known CPU type"
#   endif
#  endif
# endif
#endif
#include <string>
#include <map>
#include <set>
#include <boost/tr1/memory.hpp>
#include <boost/tr1/array.hpp>
#include <boost/tr1/functional.hpp>
#include <boost/tr1/unordered_set.hpp>
#include <boost/tr1/unordered_map.hpp>

#define tech std::tr1
namespace Sirikata {

// numeric typedefs to get standardized types
typedef unsigned char uchar;
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
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
typedef float float32;
typedef double float64;

typedef uchar byte;

typedef std::string String;

} // namespace Sirikata

#endif //_SIRIKATA_PLATFORM_HPP_
