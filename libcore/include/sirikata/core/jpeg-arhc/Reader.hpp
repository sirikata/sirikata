#ifndef _SIRIKATA_READER_HPP_
#define _SIRIKATA_READER_HPP_

#include "Allocator.hpp"
#include "Error.hpp"
namespace Sirikata {
class SIRIKATA_EXPORT DecoderReader {
public:
    virtual std::pair<uint32, JpegError> Read(uint8*data, unsigned int size) = 0;
    virtual ~DecoderReader(){}
};
class SIRIKATA_EXPORT DecoderWriter {
public:
    virtual std::pair<uint32, JpegError> Write(const uint8*data, unsigned int size) = 0;
    virtual void Close() = 0;
    virtual ~DecoderWriter(){}
};

class SIRIKATA_EXPORT MemReadWriter : public Sirikata::DecoderWriter, public Sirikata::DecoderReader {
    std::vector<Sirikata::uint8, JpegAllocator<uint8_t> > mBuffer;
    size_t mReadCursor;
    size_t mWriteCursor;
  public:
    MemReadWriter(const JpegAllocator<uint8_t> &alloc) : mBuffer(alloc){
        mReadCursor = 0;
        mWriteCursor = 0;
    }
    void Close() {
        mReadCursor = 0;
        mWriteCursor = 0;
    }
    void SwapIn(std::vector<Sirikata::uint8, JpegAllocator<uint8_t> > &buffer, size_t offset) {
        mReadCursor = offset;
        mWriteCursor = buffer.size();
        buffer.swap(mBuffer);
    }
    void CopyIn(const std::vector<Sirikata::uint8, JpegAllocator<uint8_t> > &buffer, size_t offset) {
        mReadCursor = offset;
        mWriteCursor = buffer.size();
        mBuffer = buffer;
    }
    virtual std::pair<Sirikata::uint32, Sirikata::JpegError> Write(const Sirikata::uint8*data, unsigned int size);
    virtual std::pair<Sirikata::uint32, Sirikata::JpegError> Read(Sirikata::uint8*data, unsigned int size);
    std::vector<Sirikata::uint8, JpegAllocator<uint8_t> > &buffer() {
        return mBuffer;
    }
    const std::vector<Sirikata::uint8, JpegAllocator<uint8_t> > &buffer() const{
        return mBuffer;
    }
};
SIRIKATA_FUNCTION_EXPORT JpegError Copy(DecoderReader &r, DecoderWriter &w, const JpegAllocator<uint8> &alloc);
}
#endif
