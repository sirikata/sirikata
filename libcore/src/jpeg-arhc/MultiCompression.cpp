#include <assert.h>
#include <string.h>

#include <sirikata/core/jpeg-arhc/Compression.hpp>
#include <sirikata/core/jpeg-arhc/MultiCompression.hpp>
#include <sirikata/core/jpeg-arhc/Decoder.hpp>

#include <lzma.h>
#if __GXX_EXPERIMENTAL_CXX0X__ || __cplusplus > 199711L
#include <thread>
#define USE_BUILTIN_THREADS
#define USE_BUILTIN_ATOMICS
#else
#include <sirikata/core/util/Thread.hpp>
#endif
#ifdef HAS_LZHAM
#include <lzham.h>
#endif

#ifdef __linux
#include <sys/wait.h>
#include <linux/seccomp.h>

#include <sys/prctl.h>
#include <sys/syscall.h>
#endif

#define THREAD_COMMAND_BUFFER_SIZE 5
//#define LZHAMTEST_DEFAULT_DICT_SIZE 28
#define LZHAMTEST_DEFAULT_DICT_SIZE 24
#include <sirikata/core/jpeg-arhc/ThreadContext.hpp>

// 3 headers also defined in Compression.cpp
static const unsigned char lzham_fixed_header[] = {'L','Z','H','0'};
static const unsigned char xzheader[6] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };
static const unsigned char arhcheader[6] = { 'A', 'R', 'H', 'C', 0x01, 0x00 };
static const unsigned char arhclzhamheader[6] = { 'A', 'R', 'H', 'C', 0x01, 0x01 };

