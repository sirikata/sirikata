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
namespace Sirikata {

struct TableSelector {
	uint8 compIndex;
	uint8 td; // DC table selector.
	uint8 ta; // AC table selector.
};
// Specified in section B.2.3.
JpegError Decoder::processSOS(int n) {
    Decoder &d = *this;
	if (d.nComp == 0) {
		return FormatError("missing SOF marker");
	}
	if (n < 6 || 4+2*d.nComp < n || n%2 != 0) {
        return FormatError("SOS has wrong length");
	}
    JpegError err;
	if ((err = d.readFull(d.tmp, n)) != JpegError::nil()) {
		return err;
	}
	int nComp = (int)(d.tmp[0]);
	if (n != 4+2*nComp) {
		return FormatError("SOS length inconsistent with number of components");
	}
	TableSelector  scan[nColorComponent];
	
	for (int i = 0; i < nComp; i++) {
		uint8 cs = d.tmp[1+2*i]; // Component selector.
        int32 compIndex = -1;
        for (int j = 0; j < nColorComponent; ++j) {
            component xcomp = d.comp[j];
			if (cs == xcomp.c) {
				compIndex = j;
			}
		}
		if (compIndex < 0) {
			return FormatError("unknown component selector");
		}
		scan[i].compIndex = uint8(compIndex);
		scan[i].td = d.tmp[2+2*i] >> 4;
		if (scan[i].td > maxTh) {
			return FormatError("bad Td value");
		}
		scan[i].ta = d.tmp[2+2*i] & 0x0f;
		if (scan[i].ta > maxTh) {
			return FormatError("bad Ta value");
		}
	}

	// zigStart and zigEnd are the spectral selection bounds.
	// ah and al are the successive approximation high and low values.
	// The spec calls these values Ss, Se, Ah and Al.
	//
	// For (progressive JPEGs, these are the two more-or-less independent
	// aspects of progression. Spectral selection progression is when not
	// all of a block's 64 DCT coefficients are transmitted in one pass.
	// For (example, three passes could transmit coefficient 0 (the DC
	// component), coefficients 1-5, and coefficients 6-63, in zig-zag
	// order. Successive approximation is when not all of the bits of a
	// band of coefficients are transmitted in one pass. For (example,
	// three passes could transmit the 6 most significant bits, followed
	// by the second-least significant bit, followed by the least
	// significant bit.
	//
	// For (baseline JPEGs, these parameters are hard-coded to 0/63/0/0.
	int32 zigStart = 0;
    int32 zigEnd = JpegBlock::blockSize - 1;
    uint32 ah = 0;
    uint32 al = 0;
	if (d.progressive) {
		zigStart = int32(d.tmp[1+2*nComp]);
		zigEnd = int32(d.tmp[2+2*nComp]);
		ah = uint32(d.tmp[3+2*nComp] >> 4);
		al = uint32(d.tmp[3+2*nComp] & 0x0f);
		if ((zigStart == 0 && zigEnd != 0) || zigStart > zigEnd || JpegBlock::blockSize <= zigEnd) {
			return FormatError("bad spectral selection bounds");
		}
		if (zigStart != 0 && nComp != 1) {
			return FormatError("progressive AC coefficients for (more than one component");
		}
		if (ah != 0 && ah != al+1) {
			return FormatError("bad successive approximation values");
		}
	}

	// mxx and myy are the number of MCUs (Minimum Coded Units) in the image.
	int h0 = d.comp[0].h;
    int v0 = d.comp[0].v; // The h and v values from the Y components.
    int mxx = (d.width + 8*h0 - 1) / (8 * h0);
    int myy = (d.height + 8*v0 - 1) / (8 * v0);
	if (d.progressive) {
		for (int i = 0; i < nComp; i++) {
			uint8 compIndex = scan[i].compIndex;
			if (d.progCoeffs[compIndex].empty()) {
				d.progCoeffs[compIndex].reserve(mxx*myy*d.comp[compIndex].h*d.comp[compIndex].v);
			}
		}
	}

	d.bits = Bits();
	int mcu = 0;
    uint8 expectedRST = uint8(rst0Marker);
    JpegBlock b;
    int32 dc[nColorComponent] = {0};
    int bx = 0;
    int by = 0;
    int blockCount = 0;
    for (int my = 0; my < myy; my++) {
        for (int mx = 0; mx < mxx; mx++) {
			d.huffTransSect = true;
			for (int i = 0; i < nComp; i++) {
                uint8 compIndex = scan[i].compIndex;
                JpegBlock *qt = &d.quant[d.comp[compIndex].tq];
				for (int j = 0; j < d.comp[compIndex].h*d.comp[compIndex].v; j++) {
					// The blocks are traversed one MCU at a time. For (4:2:0 chroma
					// subsampling, there are four Y 8x8 blocks in every 16x16 MCU.
					//
					// For (a baseline 32x16 pixel image, the Y blocks visiting order is:
					//	0 1 4 5
					//	2 3 6 7
					//
					// For (progressive images, the interleaved scans (those with nComp > 1)
					// are traversed as above, but non-interleaved scans are traversed left
					// to right, top to bottom:
					//	0 1 2 3
					//	4 5 6 7
					// Only DC scans (zigStart == 0) can be interleaved. AC scans must have
					// only one component.
					//
					// To further complicate matters, for (non-interleaved scans, there is no
					// data for (any blocks that are inside the image at the MCU level but
					// outside the image at the pixel level. For (example, a 24x16 pixel 4:2:0
					// progressive image consists of two 16x16 MCUs. The interleaved scans
					// will process 8 Y blocks:
					//	0 1 4 5
					//	2 3 6 7
					// The non-interleaved scans will process only 6 Y blocks:
					//	0 1 2
					//	3 4 5
					if (nComp != 1) {
						bx = d.comp[compIndex].h*mx;
                        by = d.comp[compIndex].v*my;
						if (h0 == 1) {
                            by += j;
						} else {
							bx += j % 2;
							by += j / 2;
						}
					} else {
                        int q = mxx * d.comp[compIndex].h;
						bx = blockCount % q;
						by = blockCount / q;
						blockCount++;
                        if (bx*8 >= d.width || by*8 >= d.height) {
							continue;
						}
					}

					// Load the previous partially decoded coefficients, if (applicable.
					if (d.progressive) {
						b = d.progCoeffs[compIndex][by*mxx*d.comp[compIndex].h+bx];
					} else {
						b = JpegBlock();
					}
					if (ah != 0) {
                        JpegError err;
						if ((err = d.refine(&b, &d.huff[acTable][scan[i].ta], zigStart, zigEnd, 1<<al, i)) != JpegError::nil()) {
                            d.wbuffer.flushBits(true);
							return err;
						}
					} else {
						int zig = zigStart;
                        if (zig == 0) {
                            zig++;
							// Decode the DC coefficient, as specified in section F.2.2.1.
                            std::pair<uint8, JpegError>valueErr = d.decodeHuffman(&d.huff[dcTable][scan[i].td], zig-1, i);
							if (valueErr.second != JpegError::nil()) {
                                d.wbuffer.flushBits(true);
								return valueErr.second;
							}
							if (valueErr.first > 16) {
                                d.wbuffer.flushBits(true);
								return UnsupportedError("excessive DC component");
							}
							std::pair<int32, JpegError>dcDeltaErr = d.receiveExtend(valueErr.first);
							if (dcDeltaErr.second != JpegError::nil()) {
                                d.wbuffer.flushBits(true);
								return dcDeltaErr.second;
							}
							dc[compIndex] += dcDeltaErr.first;
							b[0] = dc[compIndex] << al;
						}

						if (zig <= zigEnd && d.eobRun > 0) {
							d.eobRun--;
						} else {
							// Decode the AC coefficients, as specified in section F.2.2.2.
                            huffman *huff = &d.huff[acTable][scan[i].ta];
							for (; zig <= zigEnd; zig++) {
                                uint8E valueErr = d.decodeHuffman(huff, zig, i /*component*/);
								if (valueErr.second != JpegError::nil()) {
                                    d.wbuffer.flushBits(true);
									return valueErr.second;
								}
                                uint8 val0 = valueErr.first >> 4;
                                uint8 val1 = valueErr.first & 0x0f;
								if (val1 != 0) {
                                    zig += int32(val0);
									if (zig > zigEnd) {
										break;
									}
									int32E acErr = d.receiveExtend(val1);
									if (acErr.second != JpegError::nil()) {
                                        d.wbuffer.flushBits(true);
										return acErr.second;
									}
									b[unzig()[zig]] = acErr.first << al;
								} else {
									if (val0 != 0x0f) {
										d.eobRun = uint16(1 << val0);
										if (val0 != 0) {
											uint32E bitsErr = d.decodeBits(int32(val0));
											if (bitsErr.second != JpegError::nil()) {
                                                    d.wbuffer.flushBits(true);
                                                    return bitsErr.second;
											}
                                            d.eobRun |= (uint16)bitsErr.first;
										}
										d.eobRun--;
										break;
									}
									zig += 0x0f;
								}
							}
						}
					}

					if (d.progressive) {
						if (zigEnd != JpegBlock::blockSize-1 || al != 0) {
							// We haven't completely decoded this 8x8 block. Save the coefficients.
							d.progCoeffs[compIndex][by*mxx*d.comp[compIndex].h+bx] = b;
							// At this point, we could execute the rest of the loop body to dequantize and
							// perform the inverse DCT, to save early stages of a progressive image to the
							// *image.YCbCr buffers (the whole point of progressive encoding), but in Go,
							// the jpeg.Decode function does not return until the entire image is decoded,
							// so we "continue" here to avoid wasted computation.
							continue;
						}
					}
/*
#if 0
					// Dequantize, perform the inverse DCT and store the block to the image.
					for (int zig = 0; zig < JpegBlock::blockSize; zig++) {
						b[unzig()[zig]] *= qt[zig];
					}
					idct(&b);
                    dst, stride := []byte(nil), 0)
					if (d.nComp == nGrayComponent) {
						dst, stride = d.img1.Pix[8*(by*d.img1.Stride+bx):], d.img1.Stride
					} else {
						switch compIndex {
						case 0:
							dst, stride = d.img3.Y[8*(by*d.img3.YStride+bx):], d.img3.YStride
						case 1:
							dst, stride = d.img3.Cb[8*(by*d.img3.CStride+bx):], d.img3.CStride
						case 2:
							dst, stride = d.img3.Cr[8*(by*d.img3.CStride+bx):], d.img3.CStride
						default:
                            d.wbuffer.flushBits(true);
							return UnsupportedError("too many components")
						}
					}
					// Level shift by +128, clip to [0, 255], and write to dst.
					for (y := 0; y < 8; y++) {
						y8 := y * 8
						yStride := y * stride
						for (x := 0; x < 8; x++) {
							c := b[y8+x]
							if (c < -128) {
								c = 0
							} else if (c > 127) {
								c = 255
							} else {
								c += 128
							}
							dst[yStride+x] = uint8(c)
						}
					}
#endif
*/
				} // for (j
			} // for (i
            d.huffTransSect = false;
            mcu++;
            if (d.ri > 0 && mcu%d.ri == 0 && mcu < mxx*myy) {
				// if (any bits are outstanding at this time, we must flush them now
                d.wbuffer.flushBits(true);
                // A more sophisticated decoder could use RST[0-7] markers to resynchronize from corrupt input,
                // but this one assumes well-formed input, and hence the restart marker follows immediately.
                if ((err = d.readFull(d.tmp, 2)) != JpegError::nil()) {
                    d.wbuffer.flushBits(true);
                    return err;
                }
                //fprintf(stderr, "Expected Restart marker %x\n", d.tmp);
                if (d.tmp[0] != 0xff || d.tmp[1] != expectedRST) {
                    d.wbuffer.flushBits(true);
                    return FormatError("bad RST marker");
                }
                expectedRST++;
                if (expectedRST == rst7Marker+1) {
                    expectedRST = rst0Marker;
                }
                // Reset the Huffman decoder.
                d.bits = Bits();
                // Reset the DC components, as per section F.2.1.3.1.
                memset(dc, 0, sizeof(int32) * nColorComponent);
                // Reset the progressive decoder state, as per section G.1.2.2.
                d.eobRun = 0;
            }
        } // for (mx
	} // for (my
    d.wbuffer.flushBits(true);
	return JpegError::nil();
}

// refine decodes a successive approximation refinement block, as specified in
// section G.1.2.
JpegError Decoder::refine(JpegBlock* b, huffman* h, int32 zigStart, int32 zigEnd, int32 delta, int component) {
    Decoder & d = *this;
	// Refining a DC component is trivial.
	if (zigStart == 0) {
		if (zigEnd != 0) {
			assert(false && "unreachable");
		}
		uint32E bitErr = d.decodeBit();
		if (bitErr.second != JpegError::nil()) {
			return bitErr.second;
		}
        if (bitErr.first) {
			(*b)[0] |= delta;
		}
		return JpegError::nil();
	}

	// Refining AC components is more complicated; see sections G.1.2.2 and G.1.2.3.
    int32 zig = zigStart;
	if (d.eobRun == 0) {
		for (; zig <= zigEnd; zig++) {
            int32 z = 0;
			uint8E valueErr = d.decodeHuffman(h, zig, component);
            if (valueErr.second != JpegError::nil()) {
				return valueErr.second;
			}
            uint8 val0 = valueErr.first >> 4;
			uint8 val1 = valueErr.first & 0x0f;

			if (val1 == 0) {
              if (val0 != 0x0f) {
                  d.eobRun = uint16(1 << val0);
                  if (val0 != 0) {
                      uint32E bitsErr = d.decodeBits(int32(val0));
                      if (bitsErr.second != JpegError::nil()) {
                          return bitsErr.second;
                      }
                      d.eobRun |= (uint16)(bitsErr.first);
                  }
                  break;
              }
            } else if (val1 == 1) {
              z = delta;
              {
                  uint8E bitErr = d.decodeBit();
                  if (bitErr.second != JpegError::nil()) {
                      return bitErr.second;
                  }
                  if (!bitErr.first) {
                      z = -z;
                  }
              }
            } else {
              return FormatError("unexpected Huffman code");
			}
			int32E zigErr = d.refineNonZeroes(b, zig, zigEnd, int32(val0), delta);
			if (zigErr.second != JpegError::nil()) {
				return zigErr.second;
			}
            zig = zigErr.first;
			if (zig > zigEnd) {
				return FormatError("too many coefficients");
			}
			if (z != 0) {
                (*b)[unzig()[zig]] = z;
			}
		}
	}
	if (d.eobRun > 0) {
		d.eobRun--;
        int32E zigErr;
		if ((zigErr = d.refineNonZeroes(b, zig, zigEnd, -1, delta)).second != JpegError::nil()) {
			return zigErr.second;
		}
	}
	return JpegError::nil();
}

// refineNonZeroes refines non-zero entries of b in zig-zag order. If (nz >= 0,
// the first nz zero entries are skipped over.
std::pair<int32, JpegError> Decoder::refineNonZeroes(JpegBlock *b, int32 zig, int32 zigEnd, int32 nz, int32 delta) {
    Decoder &d = *this;
	for (; zig <= zigEnd; zig++) {
        int u = unzig()[zig];
		if ((*b)[u] == 0) {
            if (nz == 0) {
				break;
			}
			nz--;
            continue;
		}
		uint8E bitErr = d.decodeBit();
		if (bitErr.second != JpegError::nil()) {
            return uint32E(0, bitErr.second);
		}
		if (!bitErr.first) {
			continue;
		}
		if ((*b)[u] >= 0) {
			(*b)[u] += delta;
		} else {
			(*b)[u] -= delta;
		}
	}
	return int32E(zig, JpegError::nil());
}
}
