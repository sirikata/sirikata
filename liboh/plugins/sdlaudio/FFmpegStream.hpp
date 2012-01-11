// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SDLAUDIO_FFMPEG_STREAM_HPP_
#define _SIRIKATA_SDLAUDIO_FFMPEG_STREAM_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/util/SelfWeakPtr.hpp>

extern "C" {
    struct AVFormatContext;
}

namespace Sirikata {
namespace SDL {

class FFmpegURLProtocol;

class FFmpegStream;
typedef std::tr1::shared_ptr<FFmpegStream> FFmpegStreamPtr;

class FFmpegAudioStream;
typedef std::tr1::shared_ptr<FFmpegAudioStream> FFmpegAudioStreamPtr;

/** Represents a media stream in FFmpeg. Mostly acts as a container for
 *  FFmpegAudioStreams, decoding the initial FFmpegURLProtocol to get at the
 *  streams within it.
 */
class FFmpegStream : public SelfWeakPtr<FFmpegStream> {
public:
    /** Construct a stream using the given data. Ownership of the data is passed
     *  to this stream.
     */
    FFmpegStream(FFmpegURLProtocol* raw);
    ~FFmpegStream();

    uint32 numAudioStreams();
    FFmpegAudioStreamPtr getAudioStream(uint32 idx, uint8 nchannels);

    // Reloads the stream to start parsing from the beginning.
    void reload();
private:

    // Helpers for cleanup/init used by both constructor/destructor and reload().
    void initDecode();
    void cleanupDecode();

    friend class FFmpegAudioStream;

    FFmpegURLProtocol* mData;
    AVFormatContext* mFormatCtx;
};


} // namespace SDL
} // namespace Sirikata

#endif //_SIRIKATA_SDLAUDIO_FFMPEG_STREAM_HPP_