namespace Sirikata {
namespace {
static ssize_t read_at_least(int fd, void *buf, size_t size, size_t at_least) {
    size_t progress = 0;
    while (progress < at_least) {
        ssize_t status = read(fd, (char*)buf + progress, size - progress);
        if (status == 0) { // EOF
            return 0;
        }
        if (status == -1) {
            if (errno != EINTR) {
                return -1;
            }
        } else {
            progress += status;
        }
    }
    return progress;
}

static ssize_t read_full(int fd, void *buf, size_t size) {
    return read_at_least(fd, buf, size, size);
}

static ssize_t write_full(int fd, const void *buf, size_t size) {
    size_t progress = 0;
    while (progress < size) {
        ssize_t status = write(fd, (char*)buf + progress, size - progress);
        if (status == -1) {
            if (status == 0) { // EOF
                return 0;
            }
            if (errno != EINTR) {
                return -1;
            }
        } else {
            progress += status;
        }
    }
    return progress;
}
uint32 LEtoUint32(const uint8*buffer) {
    uint32 retval = buffer[3];
    retval <<=8;
    retval |= buffer[2];
    retval <<= 8;
    retval |= buffer[1];
    retval <<= 8;
    retval |= buffer[0];
    return retval;
}
void uint32toLE(uint32 value, uint8*retval) {
    retval[0] = uint8(value & 0xff);
    retval[1] = uint8((value >> 8) & 0xff);
    retval[2] = uint8((value >> 16) & 0xff);
    retval[3] = uint8((value >> 24) & 0xff);
}
}
void ThreadContext::compress(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                  int level,
                  int worker_id) {
        std::vector<uint8_t, JpegAllocator<uint8_t> > compressed_data(data.get_allocator());
        compressed_data.resize(lzma_stream_buffer_bound(data.size()) + THREAD_COMMAND_BUFFER_SIZE);
        lzma_allocator lzma_alloc;
        lzma_alloc.alloc = data.get_allocator().get_custom_allocate();
        lzma_alloc.free = data.get_allocator().get_custom_deallocate();
        lzma_alloc.opaque = data.get_allocator().get_custom_state();
        size_t out_pos = 0;
        lzma_ret ret = lzma_easy_buffer_encode(level, LZMA_CHECK_NONE, &lzma_alloc,
                                               data.empty()?NULL:&data[0],
                                               data.size(),
                                               &compressed_data[THREAD_COMMAND_BUFFER_SIZE],
                                               &out_pos,
                                               compressed_data.size() - THREAD_COMMAND_BUFFER_SIZE);
        if (ret == LZMA_OK) {
            compressed_data.resize(out_pos + THREAD_COMMAND_BUFFER_SIZE);
            compressed_data[0] = XZ_OK;
            uint32toLE(out_pos, &compressed_data[1]);
            write_full(thread_output_write[worker_id], &compressed_data[0], compressed_data.size());
        } else {
            assert(ret != LZMA_BUF_ERROR && "Buffer space should have been sufficient");
            assert(ret != LZMA_UNSUPPORTED_CHECK && "No check should be supported");
            assert(ret != LZMA_OPTIONS_ERROR && "Options should be supported");
            assert(ret != LZMA_MEM_ERROR && "Memory bug?");
            assert(ret != LZMA_DATA_ERROR && "Early Memory bug?");
            assert(ret != LZMA_PROG_ERROR && "Early LZMA bug?");
            send_xz_fail(thread_output_write[worker_id]);
        }
    }
void ThreadContext::compressLZHAM(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                  int level,
                  int worker_id) {
        std::vector<uint8_t, JpegAllocator<uint8_t> > compressed_data(data.get_allocator());
        compressed_data.resize(lzham_z_compressBound(data.size()) + LZHAM0_HEADER_SIZE - THREAD_COMMAND_BUFFER_SIZE);
        size_t out_read = compressed_data.size() - LZHAM0_HEADER_SIZE - THREAD_COMMAND_BUFFER_SIZE;
        size_t in_read = data.size();
        lzham_compress_status_t ret = LZHAM_COMP_STATUS_NOT_FINISHED;
        size_t ooffset = LZHAM0_HEADER_SIZE + THREAD_COMMAND_BUFFER_SIZE;
        size_t ioffset = 0;
        do {
            ret = lzham_compress(mLzhamCompressState[worker_id],
                                 &data[ioffset], &in_read,
                                 &compressed_data[ooffset], &out_read,
                                 true);
            ioffset += in_read;
            in_read = data.size() - ioffset;
            ooffset += out_read;
            out_read = compressed_data.size() - ooffset;
        } while(ret == LZHAM_COMP_STATUS_NOT_FINISHED);
        if (ret == LZHAM_COMP_STATUS_SUCCESS) {
            compressed_data.resize(ooffset);
            compressed_data[0] = XZ_OK;
            uint32toLE(ooffset - THREAD_COMMAND_BUFFER_SIZE, &compressed_data[1]);
            writeLZHAMHeader(&compressed_data[THREAD_COMMAND_BUFFER_SIZE], LZHAMTEST_DEFAULT_DICT_SIZE, data.size());
            write_full(thread_output_write[worker_id], &compressed_data[0], compressed_data.size());
        } else {
            assert(ret != LZHAM_COMP_STATUS_OUTPUT_BUF_TOO_SMALL && "Output buf too small");
            assert(ret != LZHAM_COMP_STATUS_INVALID_PARAMETER && "Invalid parameter");
            assert(ret != LZHAM_COMP_STATUS_FAILED_INITIALIZING && "Comp status failed to initialize");
            assert(ret != LZHAM_COMP_STATUS_FAILED && "Comp status failed");
            assert(ret != LZHAM_COMP_STATUS_NOT_FINISHED && "Comp status not finished");
            assert(ret != LZHAM_COMP_STATUS_NEEDS_MORE_INPUT && "Comp status needs more input");
            assert(ret != LZHAM_COMP_STATUS_HAS_MORE_OUTPUT && "Comp status needs more output room");
            assert("Unreachable");
            send_xz_fail(thread_output_write[worker_id]);
        }
}
extern std::pair<lzham_decompress_params, JpegError> makeLZHAMDecodeParams(const uint8 header[LZHAM0_HEADER_SIZE]);
void ThreadContext::decompressLZHAM(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                          int worker_id) {
        std::vector<uint8_t, JpegAllocator<uint8_t> > decompressed_data(data.get_allocator());
        decompressed_data.resize(THREAD_COMMAND_BUFFER_SIZE); // save room to return the buffer headers
        uint32 inputDataOffset = 0;
        while(true) {
            std::pair<lzham_decompress_params, JpegError> p = makeLZHAMDecodeParams(&data[inputDataOffset]);
            if (p.second != JpegError::nil()) {
                send_xz_fail(thread_output_write[worker_id]);                
                return;
            }
            p.first.m_decompress_flags = LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED;
            uint32 outputDataSize = LEtoUint32(&data[inputDataOffset + 5]);
            uint32 outputDataOffset = decompressed_data.size();
            decompressed_data.resize(outputDataSize + outputDataOffset);
            if (mLzhamDecompressState[worker_id] == NULL) {
                mLzhamDecompressState[worker_id] = lzham_decompress_init(&p.first);
            } else{
                mLzhamDecompressState[worker_id] = lzham_decompress_reinit(mLzhamDecompressState[worker_id], &p.first);
            }

            inputDataOffset += LZHAM0_HEADER_SIZE;

            size_t in_read = data.size() - inputDataOffset;
            size_t out_write = outputDataSize;
            lzham_decompress_status_t ret;
            do {
                ret = lzham_decompress(mLzhamDecompressState[worker_id],
                                      &data[inputDataOffset], &in_read,
                                      &decompressed_data[outputDataOffset], &out_write,
                                      true);
                inputDataOffset += in_read;
                outputDataOffset += out_write;
                outputDataSize -= out_write;
                in_read = data.size() - inputDataOffset;
                out_write = outputDataSize;
            } while (ret == LZHAM_DECOMP_STATUS_NOT_FINISHED);
            if (ret != LZHAM_DECOMP_STATUS_SUCCESS) {
                assert(ret != LZHAM_DECOMP_STATUS_NOT_FINISHED && "Status not finished");

                assert(ret != LZHAM_DECOMP_STATUS_HAS_MORE_OUTPUT && "Has more output");

                assert(ret != LZHAM_DECOMP_STATUS_NEEDS_MORE_INPUT && "Needs more input");

                assert(ret != LZHAM_DECOMP_STATUS_FAILED_INITIALIZING && "Failed initializing");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_DEST_BUF_TOO_SMALL && "Dest buf too small");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_EXPECTED_MORE_RAW_BYTES && "Expected more raw bytes");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_BAD_CODE && "bad code");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_ADLER32 && "failed adler32");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_BAD_RAW_BLOCK && "bad raw block");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_BAD_COMP_BLOCK_SYNC_CHECK && "Bad comp sync check");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_BAD_ZLIB_HEADER && "Bad zlib header");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_NEED_SEED_BYTES && "Need seed");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_BAD_SEED_BYTES && "Bad seed");
                assert(ret != LZHAM_DECOMP_STATUS_FAILED_BAD_SYNC_BLOCK && "Bad sync block");
                assert(ret != LZHAM_DECOMP_STATUS_INVALID_PARAMETER && "Invalid parameter");
                return;
            }
            if (inputDataOffset == data.size()) {
                break;
            }
        }
        decompressed_data[0] = XZ_OK;
        uint32toLE(decompressed_data.size() - THREAD_COMMAND_BUFFER_SIZE, &decompressed_data[1]);
        write_full(thread_output_write[worker_id], &decompressed_data[0], decompressed_data.size());
    }
