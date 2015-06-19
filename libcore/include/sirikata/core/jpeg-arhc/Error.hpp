#ifndef _SIRIKATA_JPEG_ARHC_ERROR_HPP_
#define _SIRIKATA_JPEG_ARHC_ERROR_HPP_

namespace Sirikata {

class JpegError {
    explicit JpegError(const char * wh):mWhat(ERR_MISC) {
    }
public:
    enum ErrorMessage{
        NO_ERROR,
        ERR_EOF,
        ERR_FF00,
        ERR_SHORT_HUFFMAN,
        ERR_MISC
    } mWhat;
    static JpegError MakeFromStringLiteralOnlyCallFromMacro(const char*wh) {
        return JpegError(ERR_MISC, ERR_MISC);
    }
    explicit JpegError(ErrorMessage err, ErrorMessage err2):mWhat(err) {

    }
    JpegError() :mWhat() { // uses default allocator--but it won't allocate, so that's ok
        mWhat = NO_ERROR;
    }
    const char * what() const {
        if (mWhat == NO_ERROR) return "";
        if (mWhat == ERR_EOF) return "EOF";
        if (mWhat == ERR_FF00) return "MissingFF00";
        if (mWhat == ERR_SHORT_HUFFMAN) return "ShortHuffman";
        return "MiscError";
    }
    operator bool() {
        return mWhat != NO_ERROR;
    }
    static JpegError nil() {
        return JpegError();
    }
    static JpegError errEOF() {
        return JpegError(ERR_EOF,ERR_EOF);
    }
    static JpegError errMissingFF00() {
        return JpegError(ERR_FF00, ERR_FF00);
    }
    static JpegError errShortHuffmanData(){
        return JpegError(ERR_SHORT_HUFFMAN, ERR_SHORT_HUFFMAN);
    }
};
#define MakeJpegError(s) JpegError::MakeFromStringLiteralOnlyCallFromMacro("" s)
#define JpegErrorUnsupportedError(s) MakeJpegError("unsupported JPEG feature: " s)
#define JpegErrorFormatError(s) MakeJpegError("unsupported JPEG feature: " s)
}
#endif
