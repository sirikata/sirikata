#include <sirikata/core/jpeg-arhc/Compression.hpp>
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

namespace Sirikata {

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
static const unsigned char lzham_fixed_header[] = {'L','Z','H','0'};
static const unsigned char xzheader[6] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };
static const unsigned char arhcheader[6] = { 'A', 'R', 'H', 'C', 0x01, 0x00 };
static const unsigned char arhclzhamheader[6] = { 'A', 'R', 'H', 'C', 0x01, 0x01 };

void writeLZHAMHeader(uint8 * output, uint8 dictSize, uint32 fileSize) {
    output[0] = lzham_fixed_header[0];
    output[1] = lzham_fixed_header[1];
    output[2] = lzham_fixed_header[2];
    output[3] = lzham_fixed_header[3];
    output[4] = dictSize;
    output[5] = fileSize & 0xff;
    output[6] = (fileSize >> 8) & 0xff;
    output[7] = (fileSize >> 16) & 0xff;
    output[8] = (fileSize >> 24) & 0xff;
    output[9] = 0;
    output[10] = 0;
    output[11] = 0;
    output[12] = 0;
    
}

void writeARHCHeaderCharacterFromLzham(uint8_t *character,
                                       int headerindex,
                                       uint8_t original_header[LZHAM0_HEADER_SIZE]) {
    original_header[headerindex] = *character;
    if (headerindex < sizeof(lzham_fixed_header)) {
        assert(*character == lzham_fixed_header[headerindex]
               && "must start with LZHAM");
    }
    if (headerindex < sizeof(arhclzhamheader)) {
        *character = arhclzhamheader[headerindex];
    } else if (headerindex == sizeof(arhclzhamheader)) {
        *character = original_header[sizeof(lzham_fixed_header)]; // dictionary size
    } else {
        assert(sizeof(lzham_fixed_header) + headerindex - sizeof(arhclzhamheader) < LZHAM0_HEADER_SIZE);
        *character = original_header[sizeof(lzham_fixed_header) + headerindex - sizeof(arhclzhamheader)]; // LE file size
    }
}

