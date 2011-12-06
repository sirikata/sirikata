// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Platform.hpp>
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
// Hack to disable OpenCollada's stdint.h for windows. This one should be
// using the one added for ffmpeg.
#define _ZZIP__STDINT_H 1
#endif

#include "FFmpegMemoryProtocol.hpp"

// FFmpeg doesn't check for C++ in their headers, so we have to wrap all FFmpeg
// includes in extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace Sirikata {
namespace SDL {

String FFmpegMemoryProtocol::name() const {
    return mName;
}

size_t FFmpegMemoryProtocol::read(size_t size, uint8* data) {
    if (mPosition >= (int64)mData->size())
        return AVERROR_EOF;

    int64 to_end = mData->size() - mPosition;
    size_t nread = std::min((int64)size, to_end);
    memcpy(data, mData->begin() + mPosition, nread);
    mPosition += nread;

    return nread;
}

bool FFmpegMemoryProtocol::getPosition(int64* position_out) {
    if (mPosition >= 0 && mPosition < (int64)mData->size()) {
        *position_out = mPosition;
        return true;
    }
    return false;
}

bool FFmpegMemoryProtocol::setPosition(int64 position) {
    if (position >= 0 && position < (int64)mData->size()) {
        mPosition = position;
        return true;
    }
    return false;
}

bool FFmpegMemoryProtocol::getSize(int64* size_out) {
    *size_out = mData->size();
    return true;
}

bool FFmpegMemoryProtocol::isStreaming() {
    return false;
}

} // namespace SDL
} // namespace Sirikata