void ThreadContext::send_xz_fail(int output_fd) {
        uint8 failure_message[THREAD_COMMAND_BUFFER_SIZE] = {0};
        failure_message[0] = XZ_FAIL;
        write_full(output_fd, failure_message, sizeof(failure_message));
    }
void ThreadContext::decompress(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
                    int worker_id) {
        std::vector<uint8_t, JpegAllocator<uint8_t> > output_data(data.get_allocator());
        output_data.reserve(data.size()); // it should be at least as big as the input
        output_data.resize(THREAD_COMMAND_BUFFER_SIZE);
        lzma_stream stream = LZMA_STREAM_INIT;
        lzma_allocator lzma_alloc;
        memset(&lzma_alloc, 0, sizeof(lzma_allocator));
        lzma_alloc.alloc = data.get_allocator().get_custom_allocate();
        lzma_alloc.free = data.get_allocator().get_custom_deallocate();
        lzma_alloc.opaque = data.get_allocator().get_custom_state();
        stream.allocator = &lzma_alloc;
        lzma_ret ret = lzma_stream_decoder(
			&stream, UINT64_MAX, LZMA_CONCATENATED);
        if (ret != LZMA_OK) {
            send_xz_fail(thread_output_write[worker_id]);
            return;
        }
        uint8_t buf[16384];
        stream.next_in = &data[0];
        stream.avail_in = data.size();
        int decompress_counter = 0;
        do {
            stream.next_out = buf;
            stream.avail_out = sizeof(buf);
            ret = lzma_code(&stream, LZMA_FINISH);
            output_data.insert(output_data.end(), buf, buf + sizeof(buf) - stream.avail_out);
            if (mTestFaults && mTestFaults->shouldFault(decompress_counter, worker_id, num_threads)) {
                ret = LZMA_DATA_ERROR;
            }
            if (ret == LZMA_STREAM_END) {
                break;
            } else if (ret != LZMA_OK) {
                send_xz_fail(thread_output_write[worker_id]);
                return;
            }
        }while(++decompress_counter);
        output_data[0] = XZ_OK;
        uint32toLE(output_data.size() - THREAD_COMMAND_BUFFER_SIZE, &output_data[1]);
        write_full(thread_output_write[worker_id], &output_data[0], output_data.size());
    }
void ThreadContext::worker(size_t worker_id, JpegAllocator<uint8_t> alloc) {
        if (0){
            if (mAllocator.get_custom_reallocate() && mAllocator.get_custom_msize()) {
                lzham_set_memory_callbacks(mAllocator.get_custom_reallocate(),
                                           mAllocator.get_custom_msize(),
                                           mAllocator.get_custom_state());
            }

            lzham_decompress_params p;
            memset(&p, 0, sizeof(p));
            p.m_struct_size = sizeof(lzham_decompress_params);
            p.m_dict_size_log2 = LZHAMTEST_DEFAULT_DICT_SIZE;
            p.m_decompress_flags = LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED;
            mLzhamDecompressState[worker_id] = lzham_decompress_init(&p);

            lzham_compress_params q;
            memset(&q, 0, sizeof(q));
            q.m_struct_size = sizeof(lzham_compress_params);
            q.m_dict_size_log2 = LZHAMTEST_DEFAULT_DICT_SIZE;

            mLzhamCompressState[worker_id] = lzham_compress_init(&q);

        }
        unsigned char command[THREAD_COMMAND_BUFFER_SIZE] = {0};
        std::vector<uint8_t, JpegAllocator<uint8_t> > command_data(alloc);
        while (read_full(thread_input_read[worker_id], command, sizeof(command)) == sizeof(command)) {
            uint32_t command_size = LEtoUint32(command + 1);
            if (command_size > 0) {
                command_data.resize(command_size);
                if (!read_full(thread_input_read[worker_id], &command_data[0], command_size) == command_size) {
                    break;
                }
            }
            switch(command[0]) {
              case TERM:
                memset(command, 0, sizeof(command));
                command[0] = ACKTERM;
                write_full(thread_output_write[worker_id], command, sizeof(command));
                syscall(SYS_exit, 1); // this only exits the thread
                return;
              case PING:
                memset(command, 0, sizeof(command));
                command[0] = PONG;
                write_full(thread_output_write[worker_id], command, sizeof(command));
                break;
              case DEPRIVILEGE:
                memset(command, 0, sizeof(command));
                command[0] = NACKDEPRIVILEGE;
#ifdef __linux
                if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT)) {
                    syscall(SYS_exit, 1); // SECCOMP not allowed
                }
                command[0] = ACKDEPRIVILEGE;
#endif
                write_full(thread_output_write[worker_id], command, sizeof(command));
                break;
              case COMPRESS1:
              case COMPRESS2:
              case COMPRESS3:
              case COMPRESS4:
              case COMPRESS5:
              case COMPRESS6:
              case COMPRESS7:
              case COMPRESS8:
              case COMPRESS9:
                compress(command_data, command[0] - COMPRESS1 + 1, worker_id);
                break;
              case DECOMPRESS:
                if (command_data.size() > LZHAM0_HEADER_SIZE
                    && makeLZHAMDecodeParams(&command_data[0]).second == JpegError::nil()) {
                    decompressLZHAM(command_data, worker_id);
                } else {
                    decompress(command_data, worker_id);
                }
                break;
              case LZHAMCOMPRESS0:
              case LZHAMCOMPRESS1:
              case LZHAMCOMPRESS2:
              case LZHAMCOMPRESS3:
              case LZHAMCOMPRESS4:
                compressLZHAM(command_data, command[0] - LZHAMCOMPRESS0, worker_id);
                break;
                
            }
        }
        
    }


