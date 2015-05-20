#include <sirikata/core/jpeg-arhc/Compression.hpp>
#include <lzma.h>
namespace Sirikata {


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
    if (actualBytesRead != size) {
        err = JpegError("Short read", mBuffer.get_allocator());
    }
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
            const char * error_message = "Magic Number Mismatch: ";
            std::vector<char, JpegAllocator<char> > error_vec(error_message, error_message + strlen(error_message),
                                                              mNewMagic.get_allocator());
            error_vec.insert(error_vec.end(), (char*)data, (char*)data + off + 1);
            error_vec.insert(error_vec.end(), mOriginalMagic.begin() + mMagicNumbersReplaced, mOriginalMagic.end());
            retval.second = JpegError(error_vec, mNewMagic.get_allocator());
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
			&mStream, UINT64_MAX, 0);
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
                return std::pair<uint32, JpegError>(0, JpegError("Invalid XZ magic number", mAlloc));
              case LZMA_DATA_ERROR:
              case LZMA_BUF_ERROR:
                return std::pair<uint32, JpegError>(size - mStream.avail_out,
                                                    JpegError("Corrupt xz file", mAlloc));
              case LZMA_MEM_ERROR:
                assert(false && "Memory allocation failed");
                break;
              default:
                assert(false && "Unknown LZMA error code");
            }
        }
    }
    return std::pair<uint32, JpegError>(0, JpegError("Unreachable", mAlloc));
}

DecoderDecompressionReader::~DecoderDecompressionReader() {
    lzma_end(&mStream);
}


DecoderCompressionWriter::DecoderCompressionWriter(DecoderWriter *w,
                                                   const JpegAllocator<uint8_t> &alloc)
        : mAlloc(alloc) {
    mClosed = false;
    mBase = w;
    mStream = LZMA_STREAM_INIT;
    mLzmaAllocator.alloc = mAlloc.get_custom_allocate();
    mLzmaAllocator.free = mAlloc.get_custom_deallocate();
    mLzmaAllocator.opaque = mAlloc.get_custom_state();
    mStream.allocator = &mLzmaAllocator;
    lzma_ret ret = lzma_easy_encoder(&mStream, 9, LZMA_CHECK_CRC64);
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
}
