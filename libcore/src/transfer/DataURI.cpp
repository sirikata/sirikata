// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/transfer/DataURI.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <iterator>

namespace Sirikata {
namespace Transfer {

DataURI::DataURI(const String& uri)
{
    parse(uri);
}

DataURI::DataURI(const URI& uri)
{
    parse(uri.toString());
}

void DataURI::parse(const String& uri) {
    // Find the first colon and make sure we have a data URI
    String::size_type colon_pos = uri.find(':');
    if (colon_pos == String::npos)
        return;
    String scheme = uri.substr(0, colon_pos);
    if (scheme != "data")
        return;

    String scheme_specific = uri.substr(colon_pos+1);

    // Strip out ';base64' if its in there
    static String opt_base64 = ";base64";
    String::size_type b64_pos = scheme_specific.find(opt_base64);
    if (b64_pos != String::npos) {
        scheme_specific =
            scheme_specific.substr(0, b64_pos) +
            scheme_specific.substr(b64_pos + opt_base64.size());
    } else {
        // FIXME we currently only support base64 encoding. By spec,
        // you can also omit this and use URL encoding for the data.
        return;
    }

    // Split data and media type
    String::size_type split_data_pos = scheme_specific.find(',');
    if (split_data_pos == String::npos)
        return;
    String mediatype_part = scheme_specific.substr(0, split_data_pos);
    String data_part = scheme_specific.substr(split_data_pos + 1);

    // Split up the mediatype info
    bool first_media_subpart = true;
    while(mediatype_part.size() > 0) {
        // Find the next split ;
        String::size_type split_media = mediatype_part.find(';');
        String next_val;
        if (split_media == String::npos) {
            next_val = mediatype_part;
            mediatype_part = "";
        }
        else {
            next_val = mediatype_part.substr(split_media);
            mediatype_part = mediatype_part.substr(split_media+1);
        }
        // Mimetypes must have x/y form and parameters must have = in
        // them.
        String::size_type equals_idx = next_val.find('=');
        // First part might be a mimetype
        if (first_media_subpart) {
            first_media_subpart = false;
            if (equals_idx == String::npos) {
                if (next_val.find('/') == String::npos) {
                    // Bad mimetype == bad data URI.
                    mMediaType = "";
                    mMediaTypeParameters.clear();
                    return;
                }
                mMediaType = next_val;
                continue;
            }
        }
        else {
            // In all later parts, or if we skip over the mediatype, this
            // should be a parameter.
            if (equals_idx == String::npos) {
                // Failure case -- can't parse
                mMediaType = "";
                mMediaTypeParameters.clear();
                return;
            }
            mMediaTypeParameters[ next_val.substr(0,equals_idx) ] = next_val.substr(equals_idx+1);
        }
    }

    mData = decodeBase64URL(data_part);
}

String DataURI::toString() const {
    String res = String("data:");
    if (mMediaType.size() > 0) res += mMediaType;
    for(ParameterMap::const_iterator it = mMediaTypeParameters.begin(); it != mMediaTypeParameters.end(); it++)
        res += String(";") + it->first + "=" + it->second;
    res += String(",") + encodeBase64URL(mData);
    return res;
}

String DataURI::encodeBase64(const String& orig) {
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

String DataURI::encodeBase64URL(const String& orig) {
    // To properly encode, we need to buffer the end of the string so that the
    // number of bits works out.  Append = until we get a total number of bits
    // divisible by 8.
    String data_part = encodeBase64(orig);
    // We get back encoded_bytes * 6 bits.
    uint32 nbits = data_part.size() * 6;
    // We need it to be divisible by 8.
    uint32 extra_bits = nbits % 8;
    while(extra_bits % 8 != 0) {
        data_part += '=';
        extra_bits += 6;
    }
    return data_part;
}

String DataURI::decodeBase64(const String& orig) {
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

String DataURI::decodeBase64URL(const String& orig) {
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

    String res = decodeBase64(data_part);
    if (extra) res = res.substr(0, res.size()-extra);
    return res;
}

} // namespace Transfer
} // namespace Sirikata
