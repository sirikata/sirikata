#include <sirikata/core/jpeg-arhc/Compression.hpp>
#include <lzma.h>
#if __GXX_EXPERIMENTAL_CXX0X__ || __cplusplus > 199711L
#include <thread>
#define USE_BUILTIN_THREADS
#define USE_BUILTIN_ATOMICS
#else
#include <sirikata/core/util/Thread.hpp>
#endif


#ifdef __linux
#include <sys/wait.h>
#include <linux/seccomp.h>

#include <sys/prctl.h>
#include <sys/syscall.h>
#endif

#define THREAD_COMMAND_BUFFER_SIZE 5


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

uint32 LEtoUint32(uint8*buffer) {
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


class ThreadContext {
    int thread_output_write[MAX_COMPRESSION_THREADS];

    int thread_input_read[MAX_COMPRESSION_THREADS];
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
                                               compressed_data.size());
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
                decompress(command_data, worker_id);
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
};
static const unsigned char xzheader[6] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };
static const unsigned char arhcheader[6] = { 'A', 'R', 'H', 'C', 0x01, 0x00 };

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
    if (mBuffer.size() < offset + sizeof(xzheader) + 8) {
        return std::pair<uint32, JpegError>(0, JpegError::errEOF());
    }
    if (memcmp(&mBuffer[offset], arhcheader, sizeof(arhcheader)) == 0) {
        assert(sizeof(xzheader) == sizeof(arhcheader) && "The xz and arhc headers must be of same size");
        memcpy(&mBuffer[offset], xzheader, sizeof(xzheader));
    }
    for (size_t i = offset + 4, ie = mBuffer.size() - 8; i < ie; i += 4) {
        uint8_t * to_scan = &mBuffer[i];
        if (memcmp(to_scan, xzheader, 6) == 0) {
            assert (cur_component > 0 && "Code starts cur_component at 1 since 0 is covered by the start");
            mComponentEnd[cur_component - 1] = i;
            mComponentStart[cur_component] = i;
            cur_component++;
            if (cur_component >= num_threads) {
                break;
            }
        }
    }
    if (!cur_component) {
        ++cur_component; // if we didn't match the magic
    }
    mNumComponents = cur_component;
    mComponentEnd[cur_component - 1] = mBuffer.size();
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
            // FIXME: DECOMPRESS IN PLACE (this could happen if there was a stray 4 byte aligned 7zXZ header around
            mFallbackMemReader.SwapIn(mBuffer, mComponentStart[mNumSuccessfulComponents]);
            mFallbackDecompressionReader = JpegAllocator<DecoderDecompressionReader>(mBuffer.get_allocator()).allocate(1);
            new((void*)mFallbackDecompressionReader)DecoderDecompressionReader(&mFallbackMemReader, mBuffer.get_allocator()); // inplace new
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
        JpegAllocator<DecoderDecompressionReader>(mBuffer.get_allocator()).destroy(mFallbackDecompressionReader);
        JpegAllocator<DecoderDecompressionReader>(mBuffer.get_allocator()).deallocate(mFallbackDecompressionReader, 1);
        mFallbackDecompressionReader = NULL;
    }
}


DecoderCompressionMultiwriter::DecoderCompressionMultiwriter(DecoderWriter *w,
                                                             uint8_t compression_level,
                                                             bool replaceMagicWithARHC,
                                                             ThreadContext *workers, 
                                                             const JpegAllocator<uint8_t> &alloc,
                                                             SizeEstimator * estimator)
       : mWorkers(workers), mBuffer(alloc) {
    mReplace7zMagicARHC = replaceMagicWithARHC;
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
        mBuffer.insert(mBuffer.end(),
                       data,
                       data + write_to_each_worker - buffer_unique_num_bytes);
        data += write_to_each_worker - buffer_unique_num_bytes;
        size -= write_to_each_worker - buffer_unique_num_bytes;
        mBuffer[0] = ThreadContext::COMPRESS1 + mCompressionLevel - 1;
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
    mBuffer[0] = ThreadContext::COMPRESS1 + mCompressionLevel - 1;
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
                if (workerId == 0 && mReplace7zMagicARHC && read_so_far < sizeof(xzheader)) {
                    int headerindex = 0;
                    for (int i = 0; i < status; ++i, ++headerindex) {
                        assert(mBuffer[offset+read_so_far + i] == xzheader[headerindex]
                               && "must start with \\xfd7zXZ");
                        mBuffer[offset+read_so_far + i] = arhcheader[headerindex];
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
        if (read_so_far & 0x3) { // unaligned magic
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
namespace {
ThreadContext *MakeThreadContextHelper(int nthreads, const JpegAllocator<uint8> &alloc, bool depriv, FaultInjectorXZ* fi) {
    ThreadContext *retval = JpegAllocator<ThreadContext>(alloc).allocate(1);
    new((void*)retval)ThreadContext(nthreads, alloc, depriv, fi); // inplace new
    return retval;
}
}

ThreadContext *MakeThreadContext(int nthreads, const JpegAllocator<uint8> &alloc) {
    return MakeThreadContextHelper(nthreads, alloc, true, NULL);
}

ThreadContext *TestMakeThreadContext(int nthreads, const JpegAllocator<uint8> &alloc, bool depriv, FaultInjectorXZ* fi) {
    return MakeThreadContextHelper(nthreads, alloc, depriv, fi);
}

void DestroyThreadContext(ThreadContext *tc) {
    JpegAllocator<ThreadContext>(tc->get_allocator()).destroy(tc);
    JpegAllocator<ThreadContext>(tc->get_allocator()).deallocate(tc, 1);
}

// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError MultiCompressAnyto7Z(DecoderReader &r, DecoderWriter &w, uint8 compression_level,
                               SizeEstimator *sz, ThreadContext *tc) {
    DecoderCompressionMultiwriter cw(&w, compression_level, false, tc, tc->get_allocator(), sz);
    return Copy(r, cw, tc->get_allocator());
}

// Decode reads a JPEG image from r and returns it as an image.Image.
JpegError MultiDecompress7ZtoAny(DecoderReader &r, DecoderWriter &w, ThreadContext *tc) { 
    DecoderDecompressionMultireader cr(&r, tc, tc->get_allocator());
    return Copy(cr, w, tc->get_allocator());
}

}
