/*  Sirikata
 *  Frame.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava.
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

#ifndef _SIRIKATA_LIBCORE_NETWORK_FRAME_HPP_
#define _SIRIKATA_LIBCORE_NETWORK_FRAME_HPP_

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {
namespace Network {

/** A collection of simpling framing routines for network messages you want to
 *  send on a stream. This just provides utilities for writing a frame around
 *  the data and reading them back, avoiding duplicated error-prone code.
 */
struct SIRIKATA_EXPORT Frame {
    /** Writes the data to the stream as a message. */
    static std::string write(const void* data, uint32 len);
    static std::string write(const std::string& data);

    /** Checks if a full message is available. Returns the contents and removes
     *  them from the argument if it has a whole packet; returns an empty string
     *  and does nothing to the argument if it does not have a whole packet.
     */
    static std::string parse(std::string& data);

    /** Checks if a full message is available. Returns the contents and removes
     *  them from the argument if it has a whole packet; returns an empty string
     *  and does nothing to the argument if it does not have a whole packet.
     */
    static std::string parse(std::stringstream& data);
};

} // namespace Network
} // namespace Frame

#endif //_SIRIKATA_LIBCORE_NETWORK_FRAME_HPP_