ThreadContext::~ThreadContext() {
        for (size_t i = 0; i < num_threads; ++i) {
            uint8_t deprivilege_command[THREAD_COMMAND_BUFFER_SIZE] = {TERM, 0, 0, 0, 0};
            write_full(thread_input_write[i], deprivilege_command, sizeof(deprivilege_command));
        }
        for (size_t i = 0; i < num_threads; ++i) {
            uint8_t return_buffer[THREAD_COMMAND_BUFFER_SIZE] = {0};
            read_full(thread_output_read[i], return_buffer, sizeof(return_buffer));
            assert(return_buffer[0] == ACKTERM);
            assert(LEtoUint32(return_buffer + 1) == 0);
        }
    }
ThreadContext::ThreadContext(size_t num_threads, const JpegAllocator<uint8_t> & alloc, bool depriv, FaultInjectorXZ *faultInjection) : mAllocator(alloc) {
        mTestFaults = faultInjection;
        memset(mLzhamDecompressState, 0, sizeof(mLzhamDecompressState));
        memset(mLzhamCompressState, 0, sizeof(mLzhamCompressState));
        memset(thread_output_write, 0, sizeof(thread_output_write));
        memset(thread_output_read, 0, sizeof(thread_output_read));
        memset(thread_input_write, 0, sizeof(thread_input_write));
        memset(thread_input_read, 0, sizeof(thread_input_read));
        assert(num_threads <= MAX_COMPRESSION_THREADS);
        if (num_threads > MAX_COMPRESSION_THREADS) {
            num_threads = MAX_COMPRESSION_THREADS; // for those who built -DNDEBUG
        }
        this->num_threads = num_threads;
        for (size_t i = 0; i < num_threads; ++i) {
            int pipefd[2];
            int pipe_retval = pipe2(pipefd, 0);
            assert(pipe_retval == 0);
            thread_output_read[i] = pipefd[0];
            thread_output_write[i] = pipefd[1];
            pipe_retval = pipe2(pipefd, 0);
            assert(pipe_retval == 0);        
            thread_input_read[i] = pipefd[0];
            thread_input_write[i] = pipefd[1];
#ifdef USE_BUILTIN_THREADS
            std::thread t(&ThreadContext::worker, this, i, alloc);
#else
            Sirikata::Thread t("CompressionWorker", std::tr1::bind(&ThreadContext::worker, this, i, alloc));
#endif
            uint8_t deprivilege_command[THREAD_COMMAND_BUFFER_SIZE] = {uint8(depriv?DEPRIVILEGE:PING), 0, 0, 0, 0};
            write_full(thread_input_write[i], deprivilege_command, sizeof(deprivilege_command));
            t.detach();
        }
        for (size_t i = 0; i < num_threads; ++i) {
            uint8_t return_buffer[THREAD_COMMAND_BUFFER_SIZE] = {0};
            read_full(thread_output_read[i], return_buffer, sizeof(return_buffer));
            assert(return_buffer[0] == uint8(depriv?ACKDEPRIVILEGE:PONG));
            assert(LEtoUint32(return_buffer + 1) == 0);
        }
    }
void ThreadContext::initializeLzham(int nthreads) {
        if (mAllocator.get_custom_reallocate() && mAllocator.get_custom_msize()) {
            lzham_set_memory_callbacks(mAllocator.get_custom_reallocate(),
                                       mAllocator.get_custom_msize(),
                                       mAllocator.get_custom_state());
        }
 
        for (int worker_id = 0; worker_id < nthreads; ++worker_id) {

            lzham_decompress_params p;
            memset(&p, 0, sizeof(p));
            p.m_struct_size = sizeof(lzham_decompress_params);
            p.m_dict_size_log2 = LZHAMTEST_DEFAULT_DICT_SIZE;
            p.m_decompress_flags = LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED;
            mLzhamDecompressState[worker_id] = lzham_decompress_init(&p);

            lzham_compress_params q;
            memset(&q, 0, sizeof(q));
            q.m_struct_size = sizeof(lzham_compress_params);
            q.m_dict_size_log2 = LZHAMTEST_DEFAULT_DICT_SIZE;

            mLzhamCompressState[worker_id] = lzham_compress_init(&q);
        }

    }

// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError CompressJPEGtoARHCMulti(DecoderReader &r, DecoderWriter &w, uint8 compression_level, uint8 componentCoalescing,
                                  bool do_lzham,
                                  ThreadContext *tc) {
    Decoder d(tc->get_allocator());
    DecoderCompressionMultiwriter cw(&w, compression_level, true, do_lzham, tc, tc->get_allocator(), &d);
    return d.decode(r, cw, componentCoalescing);
}
// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError DecompressARHCtoJPEGMulti(DecoderReader &r, DecoderWriter &w,
                             ThreadContext *tc) {
    DecoderDecompressionMultireader cr(&r, tc, tc->get_allocator());
    Decoder d(tc->get_allocator());
    return d.decode(cr, w, 0);
}

