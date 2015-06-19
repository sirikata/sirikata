namespace Sirikata {
class ThreadContext {
    int thread_output_write[MAX_COMPRESSION_THREADS];

    int thread_input_read[MAX_COMPRESSION_THREADS];
    lzham_compress_state_ptr mLzhamCompressState[MAX_COMPRESSION_THREADS];
    lzham_decompress_state_ptr mLzhamDecompressState[MAX_COMPRESSION_THREADS];
    JpegAllocator<uint8_t> mAllocator;
    // This helps test for the case that the decompressor
    // runs into problems with an aligend, poorly placed 7zXZ in the file
    // and needs to run in single threaded mode for that file
    FaultInjectorXZ *mTestFaults;
public:
    enum WorkType {
        TERM,
        ACKTERM,
        PING,
        PONG,
        DEPRIVILEGE,
        ACKDEPRIVILEGE,
        NACKDEPRIVILEGE,
        COMPRESS1,
        COMPRESS2,
        COMPRESS3,
        COMPRESS4,
        COMPRESS5,
        COMPRESS6,
        COMPRESS7,
        COMPRESS8,
        COMPRESS9,
        DECOMPRESS,
        LZHAMCOMPRESS0,
        LZHAMCOMPRESS1,
        LZHAMCOMPRESS2,
        LZHAMCOMPRESS3,
        LZHAMCOMPRESS4,
        XZ_OK,
        XZ_FAIL
    };
private:
    void compress(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                  int level,
                  int worker_id);
    void compressLZHAM(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                  int level,
                       int worker_id);
    void decompressLZHAM(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                         int worker_id);
    void send_xz_fail(int output_fd);
    void decompress(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                    int worker_id);
    void worker(size_t worker_id, JpegAllocator<uint8_t> alloc);
public:
    int thread_output_read[MAX_COMPRESSION_THREADS];
    int thread_input_write[MAX_COMPRESSION_THREADS];
    size_t num_threads;
    size_t numThreads() const {
        return num_threads;
    }
    ~ThreadContext();
    ThreadContext(size_t num_threads, const JpegAllocator<uint8_t> & alloc, bool depriv, FaultInjectorXZ *faultInjection);
    void initializeLzham(int nthreads);
    const JpegAllocator<uint8_t> &get_allocator() const {
        return mAllocator;
    }
};
}
