// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "FFmpegAudioStream.hpp"
#include "FFmpegStream.hpp"
#include "FFmpegGlue.hpp"

// FFmpeg doesn't check for C++ in their headers, so we have to wrap all FFmpeg
// includes in extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <sirikata/sdl/SDL.hpp>
#include "SDL_audio.h"

#define AUDIO_LOG(lvl, msg) SILOG(sdl-audio, lvl, msg)

#define DECODE_BUFFER_SIZE ((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2)


namespace Sirikata {
namespace SDL {

FFmpegAudioStream::FFmpegAudioStream(FFmpegStreamPtr parent, uint32 stream_idx, uint8 target_nchannels)
: mParent(parent),
  mCodecCtx(NULL),
  mCodec(NULL),
  mStreamIndex(stream_idx),
  mFinished(false),
  mCurrentPacket(NULL),
  mCurrentPacketOffset(0),
  mDecodedData( new uint8[DECODE_BUFFER_SIZE] ),
  mConverter(NULL),
  mTargetChannels(target_nchannels),
  mConvertedData(NULL),
  mConvertedOffset(0),
  mConvertedSize(0)
{
    assert(mParent);
    assert(
        mParent->mFormatCtx->nb_streams > mStreamIndex &&
        mParent->mFormatCtx->streams[mStreamIndex]->codec->codec_type == AVMEDIA_TYPE_AUDIO
    );
    // Get a pointer to the codec context for the audio stream
    mCodecCtx = mParent->mFormatCtx->streams[mStreamIndex]->codec;

    // Find the decoder for the audio stream
    mCodec = avcodec_find_decoder(mCodecCtx->codec_id);
    if(mCodec == NULL) {
        AUDIO_LOG(error, "Unsupported codec for " << mParent->mData->name());
        return;
    }
    // Open codec
    if(avcodec_open(mCodecCtx, mCodec) < 0) {
        AUDIO_LOG(error, "Couldn't open codec for " << mParent->mData->name());
        return;
    }
}

FFmpegAudioStream::~FFmpegAudioStream() {
    if (mConverter != NULL) {
        delete mConverter;
        mConverter = NULL;
    }

    // Close the codec
    if (mCodecCtx != NULL) {
        avcodec_close(mCodecCtx);
        mCodecCtx = NULL;
    }

    delete[] mDecodedData;
    delete[] mConvertedData;
}

bool FFmpegAudioStream::finished() const {
    return mFinished;
}

AVPacket* FFmpegAudioStream::getNextPacket() {
    // FIXME reading frames in the audio stream instead of in the parent means
    // we can only decode one stream from a parent stream at a time
    AVPacket packet;
    while(true) {
        int32 res = av_read_frame(mParent->mFormatCtx, &packet);
        if (res < 0) {
            mFinished = true;
            return NULL;
        }

        if (packet.stream_index == mStreamIndex) {
            // This seems to be used to ensure the packet data outlives the next
            // call to av_read_frame
            av_dup_packet(&packet);
            AVPacket* result = new AVPacket(packet);
            return result;
        } else {
            av_free_packet(&packet);
        }
    }

    return NULL;
}

void FFmpegAudioStream::decodeSome() {
    // Grab a packet if we need one
    if (mCurrentPacket == NULL) {
        mCurrentPacket = getNextPacket();
        if (mCurrentPacket == NULL)
            return;
        mCurrentPacketOffset = 0;
    }

    // Setup fake copy of packet with used portions removed.
    AVPacket pkt_temp = *mCurrentPacket;
    pkt_temp.data = mCurrentPacket->data + mCurrentPacketOffset;
    pkt_temp.size = mCurrentPacket->size - mCurrentPacketOffset;

    int data_size = DECODE_BUFFER_SIZE;
    int32 used_bytes = avcodec_decode_audio3(
        mCodecCtx, (int16*)mDecodedData, &data_size, &pkt_temp
    );

    mCurrentPacketOffset += used_bytes;

    if (data_size <= 0) {
        // Got no data out. With these settings, we effectively ignore the frame
        // and move on
        mConvertedOffset = 0;
        mConvertedSize = 0;
    }
    else {
        // Perform conversion
        convertFormat(data_size);
    }

    // Check if we have finished with this packet, clearing it we have
    if (
        used_bytes < 0 || // Decode failure
        mCurrentPacketOffset >= mCurrentPacket->size  // Finished packet
    ) {
        if (mCurrentPacket->data)
            av_free_packet(mCurrentPacket);
        delete mCurrentPacket;
        mCurrentPacket = NULL;
    }

    return;
}

namespace {
SDL_AudioFormat FFmpegFormatToSDLFormat(AVSampleFormat fmt) {
    switch(fmt) {
      case AV_SAMPLE_FMT_NONE:
      case AV_SAMPLE_FMT_U8: return AUDIO_U8; break;
      case AV_SAMPLE_FMT_S16: return AUDIO_S16; break;
      case AV_SAMPLE_FMT_S32: return AUDIO_S32; break;
      case AV_SAMPLE_FMT_FLT: return AUDIO_F32; break;
      case AV_SAMPLE_FMT_DBL: assert(false); break;
        //case AV_SAMPLE_FMT_U8P: return AUDIO_U8;
        //case AV_SAMPLE_FMT_S16P: return AUDIO_S16;
        //case AV_SAMPLE_FMT_S32P: return AUDIO_S32;
        //case AV_SAMPLE_FMT_FLTP: return AUDIO_F32;
        //case AV_SAMPLE_FMT_DBLP: assert(false): break;
      default: return 0xFF; break;
    }
}
}
void FFmpegAudioStream::convertFormat(int decoded_size) {
    // Simple case where it's already in the right format
    if (mCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16 &&
        mCodecCtx->channels == 2) {

        if (mConvertedData == NULL)
            mConvertedData = new uint16[DECODE_BUFFER_SIZE];

        // Copy data into the conversion buffer
        memcpy(mConvertedData, mDecodedData, decoded_size);

        return;
    }

    // Otherwise, requires conversion
    if (mConverter == NULL) {
        mConverter = new SDL_AudioCVT;
        int ret = SDL_BuildAudioCVT(
            mConverter,
            FFmpegFormatToSDLFormat(mCodecCtx->sample_fmt),
            mCodecCtx->channels,
            mCodecCtx->sample_rate,
            AUDIO_S16,
            mTargetChannels,
            44100
        );

        if (ret < 0) {
            AUDIO_LOG(error, "Can't convert audio formats");
            mConvertedOffset = 0;
            mConvertedSize = 0;
            return;
        }

        // Allocate the buffer for performing the conversion, making sure it's
        // long enough to handle the converted data
        mConvertedData = new uint16[DECODE_BUFFER_SIZE * mConverter->len_mult];
    }

    // Copy data into the conversion buffer
    memcpy(mConvertedData, mDecodedData, decoded_size);

    // Perform the conversion
    mConverter->buf = (uint8*)mConvertedData;
    mConverter->len = decoded_size;
    SDL_ConvertAudio(mConverter);

    mConvertedOffset = 0;
    // Note division by size of sample to go from bytes -> samples
    mConvertedSize = decoded_size * mConverter->len_mult / sizeof(uint16);
}

void FFmpegAudioStream::samples(int16* samples_out) {
    // Decode more if we need to
    while (!finished() && mConvertedOffset >= mConvertedSize)
        decodeSome();

    // Silence if we finished the decode
    if (finished()) {
        for(uint8 ch = 0; ch < mTargetChannels; ch++)
            samples_out[ch] = 0;
        return;
    }

    // Standard copy from decoded data
    for(uint8 ch = 0; ch < mTargetChannels; ch++)
        samples_out[ch] = mConvertedData[mConvertedOffset + ch];
    mConvertedOffset += mTargetChannels;
}

} // namespace SDL
} // namespace Sirikata