DecoderDecompressionMultireader::DecoderDecompressionMultireader(DecoderReader *r,
                                                                 ThreadContext *workers,
                                                                 const JpegAllocator<uint8_t> &alloc)
    : mWorkers(workers), mBuffer(alloc), mFallbackMemReader(alloc) {
    mReader = r;
    mCurComponentSize = 0;
    mCurComponentBytesScanned = 0;
    mNumSuccessfulComponents = -1;
    mNumComponents = 0;
    memset(mComponentStart, 0, sizeof(mComponentStart));
    memset(mComponentEnd, 0, sizeof(mComponentEnd));
    mFallbackDecompressionReader = NULL;
    mDoLzham = false;
};


size_t findLzhamStart(const std::vector<uint8_t, JpegAllocator<uint8_t> > &buffer,
                      size_t search_start) {
    for (size_t i = search_start - 4, j = 0; i > 0 && j <= 8; --i, ++j) {
        if (memcmp(&buffer[i], lzham_fixed_header, sizeof(lzham_fixed_header)) == 0) {
            return i;
        }
    }
    return 0;
}

static void arhcToLzhamHeader(uint8_t buffer[LZHAM0_HEADER_SIZE]) {
    assert(sizeof(lzham_fixed_header) < LZHAM0_HEADER_SIZE);
    memcpy(buffer, lzham_fixed_header, sizeof(lzham_fixed_header));
    buffer[sizeof(lzham_fixed_header)] = buffer[sizeof(arhclzhamheader)]; // dictionary size
    size_t i;
    for (i = sizeof(arhclzhamheader) + 1; i < LZHAM0_HEADER_SIZE; ++i) {
        assert(sizeof(lzham_fixed_header) + i - sizeof(arhclzhamheader) < LZHAM0_HEADER_SIZE);
        buffer[sizeof(lzham_fixed_header) + i - sizeof(arhclzhamheader)] = buffer[i];
    }
    for (size_t j = sizeof(lzham_fixed_header) + i - sizeof(arhclzhamheader); j < LZHAM0_HEADER_SIZE; ++j) {
        buffer[j] = 0;
    }
}

std::pair<uint32_t, JpegError> DecoderDecompressionMultireader::startDecompressionThreads() {
    uint8_t command_backup[THREAD_COMMAND_BUFFER_SIZE] = {0};
    mBuffer.resize(1024 * 1024);
    uint32_t offset = sizeof(command_backup);
    size_t scanned = offset;
    std::pair<uint32, JpegError> retval(0, JpegError());
    do {
        retval = mReader->Read(&mBuffer[scanned], mBuffer.size() - scanned);
        scanned += retval.first;
        if (retval.first ==0) {
            mBuffer.resize(scanned);
            break;
        }
        if (scanned == mBuffer.size()) {
            mBuffer.resize(mBuffer.size() * 2);
        }
    }while(!retval.second);
    if (retval.second && retval.second != JpegError::errEOF()) {
        retval.first = 0;
        return retval; // pass the error back to the caller
    }
    assert(scanned == mBuffer.size());
    mComponentStart[0] = offset; // sets things up for the comm
    int num_threads = (int)mWorkers->numThreads();
    int cur_component = 1; // 0th component always starts at zero
    if (mBuffer.size() < offset + std::max(sizeof(xzheader) + 8, LZHAM0_HEADER_SIZE)) {
        return std::pair<uint32, JpegError>(0, JpegError::errEOF());
    }
    if (memcmp(&mBuffer[offset], arhcheader, sizeof(arhcheader)) == 0) {
        assert(sizeof(xzheader) == sizeof(arhcheader) && "The xz and arhc headers must be of same size");
        memcpy(&mBuffer[offset], xzheader, sizeof(xzheader));
    }
    bool is_arhc_lzham = (memcmp(&mBuffer[offset], arhclzhamheader, sizeof(arhclzhamheader)) == 0);
    bool is_lzham = (memcmp(&mBuffer[offset], lzham_fixed_header, sizeof(lzham_fixed_header)) == 0);
    if (is_arhc_lzham || is_lzham) {
        if (is_arhc_lzham) {
            arhcToLzhamHeader(&mBuffer[offset]);
        }
        for (size_t i = offset + 0x10, ie = mBuffer.size() - 8; i < ie; i += 4) {
            uint8_t * to_scan = &mBuffer[i];
            uint8_t zero[4] = {0, 0, 0, 0};
            if (memcmp(to_scan, zero, 2) == 0) {
                if (memcmp(to_scan, zero, 4) == 0
                    || memcmp(to_scan + 1, zero, 4) == 0
                    || memcmp(to_scan + 2, zero, 4) == 0
                    || memcmp(to_scan + 3, zero, 4) == 0) {
                    size_t where = findLzhamStart(mBuffer, i); //returns 0 on failure
                    if (where > 0) {
                        assert (cur_component > 0 && "Code starts cur_component at 1 since 0 is covered by the start");
                        mComponentEnd[cur_component - 1] = where;
                        mComponentStart[cur_component] = where;
                        cur_component++;
                        if (cur_component >= MAX_COMPRESSION_THREADS) {
                            break;
                        }
                    }
                }
            }
        }
    } else for (size_t i = offset + 4, ie = mBuffer.size() - 8; i < ie; i += 4) {
        uint8_t * to_scan = &mBuffer[i];
        if (memcmp(to_scan, xzheader, 6) == 0) {
            assert (cur_component > 0 && "Code starts cur_component at 1 since 0 is covered by the start");
            mComponentEnd[cur_component - 1] = i;
            mComponentStart[cur_component] = i;
            cur_component++;
            if (cur_component >= MAX_COMPRESSION_THREADS) {
                break;
            }
        }
    }
    mComponentEnd[cur_component - 1] = mBuffer.size();
    for (int component = 0; component < cur_component; ++component) {
        //fprintf(stderr, "Input components are: [%d - %d)\n", mComponentStart[component], mComponentEnd[component]);
    }
    int to_merge = cur_component - 1;
    while (cur_component > num_threads) {
        if (to_merge - 1 < 0) {
            to_merge = cur_component - 1;
        }
        assert(to_merge - 1 >= 0);
        //fprintf(stderr, "Merging: [%d - %d) and [%d - %d)\n", mComponentStart[to_merge-1], mComponentEnd[to_merge-1],mComponentStart[to_merge], mComponentEnd[to_merge]);
        for (int j = to_merge; j < cur_component; ++j) {
            mComponentEnd[j - 1] = mComponentEnd[j];
            mComponentStart[j] = mComponentEnd[j - 1];
        }
        --cur_component;
        to_merge -= 2;
    }
    for (int component = 0; component < cur_component; ++component) {
        //fprintf(stderr, "Output components are: [%d - %d)\n", mComponentStart[component], mComponentEnd[component]);
    }
    mNumComponents = cur_component;
    for (int component = 0; component < cur_component; ++component) {
        assert(mComponentStart[component] >= sizeof(command_backup) && "component start will be beyond offset");
        // backup old bits
        uint8_t *temp_decompress_command = &mBuffer[mComponentStart[component] - sizeof(command_backup)];
        memcpy(command_backup,
               temp_decompress_command,
               sizeof(command_backup));
        uint32_t component_size = mComponentEnd[component] - mComponentStart[component];
        temp_decompress_command[0] = ThreadContext::DECOMPRESS;
        uint32toLE(component_size, temp_decompress_command + 1);
        int ret = write_full(mWorkers->thread_input_write[component],
                             temp_decompress_command,
                             sizeof(command_backup) + component_size);
        assert(ret > 0 && "IPC has failed");
        // restore old bits
        memcpy(temp_decompress_command,
               command_backup,
               sizeof(command_backup));
    }
    return std::pair<uint32, JpegError>(0, JpegError::nil());
}

