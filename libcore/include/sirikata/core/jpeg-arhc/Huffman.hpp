#ifndef _SIRIKATA_HUFFMAN_HPP_
#define _SIRIKATA_HUFFMAN_HPP_

#include "DecoderPlatform.hpp"
namespace Sirikata {
// maxCodeLength is the maximum (inclusive) number of bits in a Huffman code.
// huffman is a Huffman decoder, specified in section C.
struct huffman {
    enum {
        maxCodeLength = 16,
        
        // maxNCodes is the maximum (inclusive) number of codes in a Huffman tree.
        maxNCodes = 256,
        
        // lutSize is the log-2 size of the Huffman decoder's look-up table.
        lutSize = 8
    };
	// length is the number of codes in the tree.
	int32 nCodes;
	// lut is the look-up table for the next lutSize bits in the bit-stream.
	// The high 8 bits of the uint16 are the encoded value. The low 8 bits
	// are 1 plus the code length, or 0 if the value is too large to fit in
	// lutSize bits.
	uint16 lut [1 << lutSize];
	// vals are the decoded values, sorted by their encoding.
	uint8 vals [maxNCodes];
	// minCodes[i] is the minimum code of length i, or -1 if there are no
	// codes of that length.
	int32 minCodes [maxCodeLength];
	// maxCodes[i] is the maximum code of length i, or -1 if there are no
	// codes of that length.
	int32 maxCodes [maxCodeLength];
	// valsIndices[i] is the index into vals of minCodes[i].
	int32 valsIndices [maxCodeLength];
	// encodingLut allows easy access to re-encode bits decoded by this huffman
	// As in writer.go's huffmanLUT
	// Each value maps to a uint32 of which the 8 most significant bits hold the
	// codeword size in bits and the 24 least significant bits hold the codeword.
	// The maximum codeword size is 16 bits.
	uint32 huffmanLUTencoding [256];
    huffman() {
        memset(this, 0, sizeof(huffman));
    }
};

}
#endif
