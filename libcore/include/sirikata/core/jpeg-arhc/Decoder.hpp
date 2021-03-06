/*  Sirikata Jpeg Reader Transfer -- Texture Transfer management system
 *  FileTransferHandler.hpp
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
#include <string.h>
#include <assert.h>
//#include <sirikata/core/util/Platform.hpp>
#include "DecoderPlatform.hpp"
#include "Huffman.hpp"
#include "Reader.hpp"
#include "SizeEstimator.hpp"
namespace Sirikata {

struct JpegBlock {
    enum {
        blockSize = 64
    };
    int32 block[blockSize];
    JpegBlock() {
        memset(this, 0, sizeof(JpegBlock));
    }
    int32& operator[](const uint32 offset) {
        assert(offset < blockSize);
        return block[offset];
    }
    int32 operator[](const uint32 offset) const {
        assert(offset < blockSize);
        return block[offset];
    }
};


// Component specification, specified in section B.2.2.
struct SIRIKATA_EXPORT component {
	int h;   // Horizontal sampling factor.
	int v;   // Vertical sampling factor.
	uint8 c; // Component identifier.
	uint8 tq; // Quantization table destination selector.
    component() {
        memset(this, 0, sizeof(component));
    }
};


struct SIRIKATA_EXPORT BitByteStream {
    std::vector<uint8, JpegAllocator<uint8_t> > buffer;
	uint32 bits;
	uint8 nBits;
	uint32 bitReadCursor;
    BitByteStream(const JpegAllocator<uint8>&alloc=JpegAllocator<uint8>());
    void appendByte(uint8 x);
    void appendBytes(uint8*bytes, uint32 nBytes);
    void clear();
    void flushToWriter(DecoderWriter&);
    void emitBits(uint16 bits, uint8 nBits, bool stuffZeros);
    std::pair<uint32, JpegError> scanBits(uint32 nBits, bool stuffZeros);
    std::pair<uint32, JpegError> scanBitsNoStuffedZeros(uint32 nBits);
    std::pair<uint8, JpegError> scanAlignedByte();
    void pop();
    uint32 len() const;
    uint32 estimatedByteSize()const;
    void flushBits(bool stuffBits);
};

class SIRIKATA_EXPORT Decoder : public SizeEstimator {
    mutable size_t mEstimatedSize;
    mutable enum {
        ESTIMATED_SIZE_UNREADY,
        ESTIMATED_SIZE_READY,
        ESTIMATED_SIZE_CACHED
    } mEstimatedSizeState;
public:
    size_t getEstimatedSize() const;
    bool estimatedSizeReady() const;
    virtual ~Decoder(){}
    enum {

        dcTable = 0,
        acTable = 1,
        maxTc   = 1,
        maxTh   = 3,
        maxTq   = 3,

        // A grayscale JPEG image has only a Y component.
        nGrayComponent = 1,
        // A color JPEG image has Y, Cb and Cr components.
        nColorComponent = 3,
        
        // We only support 4:4:4, 4:4:0, 4:2:2 and 4:2:0 downsampling, and therefore the
        // number of luma samples per chroma sample is at most 2 in the horizontal
        // and 2 in the vertical direction.
        maxH = 2,
        maxV = 2,
    };
    enum {
        soiMarker   = 0xd8, // Start Of Image.
        eoiMarker   = 0xd9, // End Of Image.
        sof0Marker  = 0xc0, // Start Of Frame (Baseline).
        sof2Marker  = 0xc2, // Start Of Frame (Progressive).
        dhtMarker   = 0xc4, // Define Huffman Table.
        dqtMarker   = 0xdb, // Define Quantization Table.
        sosMarker   = 0xda, // Start Of Scan.
        driMarker   = 0xdd, // Define Restart Interval.
        rst0Marker  = 0xd0, // ReSTart (0).
        rst7Marker  = 0xd7, // ReSTart (7).
        app0Marker  = 0xe0, // APPlication specific (0).
        app15Marker = 0xef, // APPlication specific (15).
        comMarker   = 0xfe // COMment.
    };
    const int * unzig() {
        // unzig maps from the zig-zag ordering to the natural ordering. For example,
        // unzig[3] is the column and row of the fourth element in zig-zag order. The
        // value is 16, which means first column (16%8 == 0) and third row (16/8 == 2).
        static int sunzig [JpegBlock::blockSize] = {
            0, 1, 8, 16, 9, 2, 3, 10,
            17, 24, 32, 25, 18, 11, 4, 5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13, 6, 7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63,
        };
        return sunzig;
    }
    struct Bits {
        uint32 a; // accumulator.
        uint32 m; //mask. m == 1<<(n-1) when n>0, with m==0 when n==0
        int32 n; // the number of unread bits in a.
        Bits() {
            memset(this, 0, sizeof(Bits));
        }
    };
    enum {
        optionReserved0 = 1 << 0,
        optionReserved1 = 1 << 1,
        optionReserved2 = 1 << 2,
        optionReserved3 = 1 << 3,
        optionReserved4 = 1 << 4,
        comp01coalesce  = 1 << 5,
        comp02coalesce  = 1 << 6,
        comp12coalesce  = 1 << 7
    };
    typedef std::pair<uint8, JpegError> uint8E;
    typedef std::pair<uint32, JpegError> uint32E;
    typedef std::pair<int32, JpegError> int32E;
    void flush(DecoderWriter &w);
    JpegError decode(DecoderReader &r, DecoderWriter &w, uint8 componentCoalescing);
    Decoder(const JpegAllocator<uint8_t>&customAlloc=JpegAllocator<uint8_t>())
        : mAllocator(customAlloc),
          wbuffer(customAlloc),
          bitbuffer(customAlloc),
          extEndFileBuffer(customAlloc) {
        mEstimatedSizeState = ESTIMATED_SIZE_UNREADY;
        for (size_t i = 0; i < sizeof(progCoeffs) / sizeof(progCoeffs[0]); ++i) {
            progCoeffs[i].~vector();
            new(&progCoeffs[i])std::vector<uint8_t, JpegAllocator<uint8_t> >(customAlloc);
        }
        for (size_t i = 0; i < sizeof(huffMultibuffer) / sizeof(huffMultibuffer[0]); ++i) {
            huffMultibuffer[i].~BitByteStream();
            new(&huffMultibuffer[i])BitByteStream(customAlloc);
        }
        r = NULL;
        width = height = 0;
        ri = 0;
        nComp = 0;
        progressive = false;
        arhc = false;
        offset = 0;
        huffTransSect = false;
        eobRun = 0;
        memset(tmp, 0, sizeof(tmp));
        componentCoalescing = 0;
        extOriginalFileSize = 0;
        extEndFileBufferCursor = 0;
        extEndEncountered = false;
    }
private:
    JpegAllocator<uint8_t> mAllocator;
	DecoderReader *r;
	Bits bits;
	// bytes is a byte buffer, similar to a bufio.Reader, except that it
	// has to be able to unread more than 1 byte, due to byte stuffing.
	// Byte stuffing is specified in section F.1.2.3.
	struct Bytes {
		// buf[i:j] are the buffered bytes read from the underlying
		// io.Reader that haven't yet been passed further on.
		uint8 buf  [4096];
		int i;
        int j;
		// nUnreadable is the number of bytes to back up i after
		// overshooting. It can be 0, 1 or 2.
		int nUnreadable;
        Bytes();
	} bytes;
	int width;
    int height;
	int ri;
	int nComp;
	bool progressive;
	bool arhc;   // is this in compressed arhc format
	uint32 offset; // where we are in the file (for debug purposes)
	bool huffTransSect;   // are we in the huffman translation section of code
	uint16 eobRun; // End-of-Band run, specified in section G.1.2.2.
	component comp[nColorComponent];
	std::vector<JpegBlock, JpegAllocator<uint8_t> > progCoeffs[nColorComponent]; // Saved state between progressive-mode scans.
	huffman huff[maxTc + 1][maxTh + 1];
	JpegBlock quant[maxTq + 1]; // Quantization tables, in zig-zag order.
	uint8 tmp[JpegBlock::blockSize + 1];
	BitByteStream wbuffer;       
	BitByteStream bitbuffer;      // the buffer of bitstreams at the beginning of file
	// this is used to reconstruct the scan regions without
	// clogging up the parts that should be huffman coded
	BitByteStream huffMultibuffer[64 * 3];      // one stream per 64 value per channel
	uint8 componentCoalescing;           // & 0x20 if comp[0] and comp[1] are coalesced
	// & 0x40 if comp[1] and comp[2] are coalesced
	// & 0x80 if comp[0] and comp[2] are coalesced
	uint32 extOriginalFileSize;
    std::vector<uint8, JpegAllocator<uint8_t> > extEndFileBuffer;
	uint32 extEndFileBufferCursor;
	bool extEndEncountered;

    // fill fills up the d.bytes.buf buffer from the underlying io.Reader. It
    // should only be called when there are no unread bytes in d.bytes.
    JpegError fill();


    // unreadByteStuffedByte undoes the most recent readByteStuffedByte call,
    // giving a byte of data back from d.bits to d.bytes. The Huffman look-up table
    // requires at least 8 bits for look-up, which means that Huffman decoding can
    // sometimes overshoot and read one or two too many bytes. Two-byte overshoot
    // can happen when expecting to read a 0xff 0x00 byte-stuffed byte.
    void unreadByteStuffedByte();

    void  appendByteToWriteBuffer(uint8 x);
    void appendBytesToWriteBuffer(uint8* x, uint32 nBytes);
    uint8E readByte();

    // readByteStuffedByte is like readByte but is for byte-stuffed Huffman data.
    uint8E readByteStuffedByte();

    // readFull reads exactly len(p) bytes into p. It does not care about byte
    // stuffing.
    JpegError readFull(uint8 *p, uint32 nBytes);
    // ignore ignores the next n bytes.
    JpegError ignore(uint32 n);

    int32 coalescedComponent(int component);
// Specified in section B.2.2.
    JpegError processSOF(int n);
// Specified in section B.2.4.1.
    JpegError processDQT(int n);
// Specified in section B.2.4.4.
    JpegError processDRI(int n);
// decode reads a JPEG image from r and returns it as an image.Image.
    JpegError ensureNBits(int32 n);
    int32E receiveExtend(uint8 t);
    JpegError processDHT(int n);
    uint8E decodeHuffman(huffman *h, int32 zig, int component);
    uint8E decodeBit();
    uint32E decodeBits(int32 n);
    JpegError processSOS(int n);
    JpegError refine(JpegBlock* b, huffman* h, int32 zigStart, int32 zigEnd, int32 delta, int component);
    int32E refineNonZeroes(JpegBlock *b, int32 zig, int32 zigEnd, int32 nz, int32 delta);
};

// Decode reads a JPEG image from r and returns it as an image.Image.
SIRIKATA_FUNCTION_EXPORT JpegError Decode(DecoderReader &r, DecoderWriter &w, uint8 componentCoalescing,
                                          const JpegAllocator<uint8_t> &alloc);
SIRIKATA_FUNCTION_EXPORT JpegError CompressJPEGtoARHC(DecoderReader &r, DecoderWriter &w,
                                                      uint8 compression_level, uint8 componentCoalescing,
                                                      const JpegAllocator<uint8_t> &alloc);
SIRIKATA_FUNCTION_EXPORT JpegError DecompressARHCtoJPEG(DecoderReader &r, DecoderWriter &w,
                                                        const JpegAllocator<uint8_t> &alloc);

class ThreadContext;
SIRIKATA_FUNCTION_EXPORT JpegError CompressJPEGtoARHCMulti(DecoderReader &r, DecoderWriter &w,
                                                           uint8 compression_level, uint8 componentCoalescing,
                                                           bool do_lzham,
                                                           ThreadContext *tc);
SIRIKATA_FUNCTION_EXPORT JpegError DecompressARHCtoJPEGMulti(DecoderReader &r, DecoderWriter &w,
                                                             ThreadContext *tc);


SIRIKATA_FUNCTION_EXPORT bool DecodeIs7z(const uint8* data, size_t size);
SIRIKATA_FUNCTION_EXPORT bool DecodeIsARHC(const uint8* data, size_t size);

}