class DeferDrainJobs {
    ThreadContext * mWorkers;
    int mStartWorker;
    int mEndWorker;
public:
    DeferDrainJobs(ThreadContext *tc, int start, int end) {
        mWorkers = tc;
        mStartWorker = start;
        mEndWorker = end;
    }
    ~DeferDrainJobs() {
        for (int i = mStartWorker; i < mEndWorker; ++i) {
            uint8_t command_buffer[THREAD_COMMAND_BUFFER_SIZE] = {0};
            read_full(mWorkers->thread_output_read[i], command_buffer, sizeof(command_buffer));
            ssize_t reply_size = LEtoUint32(command_buffer + 1);
            uint8_t discard[4096];
            ssize_t num_read = 0;
            while(num_read < reply_size) {
                ssize_t cur_read = read(mWorkers->thread_output_read[i],
                                        discard,
                                        std::min(reply_size - num_read, (ssize_t)4096));
                if (cur_read <= 0) {
                    break;
                }
                num_read += cur_read;
            }
        }
    }
};
std::pair<uint32, JpegError> DecoderDecompressionMultireader::Read(uint8*data,
                                                              unsigned int size) {
    if (mFallbackDecompressionReader) {
        return mFallbackDecompressionReader->Read(data, size);
    }
    if (mBuffer.empty()) { // need to start the compression jobs
        JpegError err = startDecompressionThreads().second;
        if (err) {
            return std::pair<uint32, JpegError>(0, err); // I/O error decompressing the file
        }
    }
    if (mNumSuccessfulComponents == mNumComponents){
        return std::pair<uint32, JpegError>(0, JpegError::errEOF());
    }
    while (mCurComponentSize == mCurComponentBytesScanned) {
        ++mNumSuccessfulComponents; // this gets set to 0 on the first pass
        mCurComponentBytesScanned = 0;
        mCurComponentSize = 0;
        if (mNumSuccessfulComponents == mNumComponents){
            return std::pair<uint32, JpegError>(0, JpegError::errEOF());
        }
        uint8_t command_buffer[THREAD_COMMAND_BUFFER_SIZE] = {0};
        read_full(mWorkers->thread_output_read[mNumSuccessfulComponents], command_buffer, sizeof(command_buffer));
        mCurComponentSize = LEtoUint32(command_buffer + 1);
        if (command_buffer[0] == ThreadContext::XZ_FAIL) {
            // DECOMPRESS IN PLACE (this could happen if there was a stray 4 byte aligned 7zXZ header around
            mFallbackMemReader.SwapIn(mBuffer, mComponentStart[mNumSuccessfulComponents]);
            if (mDoLzham) {
                mFallbackDecompressionReader = JpegAllocator<LZHAMDecompressionReader>(mBuffer.get_allocator()).allocate(1);
                new((void*)mFallbackDecompressionReader)LZHAMDecompressionReader(&mFallbackMemReader, mBuffer.get_allocator()); // inplace new
            } else {
                mFallbackDecompressionReader = JpegAllocator<DecoderDecompressionReader>(mBuffer.get_allocator()).allocate(1);
                new((void*)mFallbackDecompressionReader)DecoderDecompressionReader(&mFallbackMemReader, mBuffer.get_allocator()); // inplace new
            }
            DeferDrainJobs dj(mWorkers, mNumSuccessfulComponents + 1, mNumComponents);
            return mFallbackDecompressionReader->Read(data, size);            
        } else {
            assert(command_buffer[0] == ThreadContext::XZ_OK && "The decompressor must report either OK or fail");
        }
    }
    ssize_t retval = read(mWorkers->thread_output_read[mNumSuccessfulComponents],
                          data, std::min(size, mCurComponentSize - mCurComponentBytesScanned));
    if (retval < 0) {
        return std::pair<uint32, JpegError>(0, MakeJpegError("Error reading pipe"));
    }
    mCurComponentBytesScanned += retval;
    return std::pair<uint32, JpegError>(retval, JpegError::nil());
}

