/*  Sirikata Jpeg Texture Transfer -- Texture Transfer management system
 *  main.cpp
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
