#include "Reader.hpp"
#include <zlib.h>
namespace Sirikata{
class SIRIKATA_EXPORT ZlibDecoderDecompressionReader : public DecoderReader {
protected:
    JpegAllocator<uint8_t> mAlloc;
    unsigned char mReadBuffer[4096];
    z_stream mStream;
    DecoderReader *mBase;
    bool mClosed;
    bool mStreamEndEncountered;
public:
    static std::pair<std::vector<uint8_t,
                                 JpegAllocator<uint8_t> >,
                     JpegError> Decompress(const uint8_t *buffer,
                                             size_t size,
                                             const JpegAllocator<uint8_t> &alloc);
    ZlibDecoderDecompressionReader(DecoderReader *r, bool concatenated, const JpegAllocator<uint8_t> &alloc);
    virtual std::pair<uint32, JpegError> Read(uint8*data, unsigned int size);
    virtual ~ZlibDecoderDecompressionReader();
    void Close();
};

class SIRIKATA_EXPORT ZlibDecoderCompressionWriter : public DecoderWriter {
    JpegAllocator<uint8_t> mAlloc;
    unsigned char mWriteBuffer[4096];
    z_stream mStream;
    DecoderWriter *mBase;
    bool mClosed;
public:
    static std::vector<uint8_t, JpegAllocator<uint8_t> > Compress(const uint8_t *buffer,
                                                                  size_t size, const JpegAllocator<uint8_t> &alloc);
    // compresison level should be a value: 1 through 9
    ZlibDecoderCompressionWriter(DecoderWriter *w, uint8_t compression_level, const JpegAllocator<uint8_t> &alloc);
    virtual std::pair<uint32, JpegError> Write(const uint8*data, unsigned int size) ;
    virtual ~ZlibDecoderCompressionWriter();
    virtual void Close();
};
JpegError CompressAnytoZlib(DecoderReader &r, DecoderWriter &w, uint8 compression_level, const JpegAllocator<uint8> &alloc);
// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError DecompressZlibtoAny(DecoderReader &r, DecoderWriter &w, const JpegAllocator<uint8> &alloc);

}
