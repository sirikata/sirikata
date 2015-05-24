/*  Sirikata Jpeg Texture Transfer Test Suite -- Texture Transfer management system
 *  main.cpp
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

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package jpeg implements a JPEG image decoder and encoder.
//
// JPEG is defined in ITU-T T.81: http://www.w3.org/Graphics/JPEG/itu-t81.pdf.
//
#include <stdio.h>
#include <sirikata/core/jpeg-arhc/Compression.hpp>
namespace Sirikata {

template<class T, class Al> void appendByte(std::vector<T, Al>&buffer, T item) {
    buffer.push_back(item);
}
template<class T, class Al> void append(std::vector<T, Al>&buffer, const std::vector<T, Al> &items) {
    buffer.insert(buffer.end(), items.begin(), items.end());
}
template<class T, class Al> void append(std::vector<T, Al>&buffer,
                                        typename std::vector<T, Al>::const_iterator begin,
                                        typename std::vector<T, Al>::const_iterator end) {
    buffer.insert(buffer.end(), begin, end);
}
template<class T, class Al> void appendData(std::vector<T, Al>&buffer,
                                            const T* data,
                                            size_t size) {
    buffer.insert(buffer.end(), data,  data + size);
}
BitByteStream::BitByteStream(const JpegAllocator<uint8>& alloc) : buffer(alloc) {
    bits = 0;
    nBits = 0;
    bitReadCursor = 0;
}

void BitByteStream::clear() {
    buffer.clear();
}

void BitByteStream::flushToWriter(DecoderWriter &w) {
    if (!buffer.empty()) {
        w.Write(&buffer[0], buffer.size());
    }
}


void BitByteStream::pop() {
    BitByteStream &b = *this;
    if (b.nBits > 0 && b.nBits < 8) {
        if (b.buffer.empty()) {
            assert(false && "popping from empty buffer");
            return;
        }
        uint32 poppedByte = 0;
        poppedByte = uint32(b.buffer.back());
        b.buffer.pop_back();
        poppedByte <<= b.nBits;
        b.bits |= poppedByte;
        b.nBits += 8;
    }
    if (b.nBits >= 8) {
        b.nBits -= 8;
        b.bits >>= 8;
    } else {
        b.buffer.pop_back();
    }
}
uint32 BitByteStream::len()const {
    return uint32(buffer.size()) + uint32(nBits/8);
}
uint32 BitByteStream::estimatedByteSize()const {
    return uint32(buffer.size()) + uint32(nBits/8) + uint32(nBits % 8 ? 1 : 0);
}

void BitByteStream::flushBits(bool stuffBits) {
    while (nBits > 0) {
        emitBits(1, 1, stuffBits);
    }
}

std::vector<uint8, JpegAllocator<uint8> > streamLenToBE(uint32 streamLen, const JpegAllocator<uint8_t> & allocator) {
    uint8 retval[5] = {uint8(streamLen >> 24), uint8((streamLen >> 16) & 0xff),
                       uint8((streamLen >> 8) & 0xff), uint8(streamLen & 0xff), 0};
    return std::vector<uint8, JpegAllocator<uint8> >(retval, retval + 4, allocator);
}
uint32 bufferBEToStreamLength(uint8 *buf) {
    uint32 vectorLength = 0;
    for (int i = 0; i < 4; i++) { // read in the huffvector length
        vectorLength <<= 8;
        vectorLength |= uint32(buf[i]);
    }
    return vectorLength;
}

uint8 VERSION_INFORMATION[2] = {1, // major version
                                0}; // minor version
void Decoder::flush(DecoderWriter &w) {
    mEstimatedSizeState = ESTIMATED_SIZE_READY;
    Decoder &d = *this;
    if (d.arhc) {
        if (d.wbuffer.nBits > 0) {
            assert(false && "Bits should be flushed by here");
        }
        while (d.wbuffer.len() < d.extOriginalFileSize) {
            fprintf(stderr, "Buffer should not be %ld real: %d\n",
                    d.wbuffer.buffer.size(), int(d.extOriginalFileSize));
            appendByte(d.wbuffer.buffer, uint8(0));
        }
        uint32 addendumLength = uint32(d.extEndFileBuffer.size());
        for (uint32 i = 0; i < addendumLength; i++) {
            if (d.extEndFileBufferCursor != 0) {
                assert(false && "CIRCULAR BUFFER CURSOR IS NOT ZERO\n");
            }
            d.wbuffer.buffer[d.extOriginalFileSize-addendumLength+i] =
                d.extEndFileBuffer[(d.extEndFileBufferCursor+i)%addendumLength];
        }
    } else {
        d.bitbuffer.flushBits(false);
        uint32 bitStreamLen = d.bitbuffer.len();
        std::vector<uint8, JpegAllocator<uint8> > lengthHeader(mAllocator);
        lengthHeader.push_back('a');
        lengthHeader.push_back('r');
        lengthHeader.push_back('h');
        lengthHeader.push_back('c');
        // FIXME: deal with lzma headers
        appendData(lengthHeader, VERSION_INFORMATION, 2);
        appendByte(lengthHeader, d.componentCoalescing);
        std::vector<uint8, JpegAllocator<uint8> > originalFileSizeBE(streamLenToBE(d.extOriginalFileSize, mAllocator));
        std::vector<uint8, JpegAllocator<uint8> > extensionLength(streamLenToBE(uint32(originalFileSizeBE.size() +
                                                                                         d.extEndFileBuffer.size()),
                                                                                mAllocator));
        if (extensionLength[0] == 0) { // only support 24 bit extension length
            appendByte(lengthHeader, (uint8)0x1); // end of buffer extension
            append(lengthHeader, extensionLength.begin() + 1, extensionLength.end());
            append(lengthHeader, originalFileSizeBE);
            append(lengthHeader, d.extEndFileBuffer.begin() + d.extEndFileBufferCursor, d.extEndFileBuffer.end());
            append(lengthHeader, d.extEndFileBuffer.begin(), d.extEndFileBuffer.begin() + d.extEndFileBufferCursor);
        }
        appendByte(lengthHeader, (uint8)0); // end of extensions
        append(lengthHeader, streamLenToBE(bitStreamLen, mAllocator));
        for (size_t index = 0; index < (sizeof(d.huffMultibuffer) / sizeof(d.huffMultibuffer[0])); ++index) {
            d.huffMultibuffer[index].flushBits(false);
            uint32 huffMultiStreamLen = d.huffMultibuffer[index].len();
            append(lengthHeader, streamLenToBE(huffMultiStreamLen, mAllocator));
        }
        //fmt.Printf("Length header = %x %d %x\n", lengthHeader, bitStreamLen, bitStreamLen)
        if (!lengthHeader.empty()) {
            w.Write(&lengthHeader[0], lengthHeader.size());
        }
        d.bitbuffer.flushToWriter(w);
        for (size_t index = 0; index < (sizeof(d.huffMultibuffer) / sizeof(d.huffMultibuffer[0])); ++index) {
            d.huffMultibuffer[index].flushToWriter(w);
        }
    }
    d.wbuffer.flushToWriter(w);
}

// fill fills up the d.bytes.buf buffer from the underlying io.Reader. It
// should only be called when there are no unread bytes in d.bytes.
JpegError Decoder::fill() {
    Decoder &d = *this;
    if (d.bytes.i != d.bytes.j) {
        assert(false && "jpeg: fill called when unread bytes exist");
    }
    // Move the last 2 bytes to the start of the buffer, in case we need
    // to call unreadByteStuffedByte.
    if (d.bytes.j > 2) {
        d.bytes.buf[0] = d.bytes.buf[d.bytes.j-2];
        d.bytes.buf[1] = d.bytes.buf[d.bytes.j-1];
        d.bytes.i = 2;
        d.bytes.j = 2;
    }
    // Fill in the rest of the buffer.
    uint32E nErr = d.r->Read(d.bytes.buf + d.bytes.j, sizeof(d.bytes.buf) - d.bytes.j);
    if (!d.arhc) {
        d.extOriginalFileSize += uint32(nErr.first);
        for (uint32 index = d.bytes.j; index < d.bytes.j + nErr.first; ++index) {
            uint8 byt = d.bytes.buf[index];
            uint32 MAX_EXT_END_FILE_BUFFER = 16;
            if (d.extEndFileBuffer.size() < MAX_EXT_END_FILE_BUFFER || d.extEndEncountered) {
                appendByte(d.extEndFileBuffer, byt);
            } else {
                d.extEndFileBuffer[d.extEndFileBufferCursor] = byt;
                d.extEndFileBufferCursor += 1;
                d.extEndFileBufferCursor %= uint32(d.extEndFileBuffer.size());
            }
        }
    }
    d.bytes.j += nErr.first;
    if (nErr.first > 0) {
        nErr.second = JpegError::nil();
    }
    return nErr.second;
}
Decoder::Bytes::Bytes() {
    memset(this, 0, sizeof(Bytes));
}
// unreadByteStuffedByte undoes the most recent readByteStuffedByte call,
// giving a byte of data back from d.bits to d.bytes. The Huffman look-up table
// requires at least 8 bits for look-up, which means that Huffman decoding can
// sometimes overshoot and read one or two too many bytes. Two-byte overshoot
// can happen when expecting to read a 0xff 0x00 byte-stuffed byte.
void Decoder::unreadByteStuffedByte() {
    Decoder &d = *this;
    if (d.bytes.nUnreadable == 0) {
        assert(false && "jpeg: unreadByteStuffedByte call cannot be fulfilled");
    }
    d.bytes.i -= d.bytes.nUnreadable;
    d.bytes.nUnreadable = 0;
    d.offset--;
    if (d.bits.n >= 8) {
        d.bits.a >>= 8;
        d.bits.n -= 8;
        d.bits.m >>= 8;
    }
}

void Decoder::appendByteToWriteBuffer(uint8 x) {
    if (!huffTransSect) {
        wbuffer.appendByte(x);
    }
}

void Decoder::appendBytesToWriteBuffer(uint8 *x, uint32 length) {
    if (!huffTransSect) {
        wbuffer.appendBytes(x, length);
    }
}

// readByte returns the next byte, whether buffered or not buffered. It does
// not care about byte stuffing.
Decoder::uint8E Decoder::readByte() {
    Decoder *d = this;
    if (d->bytes.i == d->bytes.j) {
        JpegError err = d->fill();
        if (err != JpegError::nil()) {
            return uint8E(0, err);
        }
    }
    if (d->bytes.i == d->bytes.j) {
        return uint8E(0, JpegError::errEOF());
    }
    uint8 x = d->bytes.buf[d->bytes.i];
    d->bytes.i++;
    d->bytes.nUnreadable = 0;
    d->offset++;
    d->appendByteToWriteBuffer(x);
    //fprinttf(stderr, "Read %x\n", x);
    return uint8E(x, JpegError::nil());
}


// readByteStuffedByte is like readByte but is for byte-stuffed Huffman data.
Decoder::uint8E Decoder::readByteStuffedByte() {
    Decoder &d = *this;
    // Take the fast path if d.bytes.buf contains at least two bytes.
    if (d.bytes.i+2 <= d.bytes.j) {
        uint8 x = d.bytes.buf[d.bytes.i];
        d.appendByteToWriteBuffer(x);
        d.bytes.i++;
        d.bytes.nUnreadable = 1;
        if (x != 0xff) {
            return uint8E(x, JpegError::nil());
        }
        if (d.bytes.buf[d.bytes.i] != 0x00) {
            return uint8E(0, JpegError::errMissingFF00());
        }
        uint8 zero = 0;
        d.appendByteToWriteBuffer(zero);
        d.bytes.i++;
        d.bytes.nUnreadable = 2;
        return uint8E(0xff, JpegError::nil());
    }

    uint8E xErr = d.readByte();
    if (xErr.second != JpegError::nil()) {
        return uint8E(0, xErr.second);
    }
    if (xErr.first != 0xff) {
        d.bytes.nUnreadable = 1;
        return uint8E(xErr.first, JpegError::nil());
    }

    xErr = d.readByte();
    if (xErr.second != JpegError::nil()) {
        d.bytes.nUnreadable = 1;
        return uint8E(0, xErr.second);
    }
    d.bytes.nUnreadable = 2;
    if (xErr.first != 0x00) {
        return uint8E(0, JpegError::errMissingFF00());
    }
    return uint8E(0xff, JpegError::nil());
}

// readFull reads exactly len(p) bytes into p. It does not care about byte
// stuffing.
JpegError Decoder::readFull(uint8*p, uint32 pSize) {
    Decoder *d = this;
    d->offset += uint32(pSize);
    // Unread the overshot bytes, if any.
    if (d->bytes.nUnreadable != 0) {
        if (d->bits.n >= 8) {
            d->unreadByteStuffedByte();
        }
        d->bytes.nUnreadable = 0;
    }
    uint8 *pPtr = p;
    uint32 pPtrSize = pSize;
    while (true) {
        uint32 n = std::min(d->bytes.j - d->bytes.i, int(pPtrSize));
        memcpy(pPtr, d->bytes.buf + d->bytes.i, n);
        pPtr += n;
        pPtrSize -= n;
        d->bytes.i += n;
        if (pPtrSize == 0) {
            break;
        }
        JpegError err;
        if ((err = d->fill()) != JpegError()) {
            d->appendBytesToWriteBuffer(p, pSize - pPtrSize);
            return err;
        }
    }
    d->appendBytesToWriteBuffer(p, pSize);
    return JpegError::nil();
}

// ignore ignores the next n bytes.
JpegError Decoder::ignore(uint32 n) {
    Decoder & d = *this;
    if (n > 0) {
        uint8_t buffer[4096];
        uint32 bytes_read = 0;
        do {
            JpegError err = d.readFull(buffer, std::min((uint32)sizeof(buffer), n - bytes_read));
            bytes_read += sizeof(buffer);
            if (err != JpegError::nil()) {
                return err;
            }
        } while(bytes_read < n);
    }
    return JpegError::nil();
}

int32 Decoder::coalescedComponent(int component) {
    Decoder &d = *this;
    int32 retval = int32(component);
    switch (retval) {
    case 1:
        if ((d.componentCoalescing & comp01coalesce) != 0) {
            retval = 0;
        }
        break;
    case 2:
        if ((d.componentCoalescing & comp12coalesce) != 0) {
            retval = 1;
        }
        if ((d.componentCoalescing & comp02coalesce) != 0) {
            retval = 0;
        }
        break;
    }
    return retval;
}

// Specified in section B.2.2.
JpegError Decoder::processSOF(int n) {
    Decoder &d = *this;
    switch (n) {
      case 6 + 3*nGrayComponent:
        d.nComp = nGrayComponent;
        break;
    case 6 + 3*nColorComponent:
        d.nComp = nColorComponent;
        break;
    default:
        return JpegErrorUnsupportedError("SOF has wrong length");
    }
    JpegError err;
    if ((err = d.readFull(d.tmp, n)) != JpegError::nil()) {
        return err;
    }
    // We only support 8-bit precision.
    if (d.tmp[0] != 8) {
        return JpegErrorUnsupportedError("precision");
    }
    d.height = (int(d.tmp[1])<<8) + int(d.tmp[2]);
    d.width = (int(d.tmp[3])<<8) + int(d.tmp[4]);
    if (int(d.tmp[5]) != d.nComp) {
        return JpegErrorUnsupportedError("SOF has wrong number of image components");
    }
    for (int i = 0; i < d.nComp; i++) {
        d.comp[i].c = d.tmp[6+3*i];
        d.comp[i].tq = d.tmp[8+3*i];
        if (d.nComp == nGrayComponent) {
            // If a JPEG image has only one component, section A.2 says "this data
            // is non-interleaved by definition" and section A.2.2 says "[in this
            // case...] the order of data units within a scan shall be left-to-right
            // and top-to-bottom... regardless of the values of H_1 and V_1". Section
            // 4.8.2 also says "[for non-interleaved data], the MCU is defined to be
            // one data unit". Similarly, section A.1.1 explains that it is the ratio
            // of H_i to max_j(H_j) that matters, and similarly for V. For grayscale
            // images, H_1 is the maximum H_j for all components j, so that ratio is
            // always 1. The component's (h, v) is effectively always (1, 1): even if
            // the nominal (h, v) is (2, 1), a 20x5 image is encoded in three 8x8
            // MCUs, not two 16x8 MCUs.
            d.comp[i].h = 1;
            d.comp[i].v = 1;
            continue;
        }
        uint8 hv = d.tmp[7+3*i];
        d.comp[i].h = int(hv >> 4);
        d.comp[i].v = int(hv & 0x0f);
        // For color images, we only support 4:4:4, 4:4:0, 4:2:2 or 4:2:0 chroma
        // downsampling ratios. This implies that the (h, v) values for the Y
        // component are either (1, 1), (1, 2), (2, 1) or (2, 2), and the (h, v)
        // values for the Cr and Cb components must be (1, 1).
        if (i == 0) {
            if (hv != 0x11 && hv != 0x21 && hv != 0x22 && hv != 0x12) {
                return JpegErrorUnsupportedError("luma/chroma downsample ratio");
            }
        } else if (hv != 0x11) {
            return JpegErrorUnsupportedError("luma/chroma downsample ratio");
        }
    }
    return JpegError::nil();
}

// Specified in section B.2.4.1.
JpegError Decoder::processDQT(int n) {
    Decoder &d = *this;
    int qtLength = 1 + JpegBlock::blockSize;
    for (; n >= qtLength; n -= qtLength) {
        JpegError err;
        if ((err = d.readFull(d.tmp, qtLength)) != JpegError::nil()) {
            return err;
        }
        uint8 pq = d.tmp[0] >> 4;
        if (pq != 0) {
            return JpegErrorUnsupportedError("bad Pq value");
        }
        uint8 tq = d.tmp[0] & 0x0f;
        if (tq > maxTq) {
            return JpegErrorFormatError("bad Tq value");
        }
        for (int i = 0; i < JpegBlock::blockSize; ++i) {
            d.quant[tq][i] = int32(d.tmp[i+1]);
        }
    }
    if (n != 0) {
        return JpegErrorFormatError("DQT has wrong length");
    }
    return JpegError::nil();
}

// Specified in section B.2.4.4.
JpegError Decoder::processDRI(int n) {
    Decoder &d = *this;
    if (n != 2) {
        return JpegErrorFormatError("DRI has wrong length");
    }
    JpegError err;
    if ((err = d.readFull(d.tmp, 2)) != JpegError::nil()) {
        return err;
    }
    d.ri = (int(d.tmp[0])<<8) + int(d.tmp[1]);
    return JpegError::nil();
}
class DeferDFlush {
    Decoder * d;
    DecoderWriter * w;
public:
    DeferDFlush(Decoder *d, DecoderWriter *w) {
        this->d = d;
        this->w = w;
    }
    ~DeferDFlush() {
        d->flush(*w);
        w->Close();
    }
};

size_t Decoder::getEstimatedSize() const {
    if (mEstimatedSizeState == ESTIMATED_SIZE_READY && arhc) {
        mEstimatedSize = wbuffer.estimatedByteSize();
        mEstimatedSizeState = ESTIMATED_SIZE_CACHED;
    } else if (mEstimatedSizeState == ESTIMATED_SIZE_READY) {
        size_t header_size = 6;
        size_t size_size = sizeof(uint32_t);
        size_t ext_size = extEndFileBuffer.size()
            + 2* size_size // extension # + 24 bit size + file size
            + 1; // end of extension marker
        size_t retval = header_size;
        retval += ext_size;
        retval += size_size + bitbuffer.estimatedByteSize();
        for (size_t index = 0; index < sizeof(huffMultibuffer) / sizeof(huffMultibuffer[0]); ++index) {
            retval += size_size + huffMultibuffer[index].estimatedByteSize();
        }
        retval += wbuffer.estimatedByteSize();
        mEstimatedSize = retval;
        mEstimatedSizeState = ESTIMATED_SIZE_CACHED;
    }
    if (mEstimatedSizeState == ESTIMATED_SIZE_CACHED) {
        return mEstimatedSize;
    }
    return 0;
}
bool Decoder::estimatedSizeReady() const{
    return mEstimatedSizeState != ESTIMATED_SIZE_UNREADY;
}


// decode reads a JPEG image from r and returns it as an image.Image.
JpegError Decoder::decode(DecoderReader &r, DecoderWriter &w, uint8 componentCoalescing) {
    mEstimatedSizeState = ESTIMATED_SIZE_UNREADY;
    Decoder &d = *this;
    JpegError nil =JpegError::nil();
    d.componentCoalescing = componentCoalescing;
    d.r = &r;
    DeferDFlush flushOnReturn(this, &w);
    // Check for the ARHC Compressed Image marker.
    // Check for the Start Of Image marker.
    {
        JpegError err;
        if ((err = d.readFull(d.tmp, 2)) != nil) {
            return err;
        }
    }
    d.arhc = false;
    if (d.tmp[0] == 'a' && d.tmp[1] == 'r') {
        d.arhc = true;
        // read the rest of arhc headers
        {
            JpegError err;
            if ((err = d.readFull(d.tmp, 6)) != nil) {
                return err;
            }
        }
        if (d.tmp[0] == 'h' && d.tmp[1] == 'c') {
        } else {
            return JpegErrorFormatError("arhc header malformed");
        }
        uint8 *versionExtension = d.tmp + 2;
        while (versionExtension[3] != 0) {
            JpegError err;
            uint8 sizeBuf [4] = {0};
            if ((err = d.readFull(sizeBuf + 1, 3)) != nil) { // only 24 bits of size
                return err;
            }
            std::vector<uint8, JpegAllocator<uint8> > extension(d.mAllocator);
            extension.resize(bufferBEToStreamLength(sizeBuf));
            if (!extension.empty()) {
                if ((err = d.readFull(&extension[0], extension.size())) != nil) { // only 24 bits of size
                    return err;
                }
                if (versionExtension[0] != 1) {
                    assert(false && "We only support arhc major version 1 in this decoder");
                }
                d.componentCoalescing = versionExtension[2];
                switch (versionExtension[3]) {
                  case 1:
                    // end of file extension
                    d.extOriginalFileSize = bufferBEToStreamLength(&extension[0]);
                    d.extEndFileBuffer = std::vector<uint8, JpegAllocator<uint8> >(extension.begin() + 4, extension.end(), mAllocator);
                    d.extEndFileBufferCursor = 0;
                    break;
                  default:
                    fprintf(stderr, "Extension unknown 0x%x\n", versionExtension[3]);
                }
            }
            if ((err = d.readFull(versionExtension + 3, 1)) != nil) {
                return err;
            }
        }
        {
            JpegError err;
            if ((err = d.readFull(d.tmp + 6, 4)) != nil) {
                return err;
            }
        }
        uint32 bitVectorLength = bufferBEToStreamLength(d.tmp + 6);
        if (bitVectorLength > 1024*1024*1024) {
            assert(false && "Bit vector length too large %d");
        }
        byte huffMultibufferLen[sizeof(huffMultibuffer) / sizeof(huffMultibuffer[0]) * 4] = {0};
        {
            JpegError err = d.readFull(huffMultibufferLen, sizeof(huffMultibufferLen));
            if (err != JpegError::nil()) {
                return err;
            }
        }
        for (unsigned int index = 0; index < sizeof(huffMultibuffer) / sizeof(huffMultibuffer[0]); ++index) {
            uint32 huffMultiStreamLen = bufferBEToStreamLength(huffMultibufferLen + index*4);
            //fmt.Printf("Making buffer len %d\n", huffMultiStreamLen)
            if (huffMultiStreamLen > 256*1024*1024) {
                assert(false && "Multi bit vector length too large");
            }
            d.huffMultibuffer[index].buffer.resize(huffMultiStreamLen);
        }
        if (bitVectorLength) {
            d.bitbuffer.buffer.resize(bitVectorLength);
            //fprintf(stderr, "Reading ARHC header of %d bytes\n", len(bitVector))
            d.readFull(&d.bitbuffer.buffer[0], bitVectorLength);
        }

        //fprintf(stderr, "Reading ARHC header of %d bytes\n", len(huffVector))
        for (unsigned int index = 0; index < sizeof(huffMultibuffer) / sizeof(huffMultibuffer[0]); ++index) {
            if (!d.huffMultibuffer[index].buffer.empty()) {
                d.readFull(&d.huffMultibuffer[index].buffer[0], d.huffMultibuffer[index].buffer.size());
            }
        }
        // get jpeg headers
        JpegError err;
        if ((err = d.readFull(d.tmp, 2)) != nil) {
            return err;
        }
        d.wbuffer.clear();
        d.wbuffer.appendByte(0xff);
        d.wbuffer.appendByte(0xd8);
    }
    if (d.tmp[0] != 0xff || d.tmp[1] != soiMarker) {
        return JpegErrorFormatError("missing SOI marker");
    }

    // Process the remaining segments until the End Of Image marker.
    while (true) {
        {
            JpegError err = d.readFull(d.tmp, 2);
            //fprintf(stderr, "MARKER READ %x\n", d.tmp[:2])
            if (err != nil) {
                return err;
            }
        }
        while (d.tmp[0] != 0xff) {
            // Strictly speaking, this is a format error. However, libjpeg is
            // liberal in what it accepts. As of version 9, next_marker in
            // jdmarker.c treats this as a warning (JWRN_EXTRANEOUS_DATA) and
            // continues to decode the stream. Even before next_marker sees
            // extraneous data, jpeg_fill_bit_buffer in jdhuff.c reads as many
            // bytes as it can, possibly past the end of a scan's data. It
            // effectively puts back any markers that it overscanned (e.g. an
            // "\xff\xd9" EOI marker), but it does not put back non-marker data,
            // and thus it can silently ignore a small number of extraneous
            // non-marker bytes before next_marker has a chance to see them (and
            // print a warning).
            //
            // We are therefore also liberal in what we accept. Extraneous data
            // is silently ignored.
            //
            // This is similar to, but not exactly the same as, the restart
            // mechanism within a scan (the RST[0-7] markers).
            //
            // Note that extraneous 0xff bytes in e.g. SOS data are escaped as
            // "\xff\x00", and so are detected a little further down below.
            d.tmp[0] = d.tmp[1];
            uint8E readByteErr = d.readByte();
            d.tmp[1] = readByteErr.first;
            //fmt.Printf("STRICTLY SPEAKING %x\n", d.tmp[:2])
            if (readByteErr.second != nil) {
                return readByteErr.second;
            }
        }
        uint8 marker = d.tmp[1];
        if (marker == 0) {
            //fprintf(stderr, "XTRA DATA %x\n", d.tmp[:2])
            // Treat "\xff\x00" as extraneous data.
            continue;
        }
        while (marker == 0xff) {
            // Section B.1.1.2 says, "Any marker may optionally be preceded by any
            // number of fill bytes, which are bytes assigned code X'FF'".
            uint8E markerErr = d.readByte();
            marker = markerErr.first;
            //fmt.Printf("eXTRA DATA %x\n", marker)
            if (markerErr.second != nil) {
                return markerErr.second;
            }
        }
        if (marker == eoiMarker) { // End Of Image.
            //fprintf(stderr, "EOI %x\n", marker)
            //fprintf(stderr, "Last bytes of image %x nbits:%d %d\n", d.wbuffer.buffer[len(d.wbuffer.buffer) - 16:], d.wbuffer.nBits, d.wbuffer.bits)
            break;
        }
        if (rst0Marker <= marker && marker <= rst7Marker) {
            //fmt.Printf("Rst %x %x\n", rst0Marker, marker)
            // Figures B.2 and B.16 of the specification suggest that restart markers should
            // only occur between Entropy Coded Segments and not after the final ECS.
            // However, some encoders may generate incorrect JPEGs with a final restart
            // marker. That restart marker will be seen here instead of inside the processSOS
            // method, and is ignored as a harmless error. Restart markers have no extra data,
            // so we check for this before we read the 16-bit length of the segment.
            continue;
        }


        JpegError err = JpegError::nil();
        // Read the 16-bit length of the segment. The value includes the 2 bytes for the
        // length itself, so we subtract 2 to get the number of remaining bytes.
        if ((err = d.readFull(d.tmp, 2)) != nil) {
            //fmt.Printf("Seg length %x\n", d.tmp[:2])
            return err;
        }
        int n = (int(d.tmp[0])<<8) + int(d.tmp[1]) - 2;
        if (n < 0) {
            //fmt.Printf("Short semgnet %d\n", n)
            return JpegErrorFormatError("short segment length");
        }

        if (marker == sof0Marker || marker == sof2Marker) { // Start Of Frame.
            d.progressive = marker == sof2Marker;
            err = d.processSOF(n);
        } else if (marker == dhtMarker) { // Define Huffman Table.
            err = d.processDHT(n);
        } else if (marker == dqtMarker) { // Define Quantization Table.
            err = d.processDQT(n);
        } else if (marker == sosMarker) { // Start Of Scan.
            err = d.processSOS(n);
        } else if (marker == driMarker) { // Define Restart Interval.
            err = d.processDRI(n);
        } else if ((app0Marker <= marker && marker <= app15Marker) || marker == comMarker) { // APPlication specific, or Comment
            err = d.ignore(n);
        } else {
            //fprintf(stderr, "UNKNOWN %x\n", marker);
            err = JpegErrorUnsupportedError("unknown marker");
        }
        if (err != nil) {
            return err;
        }
    }
    while (true) {
        uint8E cur = readByte(); // deal with stray junk at the end of the image
        if (cur.second != JpegError::nil()) {
            break;
        }
    }
    return nil;
}

unsigned char MAGIC_7Z[6] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };
unsigned char MAGIC_ARHC[6] = { 'A', 'R', 'H', 'C', 0x01, 0x00 };

bool DecodeIs7z(const uint8* data, size_t size) {
    return memcmp(data, MAGIC_7Z, std::min(size, sizeof(MAGIC_7Z))) == 0;
}

bool DecodeIsARHC(const uint8* data, size_t size) {
    return memcmp(data, MAGIC_ARHC, std::min(size, sizeof(MAGIC_ARHC))) == 0;
}

// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError Decode(DecoderReader &r, DecoderWriter &w, uint8 componentCoalescing, const JpegAllocator<uint8_t> &alloc) {
    Decoder d(alloc);
    return d.decode(r, w, componentCoalescing);
}
// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError CompressJPEGtoARHC(DecoderReader &r, DecoderWriter &w, uint8 compression_level, uint8 componentCoalescing,
                             const JpegAllocator<uint8_t> &alloc) {
    MagicNumberReplacementWriter magic(&w,
                                       std::vector<uint8_t, JpegAllocator<uint8_t> >(MAGIC_7Z, MAGIC_7Z + sizeof(MAGIC_7Z), alloc),
                                       std::vector<uint8_t, JpegAllocator<uint8_t> >(MAGIC_ARHC, MAGIC_ARHC + sizeof(MAGIC_ARHC), alloc));
    DecoderCompressionWriter cw(&magic, compression_level, alloc);
    Decoder d(alloc);
    return d.decode(r, cw, componentCoalescing);
}
// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError DecompressARHCtoJPEG(DecoderReader &r, DecoderWriter &w,
                             const JpegAllocator<uint8_t> &alloc) {
    MagicNumberReplacementReader magic(&r,
                                       std::vector<uint8_t, JpegAllocator<uint8_t> >(MAGIC_ARHC, MAGIC_ARHC + sizeof(MAGIC_ARHC), alloc),
                                       std::vector<uint8_t, JpegAllocator<uint8_t> >(MAGIC_7Z, MAGIC_7Z + sizeof(MAGIC_7Z), alloc));
    DecoderDecompressionReader cr(&magic, alloc);
    Decoder d(alloc);
    return d.decode(cr, w, 0);
}

}
