namespace Sirikata { namespace Network {
class CheckWebSocketHeader {


    class CaseInsensitive {
    public:
        bool operator() (const uint8 &left, const uint8 &right) {
            return std::toupper(left) == std::toupper(right);
        }
    };
    const Array<uint8,TCPStream::MaxWebSocketHeaderSize> *mArray;
    typedef boost::system::error_code ErrorCode;
    uint32 mTotal;
    bool mClient;
public:
    CheckWebSocketHeader(const Array<uint8,TCPStream::MaxWebSocketHeaderSize>*array, bool client) {
        mArray=array;
        mTotal = 0;
        mClient=client;
    }
    size_t operator() (const ErrorCode& error, size_t bytes_transferred) {
        if (error) return 0;
        uint8 target[]="\r\n\r\n";
        const uint8 *version=mClient?(const uint8*)"sec-websocket-accept":(const uint8*)"sec-websocket-version";
        const uint8 * arrayEnd= mArray->begin()+bytes_transferred;
        const uint8 * where = std::search (mArray->begin(),arrayEnd,target,target+4);
        if (where==arrayEnd)
            return TCPStream::MaxWebSocketHeaderSize-bytes_transferred;
        mTotal += bytes_transferred;
        
        // Newer handshakes do not have data in the body.
        // We need to allow these to complete *before* data is received.
        
        // We check case-insensitive for Sec-WebSocket-Version: 13
        // Dumbed down due to lack of cross-platform case-insensitive
        // string functions
        
        const uint8 *whereVersion = std::search(mArray->begin(),arrayEnd, version,version+strlen((const char*)version),CaseInsensitive());
        if (whereVersion == arrayEnd) {
            size_t bytesAfterNewlines = mClient?16:8;
            return where+4+bytesAfterNewlines<=arrayEnd?0:TCPStream::MaxWebSocketHeaderSize-bytes_transferred;
        }
        return 0;
    }
};
}}
