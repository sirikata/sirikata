/*  Sirikata utilities
 *  HashMap.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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

#ifndef SIRIKATA_HashMap_HPP__
#define SIRIKATA_HashMap_HPP__

#if defined(HELL_HAS_FROZEN_OVER)
// C++0x released and supported by modern compilers!
# include <unordered_map>
# define HASH std::hash
# define HashMap std::unordered_map

#elif defined(_MSC_VER)
// Microsoft hash_map
# include <hash_map>
# define HASH stdext::hash
# define HashMap stdext::hash_map

#else
# if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2))
// G++ proprietary hash_map -- gives #warning in GCC 4.3
#  include <ext/hash_map>
#  define HASH __gnu_cxx::hash
#  define HashMap __gnu_cxx::hash_map

# else
// Proposed standard tr1 hash_map (supported in newer compilers)
#  include <tr1/unordered_map>
#  define HashMap std::tr1::unordered_map
#  define HASH std::tr1::hash

# endif
#endif


#endif
