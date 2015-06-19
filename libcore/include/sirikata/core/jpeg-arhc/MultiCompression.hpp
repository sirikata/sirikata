#include "SizeEstimator.hpp"
namespace Sirikata {
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
    DecoderReader *mFallbackDecompressionReader;
    std::pair<uint32_t, JpegError> startDecompressionThreads();
    bool mDoLzham;
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
    uint8_t mOrigLzhamHeader[LZHAM0_HEADER_SIZE];

    bool mDoLzham;

public:
    // compresison level should be a value: 1 through 9
    DecoderCompressionMultiwriter(DecoderWriter *w, uint8_t compression_level, bool replace_magic, bool do_lzham,
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
SIRIKATA_FUNCTION_EXPORT ThreadContext* MakeThreadContext(int nThreads, const JpegAllocator<uint8_t> &alloc, bool enable_lzham = true);
SIRIKATA_FUNCTION_EXPORT void DestroyThreadContext(ThreadContext *);
SIRIKATA_FUNCTION_EXPORT ThreadContext* TestMakeThreadContext(int nThreads, const JpegAllocator<uint8_t> &alloc,
                                                              bool depriv, FaultInjectorXZ *fi, bool enable_lzham = true);

SIRIKATA_FUNCTION_EXPORT JpegError MultiCompressAnyto7Z(DecoderReader &r, DecoderWriter &w,
                                                        uint8 compression_level,
                                                        bool do_lzham,
                                                        SizeEstimator *mSizeEstimate,
                                                        ThreadContext *workers);

SIRIKATA_FUNCTION_EXPORT JpegError MultiDecompress7ZtoAny(DecoderReader &r, DecoderWriter &w,
                                                          ThreadContext *workers);

/*
SIRIKATA_FUNCTION_EXPORT JpegError MultiCompressAnytoLZHAM(DecoderReader &r, DecoderWriter &w,
                                                        uint8 compression_level,
                                                        SizeEstimator *mSizeEstimate,
                                                        ThreadContext *workers);

SIRIKATA_FUNCTION_EXPORT JpegError MultiDecompressLZHAMtoAny(DecoderReader &r, DecoderWriter &w,
                                                             ThreadContext *workers);
*/

}
