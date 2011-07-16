// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/transfer/DataURI.hpp>
#include <sirikata/core/util/Base64.hpp>

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

    mData = Base64::decodeURL(data_part);
}

String DataURI::toString() const {
    String res = String("data:");
    if (mMediaType.size() > 0) res += mMediaType;
    for(ParameterMap::const_iterator it = mMediaTypeParameters.begin(); it != mMediaTypeParameters.end(); it++)
        res += String(";") + it->first + "=" + it->second;
    res += String(",") + Base64::encodeURL(mData);
    return res;
}

} // namespace Transfer
} // namespace Sirikata
