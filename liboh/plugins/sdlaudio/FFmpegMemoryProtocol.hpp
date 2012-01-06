// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SDLAUDIO_FFMPEG_MEMORY_PROTOCOL_HPP_
#define _SIRIKATA_SDLAUDIO_FFMPEG_MEMORY_PROTOCOL_HPP_

#include "FFmpegGlue.hpp"
#include <sirikata/core/transfer/TransferData.hpp>

namespace Sirikata {
namespace SDL {

/** Implementation of FFmpegURLProtocol which reads from in-memory data. */
class FFmpegMemoryProtocol : public FFmpegURLProtocol {
public:
    FFmpegMemoryProtocol(String name, Transfer::DenseDataPtr data)
     : mName(name),
       mData(data),
       mPosition(0)
    {}
    virtual ~FFmpegMemoryProtocol() {}


    virtual String name() const;
    virtual size_t read(size_t size, uint8* data);
    virtual bool getPosition(int64* position_out);
    virtual bool setPosition(int64 position);
    virtual bool getSize(int64* size_out);
    virtual bool isStreaming();

private:
    FFmpegMemoryProtocol(const FFmpegMemoryProtocol&);
    FFmpegMemoryProtocol& operator=(const FFmpegMemoryProtocol&);

    String mName;
    Transfer::DenseDataPtr mData;
    int64 mPosition;
};


} // namespace SDL
} // namespace Sirikata

#endif //_SIRIKATA_SDLAUDIO_FFMPEG_MEMORY_PROTOCOL_HPP_
