#ifndef _CRAQ_ENTRY_HPP_
#define _CRAQ_ENTRY_HPP_
namespace CBR {
enum {
    CRAQ_SERVER_SIZE             =    10
};
class CraqEntry {
    uint32 mServer;
    float mRadius;
public:
    CraqEntry(uint32 server, float radius) {
        mServer=server;
        mRadius=radius;
    }
    static CraqEntry null() {
        return CraqEntry(NullServerID,0);
    }
    bool isNull()const {
        return mServer==NullServerID&&mRadius==0;
    }
    bool notNull()const {
        return !isNull();
    }
    explicit CraqEntry(unsigned char input[CRAQ_SERVER_SIZE]){
        deserialize(input);
    }
    void serialize(unsigned char output[CRAQ_SERVER_SIZE])const;
    std::string serialize() const{
        char output[CRAQ_SERVER_SIZE];
        serialize((unsigned char*)output);
        return std::string(output,CRAQ_SERVER_SIZE);
    }
    void deserialize(unsigned char input[CRAQ_SERVER_SIZE]);
    
    uint32 server() const{
        return mServer;
    }
    float radius () const{
        return mRadius;
    }
    
};
}
#endif