void arhcToLzhamHeader(uint8_t buffer[LZHAM0_HEADER_SIZE]) {
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

namespace {
std::pair<lzham_decompress_params, JpegError> makeLZHAMDecodeParams(const uint8 header[LZHAM0_HEADER_SIZE]) {
    std::pair<lzham_decompress_params, JpegError> retval;
    retval.second = JpegError::nil();
    memset(&retval.first, 0, sizeof(retval.first));
    if (memcmp(header, "LZH0", 4)) {
        retval.second = MakeJpegError("LZHAM Header Error");
    } else {
        retval.first.m_struct_size = sizeof(lzham_decompress_params);
        retval.first.m_dict_size_log2 = header[4];
    }
    return retval;
}
}

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
    void compressLZHAM(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
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

    void decompressLZHAM(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
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
    void send_xz_fail(int output_fd) {
        uint8 failure_message[THREAD_COMMAND_BUFFER_SIZE] = {0};
        failure_message[0] = XZ_FAIL;
        write_full(output_fd, failure_message, sizeof(failure_message));
    }
    void decompress(const std::vector<uint8_t, JpegAllocator<uint8_t> > &data,
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
    void worker(size_t worker_id, JpegAllocator<uint8_t> alloc) {
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
public:
    int thread_output_read[MAX_COMPRESSION_THREADS];
    int thread_input_write[MAX_COMPRESSION_THREADS];
    size_t num_threads;
    size_t numThreads() const {
        return num_threads;
    }
    ~ThreadContext() {
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
    ThreadContext(size_t num_threads, const JpegAllocator<uint8_t> & alloc, bool depriv, FaultInjectorXZ *faultInjection) : mAllocator(alloc) {
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
    void initializeLzham(int nthreads) {
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
    const JpegAllocator<uint8_t> &get_allocator() const {
        return mAllocator;
    }
};


std::pair<Sirikata::uint32, Sirikata::JpegError> MemReadWriter::Write(const Sirikata::uint8*data, unsigned int size) {
    using namespace Sirikata;
    mBuffer.insert(mBuffer.begin() + mWriteCursor, data, data + size);
    mWriteCursor += size;
    return std::pair<Sirikata::uint32, JpegError>(size, JpegError());
}
std::pair<Sirikata::uint32, Sirikata::JpegError> MemReadWriter::Read(Sirikata::uint8*data, unsigned int size) {
    using namespace Sirikata;
    size_t bytesLeft = mBuffer.size() - mReadCursor;
    size_t actualBytesRead = size;
    if (bytesLeft < size) {
        actualBytesRead = bytesLeft;
    }
    if (actualBytesRead > 0) {
        memcpy(data, &mBuffer[mReadCursor], actualBytesRead);
    }
    mReadCursor += actualBytesRead;
    JpegError err = JpegError();
    if (actualBytesRead == 0) {
        err = JpegError::errEOF();
    }
    //fprintf(stderr, "%d READ %02x%02x%02x%02x - %02x%02x%02x%02x\n", (uint32)actualBytesRead, data[0], data[1],data[2], data[3],
    //        data[actualBytesRead-4],data[actualBytesRead-3],data[actualBytesRead-2],data[actualBytesRead-1]);

    std::pair<Sirikata::uint32, JpegError> retval(actualBytesRead, err);
    return retval;
}



MagicNumberReplacementReader::MagicNumberReplacementReader(DecoderReader *r,
                                                           const std::vector<uint8_t, JpegAllocator<uint8> >& originalMagic,
                                                           const std::vector<uint8_t, JpegAllocator<uint8> >& newMagic)
        : mOriginalMagic(originalMagic), mNewMagic(newMagic) {
    mBase = r;
    mMagicNumbersReplaced = 0;
    assert(mOriginalMagic.size() == mNewMagic.size() && "Magic numbers must be the same length");
}
std::pair<uint32, JpegError> MagicNumberReplacementReader::Read(uint8*data, unsigned int size){
    std::pair<uint32, JpegError> retval = mBase->Read(data, size);
    for (size_t off = 0;
         mMagicNumbersReplaced < mOriginalMagic.size() && off < size;
         ++mMagicNumbersReplaced, ++off) {
        if (memcmp(data + off, mOriginalMagic.data() + mMagicNumbersReplaced, 1) != 0) {
            retval.second = MakeJpegError("Magic Number Mismatch");
        }
        data[off] = mNewMagic[mMagicNumbersReplaced];
    }
    return retval;
}
MagicNumberReplacementReader::~MagicNumberReplacementReader(){

}

MagicNumberReplacementWriter::MagicNumberReplacementWriter(DecoderWriter *w,
                                                           const std::vector<uint8_t, JpegAllocator<uint8> >& originalMagic,
                                                           const std::vector<uint8_t, JpegAllocator<uint8> >& newMagic)
        : mOriginalMagic(originalMagic), mNewMagic(newMagic) {
    mBase = w;
    mMagicNumbersReplaced = 0;
    assert(mOriginalMagic.size() == mNewMagic.size() && "Magic numbers must be the same length");
}
std::pair<uint32, JpegError> MagicNumberReplacementWriter::Write(const uint8*data, unsigned int size) {
    if (mMagicNumbersReplaced < mOriginalMagic.size()) {
        if (size > mOriginalMagic.size() - mMagicNumbersReplaced) {
            std::vector<uint8> replacedMagic(data, data + size);
            for (size_t off = 0;mMagicNumbersReplaced < mOriginalMagic.size(); ++mMagicNumbersReplaced, ++off) {
                assert(memcmp(mOriginalMagic.data() + mMagicNumbersReplaced, &replacedMagic[off], 1) == 0);
                replacedMagic[off] = mNewMagic[mMagicNumbersReplaced];
            }
            return mBase->Write(&replacedMagic[0], size);
        } else {
            assert(memcmp(data, mOriginalMagic.data() + mMagicNumbersReplaced, size) == 0);
            mMagicNumbersReplaced += size;
            return mBase->Write((uint8*)mNewMagic.data() + (mMagicNumbersReplaced - size),
                                size);
        }
    } else {
        return mBase->Write(data, size);
    }
}
MagicNumberReplacementWriter::~MagicNumberReplacementWriter() {

}
void MagicNumberReplacementWriter::Close() {
    mBase->Close();
}



lzham_compress_params makeLZHAMEncodeParams(int level) {
    lzham_compress_params params;
    memset(&params, 0, sizeof(params));

    params.m_struct_size = sizeof(lzham_compress_params);
    params.m_dict_size_log2 = LZHAMTEST_DEFAULT_DICT_SIZE;//LZHAM_MAX_DICT_SIZE_LOG2_X64;
    switch(level) {
      case 0:
        params.m_level = LZHAM_COMP_LEVEL_FASTEST;
        break;
      case 1:
      case 2:
        params.m_level = LZHAM_COMP_LEVEL_FASTER;
        break;
      case 3:
      case 4:
        params.m_level = LZHAM_COMP_LEVEL_DEFAULT;
        break;
      case 9:
      case 8:
      case 7:
        params.m_level = LZHAM_COMP_LEVEL_UBER;
        break;
      case 5:
      case 6:
      default:
        params.m_level = LZHAM_COMP_LEVEL_BETTER;
    }
    params.m_compress_flags = LZHAM_COMP_FLAG_DETERMINISTIC_PARSING;
    params.m_max_helper_threads = 0;
    return params;
}

LZHAMDecompressionReader::LZHAMDecompressionReader(DecoderReader *r,
                                                       const JpegAllocator<uint8_t> &alloc)
        : mAlloc(alloc) {
    mBase = r;
    mHeaderBytesRead = 0;
    mLzham = NULL;
    mAvailIn = 0;
    mReadOffset = mReadBuffer;
    memset(mHeader, 0xff, sizeof(mHeader));
    if (alloc.get_custom_reallocate() && alloc.get_custom_msize()) {
        lzham_set_memory_callbacks(alloc.get_custom_reallocate(),
                                   alloc.get_custom_msize(),
                                   alloc.get_custom_state());
    }
};

std::pair<uint32, JpegError> LZHAMDecompressionReader::Read(uint8*data,
                                                              unsigned int size){
    
    bool inputEof = false;
    bool outputEof = false; // when we reach the end of one (of possibly many) concatted streams
    size_t outAvail = size;
    while(true) {
        JpegError err = JpegError::nil();
        lzma_action action = LZMA_RUN;
        if (inputEof == false  && mAvailIn < sizeof(mReadBuffer) / 8) {
            if (mAvailIn == 0) {
                mReadOffset = mReadBuffer;
            } else if (mReadOffset - mReadBuffer> sizeof(mReadBuffer) * 3 / 4) {
                // guaranteed not to overlap since it will only be at
                // most 1/8 the size of the buffer
                memcpy(mReadBuffer, mReadOffset, mAvailIn);
                mReadOffset = mReadBuffer;
            }
            size_t toRead = sizeof(mReadBuffer) - (mReadOffset + mAvailIn - mReadBuffer);
            assert(toRead > 0);
            std::pair<uint32, JpegError> bytesRead = mBase->Read(mReadOffset + mAvailIn, toRead);
            mAvailIn += bytesRead.first;
            err = bytesRead.second;
            if (bytesRead.first == 0) {
                inputEof = true;
            }
        }
        if (outputEof && !inputEof) {
            // gotta reset the lzham state
            mHeaderBytesRead = 0;
            outputEof = false;
        }
        if (mHeaderBytesRead < LZHAM0_HEADER_SIZE) {
            if (mHeaderBytesRead + mAvailIn <= LZHAM0_HEADER_SIZE) {
                memcpy(mHeader + mHeaderBytesRead, mReadOffset, mAvailIn);
                mHeaderBytesRead += mAvailIn;
                mAvailIn = 0;
            } else {
                memcpy(mHeader + mHeaderBytesRead, mReadOffset, LZHAM0_HEADER_SIZE - mHeaderBytesRead);
                mAvailIn -= LZHAM0_HEADER_SIZE - mHeaderBytesRead;
                mReadOffset += LZHAM0_HEADER_SIZE - mHeaderBytesRead;
                mHeaderBytesRead = LZHAM0_HEADER_SIZE;
            }
            if (mHeaderBytesRead == LZHAM0_HEADER_SIZE) {
                std::pair<lzham_decompress_params, JpegError> p = makeLZHAMDecodeParams(mHeader);
                if (p.second != JpegError::nil()) {
                    return std::pair<uint32, JpegError>(0, p.second);
                }
                if (mLzham == NULL) {
                    mLzham = lzham_decompress_init(&p.first);
                } else {
                    mLzham = lzham_decompress_reinit((lzham_decompress_state_ptr)mLzham, &p.first);
                }
                assert(mLzham && "the stream decoder had insufficient memory");
            }
        }
        size_t nread = mAvailIn;
        size_t nwritten = outAvail;
        lzham_decompress_status_t status = lzham_decompress(
            (lzham_compress_state_ptr)mLzham,
            mReadOffset, &nread,
            data + size - outAvail, &nwritten,
            inputEof);
        mReadOffset += nread;
        mAvailIn -= nread;
        outAvail -= nwritten;
        if (status >= LZHAM_DECOMP_STATUS_FIRST_SUCCESS_OR_FAILURE_CODE) {
            if (status == LZHAM_DECOMP_STATUS_SUCCESS) {
                outputEof = true;
            }
            if (status >= LZHAM_DECOMP_STATUS_FIRST_FAILURE_CODE) {
                switch(status) {
                  case LZHAM_DECOMP_STATUS_FAILED_INITIALIZING:
                  case LZHAM_DECOMP_STATUS_FAILED_DEST_BUF_TOO_SMALL:
                  case LZHAM_DECOMP_STATUS_FAILED_EXPECTED_MORE_RAW_BYTES:
                  case LZHAM_DECOMP_STATUS_FAILED_BAD_CODE:
                  case LZHAM_DECOMP_STATUS_FAILED_ADLER32:
                  case LZHAM_DECOMP_STATUS_FAILED_BAD_RAW_BLOCK:
                  case LZHAM_DECOMP_STATUS_FAILED_BAD_COMP_BLOCK_SYNC_CHECK:
                  case LZHAM_DECOMP_STATUS_INVALID_PARAMETER:
                  default:
                    return std::pair<uint32, JpegError>(size - outAvail, MakeJpegError("LZHAM error"));
                }
                break;
            }
        }
        if (outAvail == 0 || (inputEof && outputEof)) {
            unsigned int write_size = size - outAvail;
            return std::pair<uint32, JpegError>(write_size,JpegError::nil());
        }
    }
    assert(false);//FIXME
    unsigned int write_size = size - outAvail;
    return std::pair<uint32, JpegError>(write_size,JpegError::nil());
    //return std::pair<uint32, JpegError>(0, MakeJpegError("Unreachable"));
}

LZHAMDecompressionReader::~LZHAMDecompressionReader() {
    if (mLzham) {
        lzham_decompress_deinit((lzham_decompress_state_ptr)mLzham);
    }
}




LZHAMCompressionWriter::LZHAMCompressionWriter(DecoderWriter *w,
                                               uint8_t compression_level,
                                               const JpegAllocator<uint8_t> &alloc)
    : mWriteBuffer(alloc) {
    if (alloc.get_custom_reallocate() && alloc.get_custom_msize()) {
        lzham_set_memory_callbacks(alloc.get_custom_reallocate(),
                                   alloc.get_custom_msize(),
                                   alloc.get_custom_state());
    }
    mClosed = false;
    mBase = w;
    lzham_compress_params p = makeLZHAMEncodeParams(compression_level);
    mDictSizeLog2 = p.m_dict_size_log2;
    mLzham = lzham_compress_init(&p);
    assert(mLzham && "Problem with initialization");
    size_t defaultStartBufferSize = 1024 - LZHAM0_HEADER_SIZE;
    mAvailOut = defaultStartBufferSize;
    mWriteBuffer.resize(mAvailOut + LZHAM0_HEADER_SIZE);
    mBytesWritten = 0;
}



void LZHAMCompressionWriter::Close(){
    assert(!mClosed);
    mClosed = true;
    //assert((mBytesWritten >> 24) == 0 && "Expect a small item to compress");
    writeLZHAMHeader(&mWriteBuffer[0], mDictSizeLog2, mBytesWritten);
    std::pair<uint32, JpegError> r = mBase->Write(&mWriteBuffer[0], mWriteBuffer.size() - mAvailOut);
    if (r.second != JpegError::nil()) {
        return; // ERROR
    }
    if (mWriteBuffer.size() < 4 * 65536) {
        mWriteBuffer.resize(4 * 65536);
    }
    while(true) {
        size_t zero = 0;
        size_t nwritten = mWriteBuffer.size();
        lzham_compress_status_t ret = lzham_compress((lzham_compress_state_ptr)mLzham,
                                                     NULL, &zero,
                                                     &mWriteBuffer[0], &nwritten,
                                                     true);
        if (ret == LZHAM_COMP_STATUS_HAS_MORE_OUTPUT) {
            assert(nwritten == mWriteBuffer.size());
        }
        if (nwritten > 0) {
            std::pair<uint32, JpegError> r = mBase->Write(&mWriteBuffer[0], nwritten);
            if (r.second != JpegError::nil()) {
                return;
            }
        }
        if (ret == LZHAM_COMP_STATUS_NOT_FINISHED) {
            continue;
        } else if (ret == LZHAM_COMP_STATUS_HAS_MORE_OUTPUT) {
            continue;
        } else if (ret == LZHAM_COMP_STATUS_SUCCESS) {
            return;
        } else {
            assert(ret != LZHAM_COMP_STATUS_FAILED && "Something went wrong");
            assert(ret != LZHAM_COMP_STATUS_INVALID_PARAMETER && "Something went wrong with param");
            assert(ret != LZHAM_COMP_STATUS_FAILED_INITIALIZING && "Something went wrong with init");
            assert(false && "UNREACHABLE");
            return;
        }
    }
}

std::pair<uint32, JpegError> LZHAMCompressionWriter::Write(const uint8*data,
                                                             unsigned int size){
    size_t availIn = size;
    std::pair<uint32, JpegError> retval (0, JpegError::nil());
    mBytesWritten += size;
    while(availIn > 0) {
        if (mAvailOut == 0) {
            mAvailOut += mWriteBuffer.size();
            mWriteBuffer.resize(mWriteBuffer.size() * 2);
        }
        size_t nread = availIn; // this must be temporarily set to to
                                // the bytes avail
        size_t nwritten = mAvailOut;
        lzham_compress_status_t ret = lzham_compress((lzham_compress_state_ptr)mLzham,
                                                     data + size - availIn, &nread,
                                                     &mWriteBuffer[0] + mWriteBuffer.size() - mAvailOut, &nwritten,
                                                     false);
        mAvailOut -= nwritten;
        availIn -= nread;
        if (ret == LZHAM_COMP_STATUS_NEEDS_MORE_INPUT) {
            assert(availIn == 0);
        }
        if (ret >= LZHAM_COMP_STATUS_FIRST_FAILURE_CODE) {
            assert(false && "LZHAM COMPRESSION FAILED");
        }
    }
    return std::pair<uint32, JpegError>(size - availIn, JpegError::nil());
}

LZHAMCompressionWriter::~LZHAMCompressionWriter() {
    if (!mClosed) {
        Close();
    }
    assert(mClosed);
    lzham_compress_deinit((lzham_compress_state_ptr)mLzham);
}


DecoderDecompressionReader::DecoderDecompressionReader(DecoderReader *r,
                                                       const JpegAllocator<uint8_t> &alloc)
        : mAlloc(alloc) {
    mBase = r;
    mStream = LZMA_STREAM_INIT;
    mLzmaAllocator.alloc = mAlloc.get_custom_allocate();
    mLzmaAllocator.free = mAlloc.get_custom_deallocate();
    mLzmaAllocator.opaque = mAlloc.get_custom_state();
    mStream.allocator = &mLzmaAllocator;

    lzma_ret ret = lzma_stream_decoder(
			&mStream, UINT64_MAX, LZMA_CONCATENATED);
	mStream.next_in = NULL;
	mStream.avail_in = 0;
    if (ret != LZMA_OK) {
        switch(ret) {
          case LZMA_MEM_ERROR:
            assert(ret == LZMA_OK && "the stream decoder had insufficient memory");
            break;
          case LZMA_OPTIONS_ERROR:
            assert(ret == LZMA_OK && "the stream decoder had incorrect options for the system version");
            break;
          default:
            assert(ret == LZMA_OK && "the stream decoder was not initialized properly");
        }
    }
};

std::pair<uint32, JpegError> DecoderDecompressionReader::Read(uint8*data,
                                                              unsigned int size){
    mStream.next_out = data;
    mStream.avail_out = size;
    while(true) {
        JpegError err = JpegError::nil();
        lzma_action action = LZMA_RUN;
        if (mStream.avail_in == 0) {
            mStream.next_in = mReadBuffer;
            std::pair<uint32, JpegError> bytesRead = mBase->Read(mReadBuffer, sizeof(mReadBuffer));
            mStream.avail_in = bytesRead.first;
            err = bytesRead.second;
            if (bytesRead.first == 0) {
                action = LZMA_FINISH;
            }
        }
        lzma_ret ret = lzma_code(&mStream, action);
        if (mStream.avail_out == 0 || ret == LZMA_STREAM_END) {
            unsigned int write_size = size - mStream.avail_out;
            return std::pair<uint32, JpegError>(write_size,JpegError::nil());
/*                                                (ret == LZMA_STREAM_END
                                                 || (ret == LZMA_OK &&write_size > 0))
                                                 ? JpegError::nil() : err);*/
        }
        if (ret != LZMA_OK) {
            switch(ret) {
              case LZMA_FORMAT_ERROR:
                return std::pair<uint32, JpegError>(0, MakeJpegError("Invalid XZ magic number"));
              case LZMA_DATA_ERROR:
              case LZMA_BUF_ERROR:
                return std::pair<uint32, JpegError>(size - mStream.avail_out,
                                                    MakeJpegError("Corrupt xz file"));
              case LZMA_MEM_ERROR:
                assert(false && "Memory allocation failed");
                break;
              default:
                assert(false && "Unknown LZMA error code");
            }
        }
    }
    return std::pair<uint32, JpegError>(0, MakeJpegError("Unreachable"));
}

DecoderDecompressionReader::~DecoderDecompressionReader() {
    lzma_end(&mStream);
}


DecoderCompressionWriter::DecoderCompressionWriter(DecoderWriter *w,
                                                   uint8_t compression_level,
                                                   const JpegAllocator<uint8_t> &alloc)
        : mAlloc(alloc) {
    mClosed = false;
    mBase = w;
    mStream = LZMA_STREAM_INIT;
    mLzmaAllocator.alloc = mAlloc.get_custom_allocate();
    mLzmaAllocator.free = mAlloc.get_custom_deallocate();
    mLzmaAllocator.opaque = mAlloc.get_custom_state();
    mStream.allocator = &mLzmaAllocator;
    lzma_ret ret = lzma_easy_encoder(&mStream, compression_level, LZMA_CHECK_NONE);
	mStream.next_in = NULL;
	mStream.avail_in = 0;
    if (ret != LZMA_OK) {
        switch(ret) {
          case LZMA_MEM_ERROR:
            assert(ret == LZMA_OK && "the stream decoder had insufficient memory");
            break;
          case LZMA_OPTIONS_ERROR:
            assert(ret == LZMA_OK && "the stream decoder had incorrect options for the system version");
            break;            
          case LZMA_UNSUPPORTED_CHECK:
            assert(ret == LZMA_OK && "Specified integrity check but not supported");
          default:
            assert(ret == LZMA_OK && "the stream decoder was not initialized properly");
        }
    }
}


void DecoderCompressionWriter::Close(){
    assert(!mClosed);
    mClosed = true;
    while(true) {
        lzma_ret ret = lzma_code(&mStream, LZMA_FINISH);
        if (mStream.avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = sizeof(mWriteBuffer) - mStream.avail_out;
            if (write_size > 0) {
                std::pair<uint32, JpegError> r = mBase->Write(mWriteBuffer, write_size);
                if (r.second != JpegError::nil()) {
                    return;
                }
                mStream.avail_out = sizeof(mWriteBuffer);
                mStream.next_out = mWriteBuffer;
            }
        }
        if (ret == LZMA_STREAM_END) {
            return;
        }
    }
}

std::pair<uint32, JpegError> DecoderCompressionWriter::Write(const uint8*data,
                                                             unsigned int size){
    mStream.next_out = mWriteBuffer;
    mStream.avail_out = sizeof(mWriteBuffer);
    mStream.avail_in = size;
    mStream.next_in = data;
    std::pair<uint32, JpegError> retval (0, JpegError::nil());
    while(mStream.avail_in > 0) {
        lzma_ret ret = lzma_code(&mStream, LZMA_RUN);
        if (mStream.avail_in == 0 || mStream.avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = sizeof(mWriteBuffer) - mStream.avail_out;
            if (write_size > 0) {
                std::pair<uint32, JpegError> r = mBase->Write(mWriteBuffer, write_size);
                mStream.avail_out = sizeof(mWriteBuffer);
                mStream.next_out = mWriteBuffer;
                retval.first += r.first;
                if (r.second != JpegError::nil()) {
                    retval.second = r.second;
                    return retval;
                }
            }
        }
    }
    return retval;
}

DecoderCompressionWriter::~DecoderCompressionWriter() {
    if (!mClosed) {
        Close();
    }
    assert(mClosed);
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
        size_t worker_progress = 0;
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
                            for (int i = 0; headerindex < LZHAM0_HEADER_SIZE && i < status; ++i, ++headerindex) {
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


static JpegError Copy(DecoderReader &r, DecoderWriter &w, const JpegAllocator<uint8> &alloc) {
    std::vector<uint8, JpegAllocator<uint8> > buffer(alloc);
    size_t bufferSize = 16384;
    buffer.resize(bufferSize);
    std::pair<uint32, JpegError> ret;
    while (true) {
        ret = r.Read(&buffer[0], bufferSize);
        if (ret.first == 0) {
            w.Close();
            return JpegError::nil();
        }
        uint32 offset = 0;
        std::pair<uint32, JpegError> wret = w.Write(&buffer[offset], ret.first - offset);
        offset += wret.first;
        if (wret.second != JpegError::nil()) {
            w.Close();
            return wret.second;
        }
        if (ret.second != JpegError::nil()) {
            w.Close();
            if (ret.second == JpegError::errEOF()) {
                return JpegError::nil();
            }
            return ret.second;
        }
    }
}


// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError CompressAnyto7Z(DecoderReader &r, DecoderWriter &w, uint8 compression_level, const JpegAllocator<uint8> &alloc) {
    DecoderCompressionWriter cw(&w, compression_level, alloc);
    return Copy(r, cw, alloc);
}
// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError Decompress7ZtoAny(DecoderReader &r, DecoderWriter &w, const JpegAllocator<uint8> &alloc) {
    DecoderDecompressionReader cr(&r, alloc);
    return Copy(cr, w, alloc);
}


// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError CompressAnytoLZHAM(DecoderReader &r, DecoderWriter &w, uint8 compression_level, const JpegAllocator<uint8> &alloc) {
    LZHAMCompressionWriter cw(&w, compression_level, alloc);
    return Copy(r, cw, alloc);
}
// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError DecompressLZHAMtoAny(DecoderReader &r, DecoderWriter &w, const JpegAllocator<uint8> &alloc) {
    LZHAMDecompressionReader cr(&r, alloc);
    return Copy(cr, w, alloc);
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

}
