#include "Decoder.hpp"
#include <lzma.h>
namespace Sirikata {

class SIRIKATA_EXPORT MagicNumberReplacementReader : public DecoderReader {
    uint32 mMagicNumbersReplaced;
    DecoderReader *mBase;
    std::vector<uint8_t, JpegAllocator<uint8> > mOriginalMagic;
    std::vector<uint8_t, JpegAllocator<uint8> > mNewMagic;
public:
    MagicNumberReplacementReader(DecoderReader *r,
                                 const std::vector<uint8_t, JpegAllocator<uint8> >& originalMagic,
                                 const std::vector<uint8_t, JpegAllocator<uint8> >& newMagic);
    virtual std::pair<uint32, JpegError> Read(uint8*data, unsigned int size);
    ~MagicNumberReplacementReader();
};

class SIRIKATA_EXPORT MagicNumberReplacementWriter : public DecoderWriter {
    uint32 mMagicNumbersReplaced;
    DecoderWriter *mBase;
    std::vector<uint8_t, JpegAllocator<uint8> > mOriginalMagic;
    std::vector<uint8_t, JpegAllocator<uint8> > mNewMagic;
public:
    MagicNumberReplacementWriter(DecoderWriter *w,
                                 const std::vector<uint8_t, JpegAllocator<uint8> >& originalMagic,
                                 const std::vector<uint8_t, JpegAllocator<uint8> >& newMagic);
    virtual std::pair<uint32, JpegError> Write(const uint8*data, unsigned int size) ;
    virtual ~MagicNumberReplacementWriter();
    virtual void Close();
};

class SIRIKATA_EXPORT DecoderDecompressionReader : public DecoderReader {
    JpegAllocator<uint8_t> mAlloc;
    unsigned char mReadBuffer[4096];
    lzma_allocator mLzmaAllocator;
    lzma_stream mStream;
    DecoderReader *mBase;
public:
    DecoderDecompressionReader(DecoderReader *r, const JpegAllocator<uint8_t> &alloc);
    virtual std::pair<uint32, JpegError> Read(uint8*data, unsigned int size);
    virtual ~DecoderDecompressionReader();
};
class SIRIKATA_EXPORT DecoderCompressionWriter : public DecoderWriter {
    JpegAllocator<uint8_t> mAlloc;
    lzma_allocator mLzmaAllocator;
    unsigned char mWriteBuffer[4096];
    lzma_stream mStream;
    DecoderWriter *mBase;
    bool mClosed;
public:
    // compresison level should be a value: 1 through 9
    DecoderCompressionWriter(DecoderWriter *w, uint8_t compression_level, const JpegAllocator<uint8_t> &alloc);
    virtual std::pair<uint32, JpegError> Write(const uint8*data, unsigned int size) ;
    virtual ~DecoderCompressionWriter();
    virtual void Close();
};
}
