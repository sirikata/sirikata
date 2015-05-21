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

#include <sirikata/core/jpeg-arhc/Decoder.hpp>
#include <stdio.h>

namespace Sirikata {

void BitByteStream::appendByte(uint8 x) {
    buffer.push_back(x);
}
void BitByteStream::appendBytes(uint8*bytes, uint32 nBytes) {
    buffer.insert(buffer.end(), bytes, bytes + nBytes);
}

std::pair<uint32, JpegError> BitByteStream::scanAlignedByte() {
    assert((bitReadCursor&0x7) == 0);
    uint32 byteAddress = (bitReadCursor >> 3);
    bitReadCursor +=8;
    if(byteAddress >= buffer.size()) {
        return Decoder::uint32E(0, MakeJpegError("Reading off the end of byte buffer"));
    }
    return Decoder::uint32E(buffer[byteAddress], JpegError::nil());
}
std::pair<uint32, JpegError> BitByteStream::scanBitsNoStuffedZeros(uint32 nBits) {

    if (nBits == 0) {
        return Decoder::int32E(0, JpegError::nil()); // don't read off the array since it may be empty or at its end
    }
    uint32 byteAddress = (this->bitReadCursor >> 3);
    if (nBits > 16) {
        assert(nBits <= 16);
        return Decoder::uint32E(0, MakeJpegError("Must have nBits < 16"));
    }
    uint32 bitAddress = this->bitReadCursor - byteAddress*8;
    if (int(byteAddress) + ((bitAddress + nBits - 1) >> 3) >= this->buffer.size()) {
        return Decoder::uint32E(0, MakeJpegError("Reading off the end of bit buffer"));
    }
    uint32 retval = 0;
    std::vector<uint8, JpegAllocator<uint8_t> >::const_iterator bufferPtr = this->buffer.begin() + byteAddress;
    uint32 curByte = (*bufferPtr) & ((1 << (8 - bitAddress)) - 1);
    retval |= uint32(curByte);
    uint8 remainingBitsInByte = 8 - bitAddress;
    if (remainingBitsInByte >= nBits) {
        retval >>= remainingBitsInByte - nBits;
        this->bitReadCursor += nBits;
        return Decoder::uint32E(retval, JpegError::nil());
    }
    this->bitReadCursor += remainingBitsInByte;
    nBits -= remainingBitsInByte;
    if (nBits > 8) {
        this->bitReadCursor += 8;
        ++bufferPtr;
        retval <<= 8;
        retval |= uint32(*bufferPtr);
        nBits -= 8;
    }
    retval <<= nBits;
    retval |= uint32((*++bufferPtr) >> (8 - nBits));
    this->bitReadCursor += nBits;
    return Decoder::uint32E(retval, JpegError::nil());    
}
std::pair<uint32, JpegError> BitByteStream::scanBits(uint32 nBits, bool stuffZeros) {
    BitByteStream &b = *this;
    if (nBits > 16) {
    }
    if (nBits == 0) {
        return Decoder::int32E(0, JpegError::nil()); // don't read off the array since it may be empty or at its end
    }
    uint32 byteAddress = b.bitReadCursor / 8;
    if (int(byteAddress) >= b.buffer.size()) {
        return Decoder::uint32E(0, MakeJpegError("Reading off the end of bit buffer"));
    }
    uint32 bitAddress = b.bitReadCursor - byteAddress*8;
    uint32 retval = 0;
    uint32 curByte = b.buffer[byteAddress] & ((1 << (8 - bitAddress)) - 1);
    retval |= uint32(curByte);
    uint8 remainingBitsInByte = 8 - bitAddress;
    //fmt.Printf("Retval %x[%d].%d so far,  Remaining bits %d\n", retval, byteAddress,bitAddress,nBits)
    if (remainingBitsInByte <= nBits && b.buffer[byteAddress] == 0xff && stuffZeros) {
        if (b.buffer[byteAddress+1] != 0x0) {
            assert(false && "Must be stuffed byte");
        }
        //fprintf(stderr, "Stuffed byte at %d\n", byteAddress)
        byteAddress++;
        b.bitReadCursor += 8;
    }
    if (remainingBitsInByte >= nBits) {
        retval >>= remainingBitsInByte - nBits;
        //fmt.Printf("Returning early after single byte read\n")
        b.bitReadCursor += nBits;
        return Decoder::uint32E(retval, JpegError::nil());
    }
    if (int(byteAddress) >= b.buffer.size()) {
        return Decoder::uint32E(0, MakeJpegError("Reading off end of bit buffer"));
    }
    b.bitReadCursor += remainingBitsInByte;
    nBits -= remainingBitsInByte;
    if (nBits > 8) {
        b.bitReadCursor += 8;
        byteAddress += 1;
        retval <<= 8;
        retval |= uint32(b.buffer[byteAddress]);
        nBits -= 8;
        if (b.buffer[byteAddress] == 0xff && stuffZeros) {
            //fprintf(stderr, "Second byte needs stuffing @ %d\n", byteAddress);
            if (size_t(byteAddress)+1 >= b.buffer.size()) {
                return Decoder::uint32E(0, MakeJpegError("Reading off end of bit buffer"));
            }
            if (b.buffer[byteAddress+1] != 0x0) {
                return Decoder::uint32E(0, MakeJpegError("Reading should be stuffed with 0 not <item>"));
            }
            byteAddress++;
            b.bitReadCursor += 8;
        }
    }

    if (nBits > 8) {
        assert(false &&"unreachable: we should have only 16 bits to grab");
    }
    //fmt.Printf("Pref Final things are %x going to read %x after shifting %d\n", retval, b.buffer[byteAddress + 1], nBits)
    if (size_t(byteAddress)+1 >= b.buffer.size()) {
        return Decoder::uint32E(0, MakeJpegError("Reading off end of bit buffer"));
    }
    retval <<= nBits;
    retval |= uint32(b.buffer[byteAddress+1] >> (8 - nBits));
    b.bitReadCursor += nBits;
    //fprintf(stderr, "Final value %x\n", retval)
    if (nBits == 8 && b.buffer[byteAddress+1] == 0xff && stuffZeros) {
        if (size_t(byteAddress)+2 >= b.buffer.size()) {
            return Decoder::uint32E(0, MakeJpegError("Reading off end of bit buffer"));
        }
        if (b.buffer[byteAddress+2] != 0x0) {
            return Decoder::uint32E(retval, MakeJpegError("Reading should be stuffed with 0 not <item>"));
        }
        byteAddress++;
        b.bitReadCursor += 8;
    }
    return Decoder::uint32E(retval, JpegError::nil());
}


void BitByteStream::emitBits(uint32 bits, uint32 nBits, bool stuffZeros) {
    BitByteStream &b = *this;
    nBits += uint32(b.nBits);
    if (nBits > 32) {
        assert(nBits <=32 && "emitBits only supports 32 at a time");
        return;
    }
    bits <<= 32 - nBits;
    bits |= b.bits;
    uint8 bb = 0;
    while (nBits >=8) {
        bb = uint8(bits >> 24);
        b.appendByte(bb);
        if (bb == 0xff && stuffZeros) {
            b.appendByte(0);
        }
        bits <<= 8;
        nBits -= 8;
    }
    if(0)
    switch (nBits) {
      case 32:
      case 31:
      case 30:
      case 29:
      case 28:
      case 27:
      case 26:
      case 25:
        bb = uint8(bits >> 24);
        b.appendByte(bb);
        if (bb == 0xff && stuffZeros) {
            b.appendByte(0);
        }
        bits <<= 8;
        nBits -= 8;
      case 24:
      case 23:
      case 22:
      case 21:
      case 20:
      case 19:
      case 18:
      case 17:
        bb = uint8(bits >> 24);
        b.appendByte(bb);
        if (bb == 0xff && stuffZeros) {
            b.appendByte(0);
        }
        bits <<= 8;
        nBits -= 8;
      case 16:
      case 15:
      case 14:
      case 13:
      case 12:
      case 11:
      case 10:
      case 9:
      case 8:
        bb = uint8(bits >> 24);
        b.appendByte(bb);
        if (bb == 0xff && stuffZeros) {
            b.appendByte(0);
        }
        bits <<= 8;
        nBits -= 8;
      default:
        break;
    }
    b.bits = bits;
    b.nBits = uint8(nBits);
}

// ensureNBits reads bytes from the byte buffer to ensure that d.bits.n is at
// least n. For best performance (avoiding function calls inside hot loops),
// the caller is the one responsible for first checking that d.bits.n < n.
JpegError Decoder::ensureNBits(int32 n) {
    Decoder &d = *this;
	while (true) {
		uint8E cErr = d.readByteStuffedByte();
        if (cErr.second != JpegError::nil()) {
			if (cErr.second == JpegError::errEOF()) {
                return JpegError::errShortHuffmanData();
			}
			return cErr.second;
		}
		d.bits.a = (d.bits.a<<8) | (uint32)(cErr.first);
		d.bits.n += 8;
		//fmt.Printf("Ensuring %d bits by reading %x: next bits for reading are %d bits %x max:%x\n",
		//    n, c, d.bits.n, d.bits.a, d.bits.m)
		if (d.bits.m == 0) {
			d.bits.m = 1 << 7;
		} else {
			d.bits.m <<= 8;
		}
		if (d.bits.n >= n) {
            break;
		}
	}
	return JpegError::nil();
}

// receiveExtend is the composition of RECEIVE and EXTEND, specified in section
// F.2.2.1.
Decoder::int32E Decoder::receiveExtend(uint8 t) {
    Decoder &d = *this;
	if (d.arhc) { // here we just return the original value
		uint32E valErr = d.bitbuffer.scanBitsNoStuffedZeros(uint32(t));
        d.wbuffer.emitBits(valErr.first, uint32(t), true);
        int32 s = (int32)(1) << t;
        int32 x = (int32)(valErr.first);
        if (x < (s>>1)) { // FIXME:
			x += ((-1) << t) + 1;
		}
		return int32E(x, valErr.second);
	}
	if (!d.huffTransSect) {
		assert(false && "HUFF TRANS SECT SHOULD BE SET");
	}

	if (d.bits.n < int32(t)) {
        JpegError err;
		if ((err = d.ensureNBits(int32(t))) != JpegError::nil()) {
			return int32E(0, err);
		}
	}
	d.bits.n -= int32(t);
	d.bits.m >>= t;
    int32 s = int32(1) << t;
    int32 x = int32(d.bits.a>>uint8(d.bits.n)) & (s - 1);
	if (x > 65535) {
		assert(false && "Cannot decode more than 16 bits at a time");
	}
	//fmt.Printf("Writing raw %x (%d bits)\n", x, t)
	d.bitbuffer.emitBits(uint32(x), uint32(t), false);
	if (x < (s>>1)) {
        x += ((-1) << t) + 1;
	}
	return int32E(x, JpegError::nil());
}

// processDHT processes a Define Huffman Table marker, and initializes a huffman
// struct from its contents. Specified in section B.2.4.2.
JpegError Decoder::processDHT(int n) {
    Decoder &d = *this;
    while (n > 0) {
        if (n < 17) {
            return JpegErrorFormatError("DHT has wrong length");
		}
        {
            JpegError err;
            if ((err = d.readFull(d.tmp, 17)) != JpegError::nil()) {
                return err;
            }
        }
        uint8 tc = d.tmp[0] >> 4;
		if (tc > maxTc) {
			return JpegErrorFormatError("bad Tc value");
		}
        uint8 th = d.tmp[0] & 0x0f;
        if (th > maxTh || (th > 1 && !d.progressive)) {
            return JpegErrorFormatError("bad Th value");
		}
        huffman &h = d.huff[tc][th];

		// Read nCodes and h.vals (and derive h.nCodes).
		// nCodes[i] is the number of codes with code length i.
		// h.nCodes is the total number of codes.
		h.nCodes = 0;
        int32 nCodes [huffman::maxCodeLength];
        for (int i = 0; i < huffman::maxCodeLength; ++i) {
			nCodes[i] = (int32)(d.tmp[i+1]);
			h.nCodes += nCodes[i];
		}
		if (h.nCodes == 0) {
			return JpegErrorFormatError("Huffman table has zero length");
		}
		if (h.nCodes > huffman::maxNCodes) {
			return JpegErrorFormatError("Huffman table has excessive length");
		}
		n -= int(h.nCodes) + 17;
		if (n < 0) {
			return JpegErrorFormatError("DHT has wrong length");
		}
        {
            JpegError err;
            if ((err = d.readFull(h.vals, h.nCodes)) != JpegError::nil()) {
                return err;
            }
        }
		// Derive the look-up table.
		for (int i = 0; i < (1 << huffman::lutSize); ++i) {
			h.lut[i] = 0;
		}
		uint32 x = 0;
        uint32 code = 0;
		for (uint32 i = 0; i < huffman::lutSize; i++) {
			code <<= 1;
			for (int32 j = 0; j < nCodes[i]; j++) {
				// The codeLength is 1+i, so shift code by 8-(1+i) to
				// calculate the high bits for every 8-bit sequence
				// whose codeLength's high bits matches code.
				// The high 8 bits of lutValue are the encoded value.
				// The low 8 bits are 1 plus the codeLength.
				uint8 base = uint8(code << (7 - i));
				uint16 lutValue = (uint16(h.vals[x])<<8) | uint16(2+i);
                for (uint8 k = 0; k < (1<<(7-i)); k++) {
					h.lut[base | k] = lutValue;
				}
				code++;
				x++;
			}
		}
		code = 0;
        x = 0;
        for (uint32 nBits = 0; nBits < huffman::maxCodeLength; nBits++) {
			code <<= 1;
			for (int32 j = 0; j < nCodes[nBits]; j++) {
				uint32 encodingValue = 0;
				encodingValue = nBits + 1;
				encodingValue <<= 24;
				encodingValue |= code;
				h.huffmanLUTencoding[h.vals[x]] = encodingValue;
				code++;
				x++;
			}
		}

		// Derive minCodes, maxCodes, and valsIndices.
		int32 c = 0;
        int32 index = 0;
        for (int i = 0; i < huffman::maxCodeLength; ++i) {
            int32 n = nCodes[i];
			if (n == 0) {
				h.minCodes[i] = -1;
				h.maxCodes[i] = -1;
				h.valsIndices[i] = -1;
			} else {
				h.minCodes[i] = c;
				h.maxCodes[i] = c + n - 1;
				h.valsIndices[i] = index;
				c += n;
				index += n;
			}
			c <<= 1;
		}
	}
	return JpegError::nil();
}

// decodeHuffman returns the next Huffman-coded value from the bit-stream,
// decoded according to h.
Decoder::uint8E Decoder::decodeHuffman(huffman *h, int32 zig, int component) {
    Decoder &d = *this;
	if (!d.huffTransSect) {
		assert(false && "HUFF TRANS SECT SHOULD BE SET");
	}
	if (d.arhc) { // here we just return the original value
		uint32E unhuffmanValueErr = d.huffMultibuffer[zig+64*d.coalescedComponent(component)].scanAlignedByte();
		// but we also need to write the huffman'd value to the array
		//printf("Writing %x => %x to the output\n", unhuffmanValueErr.first, h.huffmanLUTencoding[unhuffmanValueErr.first])
        uint32 hval = h->huffmanLUTencoding[unhuffmanValueErr.first];
		d.wbuffer.emitBits(hval&0xffff, (hval >> 24), true);
		return uint8E(uint8(unhuffmanValueErr.first), unhuffmanValueErr.second);
	}
	if (h->nCodes == 0) {
        return uint8E(0, JpegErrorFormatError("uninitialized Huffman table"));
	}

	if (d.bits.n < 8) {
        JpegError err;
		if ((err = d.ensureNBits(8)) != JpegError::nil()) {
			if (err != JpegError::errMissingFF00() && err != JpegError::errShortHuffmanData()) {
				return uint8E(0, err);
			}
			// There are no more bytes of data in this segment, but we may still
			// be able to read the next symbol out of the previously read bits.
			// First, undo the readByte that the ensureNBits call made.
			//fmt.Printf("Going to unread stuffed byte %d bits %x max:%x\n",
			//    d.bits.n, d.bits.a, d.bits.m)
			d.unreadByteStuffedByte();
			//fmt.Printf("Unread stuffed byte %d bits %x max:%x\n",
			//    d.bits.n, d.bits.a, d.bits.m)

			goto slowPath;
		}
	}
    {
        uint32 v = 0;
        if ((v = h->lut[(d.bits.a >> uint32(d.bits.n-huffman::lutSize)) & 0xff]) != 0) {
            uint8 n = (v & 0xff) - 1;
            uint32 reverseHuffmanLookupAssert = uint32(n);
            reverseHuffmanLookupAssert <<= uint32(24);
            reverseHuffmanLookupAssert |= ((d.bits.a >> (uint32(d.bits.n) - uint32(n))) &
                                           ((1 << n) - 1)); // only use that many bits as required from the accumulator
            // make sure our reverse encoding functions properly
            if (h->huffmanLUTencoding[uint8(v>>8)] != reverseHuffmanLookupAssert) {
                fprintf(stderr, "%d bits read as %x => %x =/=> %x\n",
                        n, reverseHuffmanLookupAssert & 0xffffff,
                        uint8(v>>8), h->huffmanLUTencoding[uint8(v >> 8)]);
                assert(false && "incorrect bit reading from huffman");
            }
            //printf("%d bits read as %x => %x => %x\n",
            //        n, reverseHuffmanLookupAssert & 0xffffff,
            //        uint8(v >> 8), h.huffmanLUTencoding[uint8(v >> 8)])
            d.bits.n -= int32(n);
            d.bits.m >>= n;
            uint8 appendedByte = uint8(v >> 8);
            d.huffMultibuffer[zig+64*d.coalescedComponent(component)].appendByte(appendedByte);
            return uint8E(uint8(v >> 8), JpegError::nil());
        }
    }
slowPath:
    int32 code = 0;
	for (int i = 0; i < huffman::maxCodeLength; i++) {
		if (d.bits.n == 0) {
            JpegError err;
			if ((err = d.ensureNBits(1)) != JpegError::nil()) {
				return uint8E(0, err);
			}
		}
		if ((d.bits.a & d.bits.m) != 0) {
			code |= 1;
		}
		d.bits.n--;
		d.bits.m >>= 1;
		if (code <= h->maxCodes[i]) {
			uint8 retval = h->vals[h->valsIndices[i]+code-h->minCodes[i]];
            uint8 appendedByte = uint8(retval);
			d.huffMultibuffer[zig+64*d.coalescedComponent(component)].appendByte(appendedByte);
			//printf("%d bits read as %x => %x => %x\n",
			//    i + 1, code, retval, h.huffmanLUTencoding[retval])
            return uint8E(retval, JpegError::nil());
		}
		code <<= 1;
	}
	return uint8E(0, JpegErrorFormatError("bad Huffman code"));
}

Decoder::uint8E Decoder::decodeBit() {
	uint32E bitErr = decodeBits(1);
    return uint8E(uint8(bitErr.first), bitErr.second);
}

Decoder::uint32E Decoder::decodeBits(int32 n) {
    Decoder &d = *this;
	if (d.arhc) { // here we just return the original value
		uint32E valErr = d.bitbuffer.scanBitsNoStuffedZeros(uint32(n));
		d.wbuffer.emitBits(valErr.first, uint32(n), true);
		return valErr;
	}

	if (d.bits.n < n) {
        JpegError err;
		if ((err = d.ensureNBits(n)) != JpegError::nil()) {
			return uint32E(0, err);
		}
	}
	uint32 ret = d.bits.a >> uint32(d.bits.n-n);
	ret &= (1 << uint32(n)) - 1;
	d.bits.n -= n;
	d.bits.m >>= uint32(n);
	if (ret > 65535) {
		assert(false && "Cannot decode more than 16 bits at a time");
	}
	//fmt.Printf("Writing xraw %x (%d bits)\n", ret, n);
	d.bitbuffer.emitBits(ret, uint32(n), false);
	return uint8E(ret, JpegError::nil());
}
}
