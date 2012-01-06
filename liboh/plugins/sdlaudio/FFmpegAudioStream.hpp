// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SDLAUDIO_FFMPEG_AUDIO_STREAM_HPP_
#define _SIRIKATA_SDLAUDIO_FFMPEG_AUDIO_STREAM_HPP_

#include <sirikata/proxyobject/Platform.hpp>

extern "C" {
    struct AVCodecContext;
    struct AVCodec;
    struct AVPacket;
    struct AVAudioConvert;

    struct SDL_AudioCVT;
}

namespace Sirikata {
namespace SDL {

class FFmpegStream;
typedef std::tr1::shared_ptr<FFmpegStream> FFmpegStreamPtr;

class FFmpegAudioStream;
typedef std::tr1::shared_ptr<FFmpegAudioStream> FFmpegAudioStreamPtr;

/** Represents a single audio stream within a media file. */
class FFmpegAudioStream {
public:
    ~FFmpegAudioStream();

    // Returns true if this stream.
    bool finished() const;

    /** Get the next samples from the stream, or fill them with silence if the
     *  stream has ended.  If the requested number of channels doesn't match,
     *  the conversion will be performed automatically.
     *
     *  \param samples_out output buffer for decoded samples. Always uses 16-bit
     *         signed values
     */
    void samples(int16* samples_out);

private:
    friend class FFmpegStream;

    FFmpegAudioStream(FFmpegStreamPtr parent, uint32 stream_idx, uint8 target_nchannels);

    AVPacket* getNextPacket();
    void decodeSome();
    void convertFormat(int decoded_size);

    FFmpegStreamPtr mParent;
    AVCodecContext* mCodecCtx;
    AVCodec* mCodec;
    // Index of the stream in the parent stream
    uint32 mStreamIndex;

    bool mFinished;

    // Data we're still working on decoding and our progress through it
    AVPacket* mCurrentPacket;
    int mCurrentPacketOffset;

    // Decoded audio data, directly from ffmpeg
    uint8* mDecodedData;

    // Decoded audio data, converted to target format
    SDL_AudioCVT* mConverter;
    uint8 mTargetChannels;
    uint16* mConvertedData;
    // Note that unlike some other places, these are in terms of
    // uint16 (individual samples, not bytes or
    // samples*channels). This matches the storage (mConvertedData's
    // type) so we can index using the offset.
    int mConvertedOffset;
    int mConvertedSize;
};


} // namespace SDL
} // namespace Sirikata

#endif //_SIRIKATA_SDLAUDIO_FFMPEG_AUDIO_STREAM_HPP_
