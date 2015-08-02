/*  Sirikata Jpeg Texture Transfer -- Texture Transfer management system
 *  SwitchableCompression.hpp
 *
 *  Copyright (c) 2015, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _SIRIKATA_SWITCHABLE_COMPRESSION_HH_
#define _SIRIKATA_SWITCHABLE_COMPRESSION_HH_
#include <assert.h>
#include "Compression.hpp"

namespace Sirikata {

class SIRIKATA_EXPORT SwitchableXZBase : public DecoderDecompressionReader {
  public:
    SwitchableXZBase(DecoderReader *r,
                     const JpegAllocator<uint8_t> &alloc) : DecoderDecompressionReader(r, false, alloc) {
    }
    const unsigned char * bufferNextIn() {
        return mStream.next_in;
    }
    unsigned int bufferAvailIn() {
        return mStream.avail_in;
    }
    JpegError bufferFill() {
        if (mStream.avail_in == 0) {
            std::pair<uint32, JpegError> result = mBase->Read(mReadBuffer,
                                                              sizeof(mReadBuffer));
            mStream.next_in = mReadBuffer;
            mStream.avail_in = result.first;
            return result.second;
        } else {
            return JpegError::nil();
        }
    }
    void switchToUncompressed() {
        memmove(mReadBuffer, mStream.next_in, mStream.avail_in);
        mStream.next_in = mReadBuffer;
        while (mStream.avail_in < sizeof(mReadBuffer)) {
            // need to guarantee have a full buffer at this time
            std::pair<uint32, JpegError> read = mBase->Read(mReadBuffer + mStream.avail_in,
                                                            sizeof(mReadBuffer) - mStream.avail_in);
            mStream.avail_in += read.first;
            if (read.first == 0 || read.second != JpegError::nil()) {
                break;
            }
        }
        if (!mStreamEndEncountered) {
            unsigned char nop[1];
            std::pair<uint32, JpegError> fake_read = Read(nop, 1);
            assert(fake_read.first == 0);
        }
        Close();
    }
    void bufferConsume(unsigned int amount) {
        assert(amount <= mStream.avail_in && "We are trying to consume more than avail");
        mStream.avail_in -= amount;
        mStream.next_in += amount;
    }
};

class SIRIKATA_EXPORT SwitchableLZHAMBase : public LZHAMDecompressionReader {
    SwitchableLZHAMBase(DecoderReader *r, const JpegAllocator<uint8_t> &alloc)
        : LZHAMDecompressionReader(r, alloc) {}
    unsigned char * bufferNextIn() {
        return mReadOffset;
    }
    unsigned int bufferAvailIn() {
        return mAvailIn;
    }
    JpegError bufferFill() {
        if (mAvailIn == 0) {
            std::pair<uint32, JpegError> result = mBase->Read(mReadBuffer,
                                                              sizeof(mReadBuffer));
            mReadOffset = mReadBuffer;
            mAvailIn = result.first;
            return result.second;
        } else {
            return JpegError::nil();
        }
    }
    void switchToUncompressed() {
        //noop
    }
    void bufferConsume(unsigned int amount) {
        assert(amount <= mAvailIn && "We are trying to consume more than avail");
        mAvailIn -= amount;
        mReadOffset += amount;
    }
};

class SIRIKATA_EXPORT UncloseableWriterWrapper : public Sirikata::DecoderWriter {
    DecoderWriter *mBase;
    bool mBaseClosed;
  public:
    UncloseableWriterWrapper(DecoderWriter *base) {
        mBase = base;
        mBaseClosed = false;
    }
    std::pair<unsigned int, JpegError> Write(const Sirikata::uint8*data, unsigned int size) {
        if (mBaseClosed) {
            return std::pair<unsigned int, JpegError>(size, JpegError::nil());
        }
        return mBase->Write(data, size);
    }
    void Close() {
        mBaseClosed = true;
    }
    void ResetBase() {
        mBaseClosed = false;
    }
    void UnignoredClose() {
        mBase->Close();
    }
    DecoderWriter* writer() {
        return mBase;
    }
};

template<class VarDecompressionWriter> class SwitchableCompressionWriter : public DecoderWriter {
    UncloseableWriterWrapper mBase;
    std::auto_ptr<VarDecompressionWriter> mCompressBase;
    JpegAllocator<uint8_t> mAllocator;
    uint8_t mLevel;
public:
    SwitchableCompressionWriter(DecoderWriter *base, uint8_t compression_level, const JpegAllocator<uint8_t> &alloc) :
        mBase(base), mAllocator(alloc) {
        mLevel = compression_level;
    }
    void EnableCompression() {
        mBase.ResetBase();
        if (!mCompressBase.get()) {
            mCompressBase.reset(new VarDecompressionWriter(&mBase, mLevel, mAllocator));
        }
    }
    void DisableCompression() {
        mCompressBase->Close();
        mCompressBase.reset();
    }
    DecoderWriter *getBase() {
        return mBase.writer();
    }
    std::pair<unsigned int, JpegError> Write(const Sirikata::uint8*data, unsigned int size) {
        if (mCompressBase.get()) {
            return mCompressBase->Write(data, size);
        } else {
            return mBase.writer()->Write(data, size);
        }
    }
    void Close() {
        if (mCompressBase.get()) {
            mCompressBase->Close();
        }
        mBase.Close();
        mBase.UnignoredClose();
    }
};
template<class VarDecompressionReader> class SwitchableDecompressionReader : public DecoderReader {
    VarDecompressionReader mCompressBase;
    bool decompressing;
public:
    SwitchableDecompressionReader(DecoderReader *base, const JpegAllocator<uint8_t> &alloc) :
        mCompressBase(base, alloc) {
        decompressing = false;
    }
    void EnableCompression() {
        decompressing = true;
        
    }
    void DisableCompression() {
        if (decompressing) {
            mCompressBase.switchToUncompressed();
        }
        decompressing = false;
    }
    std::pair<unsigned int, JpegError> Read(Sirikata::uint8*data, unsigned int size) {
        if (decompressing) {
            std::pair<unsigned int, JpegError> retval = mCompressBase.Read(data, size);
            return retval;
        }
        unsigned int read_so_far = 0;
        while (read_so_far < size) {
            if (!mCompressBase.bufferAvailIn()) {
                JpegError err = mCompressBase.bufferFill();
                if (err != JpegError::nil()) {
                    return std::pair<unsigned int, JpegError>(read_so_far, err);
                }
            }
            unsigned int amount_to_read = std::min(size - read_so_far, mCompressBase.bufferAvailIn());
            memcpy(data + read_so_far, mCompressBase.bufferNextIn(), amount_to_read);
            mCompressBase.bufferConsume(amount_to_read);
            read_so_far += amount_to_read;
        }
        return std::pair<unsigned int, JpegError>(read_so_far, JpegError::nil());
    }
};
}
#endif
