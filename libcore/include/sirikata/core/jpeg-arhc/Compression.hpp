#include "Decoder.hpp"
#include <lzma.h>
namespace Sirikata {

class SIRIKATA_EXPORT MagicNumberReplacementReader : public DecoderReader {
    uint32 mMagicNumbersReplaced;
    DecoderReader *mBase;
    String mOriginalMagic;
    String mNewMagic;
public:
    MagicNumberReplacementReader(DecoderReader *r, const String & originalMagic, const String& newMagic);
    virtual std::pair<uint32, JpegError> Read(uint8*data, unsigned int size);
    ~MagicNumberReplacementReader();
};

class SIRIKATA_EXPORT MagicNumberReplacementWriter : public DecoderWriter {
    uint32 mMagicNumbersReplaced;
    DecoderWriter *mBase;
    String mOriginalMagic;
    String mNewMagic;
public:
    MagicNumberReplacementWriter(DecoderWriter *w, const String & originalMagic, const String& newMagic);
    virtual std::pair<uint32, JpegError> Write(const uint8*data, unsigned int size) ;
    virtual ~MagicNumberReplacementWriter();
    virtual void Close();
};

class SIRIKATA_EXPORT DecoderDecompressionReader : public DecoderReader {
    unsigned char mReadBuffer[4096];
    lzma_stream mStream;
    DecoderReader *mBase;
public:
    DecoderDecompressionReader(DecoderReader *r);
    virtual std::pair<uint32, JpegError> Read(uint8*data, unsigned int size);
    virtual ~DecoderDecompressionReader();
};
class SIRIKATA_EXPORT DecoderCompressionWriter : public DecoderWriter {
    unsigned char mWriteBuffer[4096];
    lzma_stream mStream;
    DecoderWriter *mBase;
    bool mClosed;
public:
    DecoderCompressionWriter(DecoderWriter *w);
    virtual std::pair<uint32, JpegError> Write(const uint8*data, unsigned int size) ;
    virtual ~DecoderCompressionWriter();
    virtual void Close();
};
}