DecoderDecompressionMultireader::~DecoderDecompressionMultireader() {
    if (mFallbackDecompressionReader) {
        if (mDoLzham) {
            JpegAllocator<LZHAMDecompressionReader>(mBuffer.get_allocator()).destroy((LZHAMDecompressionReader*)mFallbackDecompressionReader);
            JpegAllocator<LZHAMDecompressionReader>(mBuffer.get_allocator()).deallocate((LZHAMDecompressionReader*)mFallbackDecompressionReader, 1);
        } else {
            JpegAllocator<DecoderDecompressionReader>(mBuffer.get_allocator()).destroy((DecoderDecompressionReader*)mFallbackDecompressionReader);
            JpegAllocator<DecoderDecompressionReader>(mBuffer.get_allocator()).deallocate((DecoderDecompressionReader*)mFallbackDecompressionReader, 1);
        }
        mFallbackDecompressionReader = NULL;
    }
}


DecoderCompressionMultiwriter::DecoderCompressionMultiwriter(DecoderWriter *w,
                                                             uint8_t compression_level,
                                                             bool replaceMagicWithARHC,
                                                             bool do_lzham,
                                                             ThreadContext *workers, 
                                                             const JpegAllocator<uint8_t> &alloc,
                                                             SizeEstimator * estimator)
       : mWorkers(workers), mBuffer(alloc) {
    mReplace7zMagicARHC = replaceMagicWithARHC;
    mDoLzham = do_lzham;
    mCompressionLevel = compression_level;
    mSizeEstimate = estimator;
    mClosed = false;
    mWriter = w;
    mCurWriteWorkerId = 0;
    mNumWorkers = mWorkers->numThreads();
    mBuffer.resize(THREAD_COMMAND_BUFFER_SIZE);
}
void DecoderCompressionMultiwriter::setEstimatedFileSize(SizeEstimator *estimator) {
    mSizeEstimate = estimator;
}
std::pair<uint32, JpegError> DecoderCompressionMultiwriter::Write(const uint8*data,
                                                             unsigned int size){
    unsigned int original_size = size;
    size_t expected_file_size = mSizeEstimate->getEstimatedSize();
    assert(mSizeEstimate->estimatedSizeReady() && "Multiwriter depends on an accurate size "
           "estimation to get full throughput");
    uint32 write_to_each_worker = expected_file_size / mNumWorkers + 1;
    size_t buffer_unique_num_bytes = (mBuffer.size() - THREAD_COMMAND_BUFFER_SIZE);

    while(mCurWriteWorkerId + 1 < mNumWorkers && size + buffer_unique_num_bytes >= write_to_each_worker) {        
        assert(write_to_each_worker > buffer_unique_num_bytes && "we cannot have written more than our buffers' worth to this place");
        uint32 amount_to_write = write_to_each_worker - buffer_unique_num_bytes;
        mBuffer.insert(mBuffer.end(),
                       data,
                       data + amount_to_write);
        data += amount_to_write;
        size -= amount_to_write;
        if(mDoLzham) {
            mBuffer[0] = ThreadContext::LZHAMCOMPRESS0 + std::min(mCompressionLevel - 1, 4);
        } else {
            mBuffer[0] = ThreadContext::COMPRESS1 + mCompressionLevel - 1;
        }
        uint32toLE(mBuffer.size() - THREAD_COMMAND_BUFFER_SIZE ,&mBuffer[1]);
        ssize_t retval = write_full(mWorkers->thread_input_write[mCurWriteWorkerId++],
                                    &mBuffer[0],
                                    mBuffer.size());
        assert(retval == (ssize_t)mBuffer.size());
        mBuffer.resize(THREAD_COMMAND_BUFFER_SIZE); // clear it out
        buffer_unique_num_bytes = (mBuffer.size() - THREAD_COMMAND_BUFFER_SIZE);
    }
    mBuffer.insert(mBuffer.end(), data, data + size);
    return std::pair<uint32, JpegError>(original_size, JpegError::nil());
}

void writeARHCHeaderCharacterFromLzham(uint8_t *character,
                                       int headerindex,
                                       uint8_t original_header[LZHAM0_HEADER_SIZE]) {
    original_header[headerindex] = *character;
    if (headerindex < (int)sizeof(lzham_fixed_header)) {
        assert(*character == lzham_fixed_header[headerindex]
               && "must start with LZHAM");
    }
    if (headerindex < (int)sizeof(arhclzhamheader)) {
        *character = arhclzhamheader[headerindex];
    } else if (headerindex == sizeof(arhclzhamheader)) {
        *character = original_header[sizeof(lzham_fixed_header)]; // dictionary size
    } else {
        assert(sizeof(lzham_fixed_header) + headerindex - sizeof(arhclzhamheader) < LZHAM0_HEADER_SIZE);
        *character = original_header[sizeof(lzham_fixed_header) + headerindex - sizeof(arhclzhamheader)]; // LE file size
    }
}

