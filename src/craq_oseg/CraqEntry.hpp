#ifndef _CRAQ_ENTRY_HPP_
#define _CRAQ_ENTRY_HPP_
namespace CBR {
class CraqEntry {
    uint32 mServer;
    float mRadius;
public:
    CraqEntry(uint32 server, float radius) {
        mServer=server;
        mRadius=radius;
    }
    CraqEntry(unsigned char input[CRAQ_SERVER_SIZE]){
        deserialize(input);
    }
    void serialize(unsigned char output[CRAQ_SERVER_SIZE]);
    std::string serialize() {
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
