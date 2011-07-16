// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/Base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <iterator>

namespace Sirikata {
namespace Base64 {

String encode(const String& orig) {
    using namespace boost::archive::iterators;
    typedef base64_from_binary< transform_width< const char *, 6, 8 > > base64_text;

    std::stringstream os;
    std::copy(
        base64_text(orig.c_str()),
        base64_text(orig.c_str() + orig.size()),
        std::ostream_iterator<char>(os)
    );
    return os.str();
}

String encodeURL(const String& orig) {
    // To properly encode, we need to buffer the end of the string so that the
    // number of bits works out.  Append = until we get a total number of bits
    // divisible by 8.
    String data_part = encode(orig);
    // We get back encoded_bytes * 6 bits.
    uint32 nbits = data_part.size() * 6;
    // We need it to be divisible by 8.
    uint32 extra_bits = nbits % 8;
    while(extra_bits % 8 != 0) {
        data_part += "%3D";
        extra_bits += 6;
    }
    return data_part;
}

String decode(const String& orig) {
    using namespace boost::archive::iterators;
    typedef transform_width< binary_from_base64< const char * >, 8, 6 > base64_text;

    std::stringstream os;
    std::copy(
        base64_text(orig.c_str()),
        base64_text(orig.c_str() + orig.size()),
        std::ostream_iterator<char>(os)
    );
    return os.str();
}

String decodeURL(const String& orig) {
    // Decode to non-URL encoded form by dropping %3D (i.e. =). The decoder
    // needs the padding, however, so we replace with a valid char and filter
    // out later.
    String data_part = orig;
    uint32 extra = 0;
    while(true) {
        String::size_type equals_pos = data_part.find("%3D");
        if (equals_pos == String::npos) break;
        data_part.replace(equals_pos, 3, String("a"));
        extra++;
    }

    String res = decode(data_part);
    if (extra) res = res.substr(0, res.size()-extra);
    return res;
}

} // namespace Base64
} // Sirikata
