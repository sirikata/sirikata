/*  Sirikata
 *  Frame.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/network/Frame.hpp>

// htonl, ntohl
#include <sirikata/core/network/Asio.hpp>

namespace Sirikata {
namespace Network {

/** Writes the data to the stream as a message. */
std::string Frame::write(const void* data, uint32 len) {
    std::string result(len + sizeof(uint32), '\0');

    uint32 encoded_len = htonl(len);

    memcpy(&(result[0]), &encoded_len, sizeof(uint32));
    memcpy(&(result[sizeof(uint32)]), data, len);

    return result;
}

std::string Frame::write(const std::string& data) {
    return Frame::write(&(data[0]), data.size());
}

std::string Frame::parse(std::string& data) {
    std::string result;

    if (data.size() < sizeof(uint32)) return result;

    // Try to parse the length
    uint32 len;
    memcpy(&len, &(data[0]), sizeof(uint32));
    len = ntohl(len);

    // Make sure we have the full packet
    if (data.size() < sizeof(uint32) + len) return result;

    // Extract it
    result = data.substr(sizeof(uint32), len);
    // Remove it
    data = data.substr(sizeof(uint32) + len);

    return result;
}

std::string Frame::parse(std::stringstream& data) {
    std::string so_far = data.str();
    std::string result = parse(so_far);

    if (!result.empty()) {
        // Read off the right amount of data from the stringstream
        data.ignore( sizeof(uint32) + result.size() );
    }

    return result;
}

} // namespace Network
} // namespace Frame