void DecoderCompressionMultiwriter::Close() {
    if(mDoLzham) {
        mBuffer[0] = ThreadContext::LZHAMCOMPRESS0 + std::min(mCompressionLevel - 1, 4);
    } else {
        mBuffer[0] = ThreadContext::COMPRESS1 + mCompressionLevel - 1;
    }
    uint32toLE(mBuffer.size() - THREAD_COMMAND_BUFFER_SIZE ,&mBuffer[1]);
    ssize_t retval = write_full(mWorkers->thread_input_write[mCurWriteWorkerId++],
                                &mBuffer[0],
                                mBuffer.size());
    assert(retval == (ssize_t)mBuffer.size());
    assert(!mClosed);
    mClosed = true;
    if (mBuffer.size()< 4096) {
        mBuffer.resize(4096);
    }
    for (int workerId = 0; workerId < mCurWriteWorkerId; ++workerId){
        int offset = THREAD_COMMAND_BUFFER_SIZE;
        size_t read_so_far = 0;
        size_t worker_goal = 0;
        while (true) {
            ssize_t status = read_at_least(mWorkers->thread_output_read[workerId],
                                           &mBuffer[0],
                                           mBuffer.size(),
                                           offset ? offset : 1);
            if (status <= 0) {
                break;
            }
            if (offset != 0) {
                assert(status >= offset && "We instructed read to read at least offset");
                assert(mBuffer[0] == ThreadContext::XZ_OK);
                worker_goal = LEtoUint32(&mBuffer[1]);
            }
            if (status > offset) {
                if (workerId == 0 && mReplace7zMagicARHC && read_so_far < std::max(LZHAM0_HEADER_SIZE,
                                                                                   sizeof(xzheader))) {
                    if (mDoLzham) {
                        if (read_so_far < LZHAM0_HEADER_SIZE) {
                            int headerindex = 0;
                            for (int i = 0; headerindex < (int)LZHAM0_HEADER_SIZE && i < (int)status; ++i, ++headerindex) {
                                writeARHCHeaderCharacterFromLzham(&mBuffer[offset + read_so_far + i], 
                                                                  headerindex,
                                                                  mOrigLzhamHeader);
                            }       
                        }
                    } else if (read_so_far < sizeof(xzheader)) {
                        int headerindex = 0;
                        for (int i = 0; headerindex < (int)sizeof(xzheader) && i < status; ++i, ++headerindex) {
                            assert(mBuffer[offset+read_so_far + i] == xzheader[headerindex]
                                   && "must start with \\xfd7zXZ");
                            mBuffer[offset+read_so_far + i] = arhcheader[headerindex];
                        }
                    }
                }
                mWriter->Write(&mBuffer[offset],
                               status - offset);
                read_so_far += status - offset;
            }
            offset = 0;
            if (read_so_far >= worker_goal) {
                assert(read_so_far == worker_goal);
                break;
            }
        }
        if ((read_so_far & 0x3) && !mDoLzham) { // unaligned magic
            uint8 zeros[4] = {0};
            mWriter->Write(zeros, 4 - (read_so_far & 0x3)); // lets make it aligned (I think it is required to be so)
            // if not, we'd only lose out in parallelism, not correctness
        }
    }
}


DecoderCompressionMultiwriter::~DecoderCompressionMultiwriter() {
    if (!mClosed) {
        Close();
    }
    assert(mClosed);
}

namespace {
ThreadContext *MakeThreadContextHelper(int nthreads, const JpegAllocator<uint8> &alloc, bool depriv, FaultInjectorXZ* fi, bool lzham) {
    ThreadContext *retval = JpegAllocator<ThreadContext>(alloc).allocate(1);
    new((void*)retval)ThreadContext(nthreads, alloc, depriv, fi); // inplace new
    if (lzham) {
        retval->initializeLzham(nthreads);
    }
    return retval;
}
}

ThreadContext *MakeThreadContext(int nthreads, const JpegAllocator<uint8> &alloc, bool lzham) {
    return MakeThreadContextHelper(nthreads, alloc, true, NULL, lzham);
}

ThreadContext *TestMakeThreadContext(int nthreads, const JpegAllocator<uint8> &alloc, bool depriv, FaultInjectorXZ* fi, bool lzham) {
    return MakeThreadContextHelper(nthreads, alloc, depriv, fi, lzham);
}

void DestroyThreadContext(ThreadContext *tc) {
    JpegAllocator<ThreadContext>(tc->get_allocator()).destroy(tc);
    JpegAllocator<ThreadContext>(tc->get_allocator()).deallocate(tc, 1);
}

// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError MultiCompressAnyto7Z(DecoderReader &r, DecoderWriter &w, uint8 compression_level,
                               bool do_lzham,
                               SizeEstimator *sz, ThreadContext *tc) {
    DecoderCompressionMultiwriter cw(&w, compression_level, false, do_lzham, tc, tc->get_allocator(), sz);
    return Copy(r, cw, tc->get_allocator());
}

// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError MultiDecompress7ZtoAny(DecoderReader &r, DecoderWriter &w, ThreadContext *tc) { 
    DecoderDecompressionMultireader cr(&r, tc, tc->get_allocator());
    return Copy(cr, w, tc->get_allocator());
}
}
