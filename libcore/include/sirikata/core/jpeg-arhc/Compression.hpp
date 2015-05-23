#include "Decoder.hpp"
#include <lzma.h>
namespace Sirikata {
enum {
    MAX_COMPRESSION_THREADS = 16  
};
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

class ThreadContext;

class SIRIKATA_EXPORT DecoderDecompressionMultireader : public DecoderReader {
    ThreadContext * context;
    uint32_t mComponentStart[MAX_COMPRESSION_THREADS]; // if for some reason we can't find the sig, start here
    uint32_t mComponentEnd[MAX_COMPRESSION_THREADS];
    int mNumSuccessfulComponents; // number of components (streams) that have been successfully extracted
    int mNumComponents; // number of components (streams) to extract from this file
    ThreadContext *mWorkers;
    DecoderReader *mReader;
    uint32_t mCurComponentSize;
    uint32_t mCurComponentBytesScanned;
    std::vector<uint8_t, JpegAllocator<uint8_t> > mBuffer; // holds the whole input file (necessary to thread it)
    MemReadWriter mFallbackMemReader;
    DecoderDecompressionReader *mFallbackDecompressionReader;
    std::pair<uint32_t, JpegError> startDecompressionThreads();

public:
    DecoderDecompressionMultireader(DecoderReader *r, ThreadContext * workers, const JpegAllocator<uint8_t> &alloc);
    virtual std::pair<uint32, JpegError> Read(uint8*data, unsigned int size);
    virtual ~DecoderDecompressionMultireader();
};
class SIRIKATA_EXPORT DecoderCompressionMultiwriter : public DecoderWriter {
    DecoderWriter *mWriter;
    ThreadContext *mWorkers;
    std::vector<uint8_t, JpegAllocator<uint8_t> > mBuffer;
    int mCurWriteWorkerId;
    int mNumWorkers;
    int mCompressionLevel;
    bool mClosed;
    bool mReplace7zMagicARHC;
    SizeEstimator *mSizeEstimate;
public:
    // compresison level should be a value: 1 through 9
    DecoderCompressionMultiwriter(DecoderWriter *w, uint8_t compression_level, bool replace_magic,
                                  ThreadContext *workers, const JpegAllocator<uint8_t> &alloc,
                                  SizeEstimator *sizeEstimate); // can pass in NULL if you plan to set it later
    void setEstimatedFileSize(SizeEstimator *sizeEstimate);
    virtual std::pair<uint32, JpegError> Write(const uint8*data, unsigned int size) ;
    virtual ~DecoderCompressionMultiwriter();
    virtual void Close();
};

class SIRIKATA_EXPORT FaultInjectorXZ {
  public:
    virtual bool shouldFault(int offset, int thread, int nThreads) = 0;
    virtual ~FaultInjectorXZ(){}
};

SIRIKATA_FUNCTION_EXPORT JpegError CompressAnyto7Z(DecoderReader &r, DecoderWriter &w,
                                                   uint8 compression_level,
                                                   const JpegAllocator<uint8_t> &alloc);
SIRIKATA_FUNCTION_EXPORT JpegError Decompress7ZtoAny(DecoderReader &r, DecoderWriter &w,
                                                     const JpegAllocator<uint8_t> &alloc);

SIRIKATA_FUNCTION_EXPORT ThreadContext* MakeThreadContext(int nThreads, const JpegAllocator<uint8_t> &alloc);
SIRIKATA_FUNCTION_EXPORT void DestroyThreadContext(ThreadContext *);
SIRIKATA_FUNCTION_EXPORT ThreadContext* TestMakeThreadContext(int nThreads, const JpegAllocator<uint8_t> &alloc, bool depriv, FaultInjectorXZ *fi);

SIRIKATA_FUNCTION_EXPORT JpegError MultiCompressAnyto7Z(DecoderReader &r, DecoderWriter &w,
                                                        uint8 compression_level,
                                                        SizeEstimator *mSizeEstimate,
                                                        ThreadContext *workers);

SIRIKATA_FUNCTION_EXPORT JpegError MultiDecompress7ZtoAny(DecoderReader &r, DecoderWriter &w,
                                                          ThreadContext *workers);

}
