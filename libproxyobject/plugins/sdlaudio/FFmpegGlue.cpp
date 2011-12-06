// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Platform.hpp>
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
// Hack to disable OpenCollada's stdint.h for windows. This one should be
// using the one added for ffmpeg.
#define _ZZIP__STDINT_H 1
#endif

#include "FFmpegGlue.hpp"

// FFmpeg doesn't check for C++ in their headers, so we have to wrap all FFmpeg
// includes in extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

#include <boost/lexical_cast.hpp>

#define AUDIO_LOG(lvl, msg) SILOG(sdl-audio, lvl, msg)

AUTO_SINGLETON_INSTANCE(Sirikata::SDL::FFmpegGlue);

namespace Sirikata {
namespace SDL {


// URL handler callabacks and structs registered with libavformat.
// This borrows heavily from Chromium's code:
//
//  Copyright (c) 2011 The Chromium Authors. All rights reserved.
//  Use of this source code is governed by a BSD-style license that can be
//  found in the LICENSE file.
//
// See chromium's src/media/filters/ffmpeg_glue*.
namespace {

FFmpegURLProtocol* ToProtocol(void* data) {
    return reinterpret_cast<FFmpegURLProtocol*>(data);
}

// FFmpeg protocol interface.
int openContext(URLContext* h, const char* filename, int flags) {
    FFmpegURLProtocol* protocol;
    FFmpegGlue::getSingleton().getProtocol(filename, &protocol);
    if (!protocol)
        return AVERROR(EIO);

    h->priv_data = protocol;
    h->flags = URL_RDONLY;
    h->is_streamed = protocol->isStreaming();
    return 0;
}

int readContext(URLContext* h, unsigned char* buf, int size) {
    FFmpegURLProtocol* protocol = ToProtocol(h->priv_data);
    int result = protocol->read(size, buf);
    if (result < 0)
        result = AVERROR(EIO);
    return result;
}

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 68, 0)
int writeContext(URLContext* h, const unsigned char* buf, int size) {
#else
int writeContext(URLContext* h, unsigned char* buf, int size) {
#endif
    // We don't support writing.
    return AVERROR(EIO);
}

int64 seekContext(URLContext* h, int64 offset, int whence) {
    FFmpegURLProtocol* protocol = ToProtocol(h->priv_data);
    int64 new_offset = AVERROR(EIO);
    switch (whence) {
      case SEEK_SET:
        if (protocol->setPosition(offset))
            protocol->getPosition(&new_offset);
        break;

      case SEEK_CUR:
        int64 pos;
        if (!protocol->getPosition(&pos))
            break;
        if (protocol->setPosition(pos + offset))
            protocol->getPosition(&new_offset);
        break;

      case SEEK_END:
        int64 size;
        if (!protocol->getSize(&size))
            break;
        if (protocol->setPosition(size + offset))
            protocol->getPosition(&new_offset);
        break;

      case AVSEEK_SIZE:
        protocol->getSize(&new_offset);
        break;

      default:
        assert(false);
    }
    if (new_offset < 0)
        new_offset = AVERROR(EIO);
    return new_offset;
}

int closeContext(URLContext* h) {
    h->priv_data = NULL;
    return 0;
}

// Protocol name, which should
const char gFFmpegURLProtocol[] = "siriffmpeg";

// Fill out our FFmpeg protocol definition.
URLProtocol kFFmpegURLProtocol = {
    gFFmpegURLProtocol,
    &openContext,
    &readContext,
    &writeContext,
    &seekContext,
    &closeContext,
};

} // namespace

FFmpegGlue::FFmpegGlue()
 : mRefCount(0)
{
  // Register our protocol glue code with FFmpeg.
  avcodec_init();
  av_register_protocol2(&kFFmpegURLProtocol, sizeof(kFFmpegURLProtocol));
  //av_lockmgr_register(&LockManagerOperation);

  // Now register the rest of FFmpeg.
  av_register_all();
}

FFmpegGlue::~FFmpegGlue() {
}

void FFmpegGlue::ref() {
    mRefCount++;
}

void FFmpegGlue::unref() {
    mRefCount--;
    if (mRefCount == 0) FFmpegGlue::destroy();
}

String FFmpegGlue::addProtocol(FFmpegURLProtocol* protocol) {
    Lock lck(mMutex);
    String key = getProtocolKey(protocol);
    if (mProtocols.find(key) == mProtocols.end()) {
        mProtocols[key] = protocol;
    }
    return key;
}

void FFmpegGlue::removeProtocol(FFmpegURLProtocol* protocol) {
    Lock lck(mMutex);
    for (ProtocolMap::iterator cur, iter = mProtocols.begin();
         iter != mProtocols.end();) {
        cur = iter;
        iter++;

        if (cur->second == protocol)
            mProtocols.erase(cur);
    }
}

void FFmpegGlue::getProtocol(const String& key,
    FFmpegURLProtocol** protocol) {
    Lock lck(mMutex);
    ProtocolMap::iterator iter = mProtocols.find(key);
    if (iter == mProtocols.end()) {
        *protocol = NULL;
        return;
    }
    *protocol = iter->second;
}

String FFmpegGlue::getProtocolKey(FFmpegURLProtocol* protocol) {
    // Use the FFmpegURLProtocol's memory address to generate the unique string.
    // This also has the nice property that adding the same FFmpegURLProtocol
    // reference will not generate duplicate entries.
    return String(gFFmpegURLProtocol) + "://" + boost::lexical_cast<String>(protocol);
}


} // namespace SDL
} // namespace Sirikata
