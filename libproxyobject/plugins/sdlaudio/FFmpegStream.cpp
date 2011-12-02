// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "FFmpegStream.hpp"
#include "FFmpegGlue.hpp"
#include "FFmpegAudioStream.hpp"

// FFmpeg doesn't check for C++ in their headers, so we have to wrap all FFmpeg
// includes in extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#define AUDIO_LOG(lvl, msg) SILOG(sdl-audio, lvl, msg)

namespace Sirikata {
namespace SDL {

FFmpegStream::FFmpegStream(FFmpegURLProtocol* raw)
 : mData(raw),
   mFormatCtx(NULL)
{
    String key = FFmpegGlue::getSingleton().addProtocol(mData);

    // Open video file
    if(av_open_input_file(&mFormatCtx, key.c_str(), NULL, 0, NULL) != 0) {
        AUDIO_LOG(error, "Failed to open " << key << " for " << mData->name());
        return;
    }

    // Retrieve stream information
    if(av_find_stream_info(mFormatCtx) < 0) {
        AUDIO_LOG(error, "Couldn't find stream information for " << mData->name());
        return;
    }

    // Dump information about file onto standard error
    av_dump_format(mFormatCtx, 0, key.c_str(), 0);
}

uint32 FFmpegStream::numAudioStreams() {
    uint32 result = 0;
    for(int i = 0; i < mFormatCtx->nb_streams; i++) {
        if (mFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            result++;
    }
    return result;
}

FFmpegAudioStreamPtr FFmpegStream::getAudioStream(uint32 idx, uint8 nchannels) {
    int nAudioStreamsSeen = -1;
    int audioStream = -1;
    for(int i = 0; i < mFormatCtx->nb_streams; i++) {
        if(mFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            nAudioStreamsSeen++;
            if (nAudioStreamsSeen == idx) {
                audioStream = i;
                break;
            }
        }
    }
    if(audioStream == -1)
        return FFmpegAudioStreamPtr();

    return FFmpegAudioStreamPtr(new FFmpegAudioStream(getSharedPtr(), audioStream, nchannels));
}

FFmpegStream::~FFmpegStream() {
    // Close the video file
    av_close_input_file(mFormatCtx);

    FFmpegGlue::getSingleton().removeProtocol(mData);
    delete mData;
}

} // namespace SDL
} // namespace Sirikata
