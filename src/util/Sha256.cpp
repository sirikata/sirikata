#include "Sha256.hpp"
#include "internal_sha2.hpp"
#include <stdexcept>
namespace Iridium {
static unsigned char numToHex(unsigned int num) {
    if (num<10)
        return num+'0';
    if (num<16)
        return (num-10)+'a';
    return 'x';
}
static unsigned int numFromHex(unsigned char charValue) {
    if (charValue>='0'&&charValue<='9')
        return charValue-'0';
    if (charValue>='a'&&charValue<='f')
        return charValue-'a';
    if (charValue>='A'&&charValue<='F')
        return charValue-'A';
    static char error_message[]="Charater \'X\' not a hexidecimal number";
     error_message[10]=charValue;
    //not thread safe--but you'll get at least one of the error messages
    throw std::invalid_argument(error_message);
    return 0;
}
std::string SHA256::convertToHexString() const {
    std::string retval;
    retval.resize(hex_size);
    for (unsigned int i=0;i<static_size;++i) {
        retval[i*2]=numToHex(mData[i]/16);
        retval[i*2+1]=numToHex(mData[i]%16);        
    }
    return retval;
}

Array<char,SHA256::hex_size> SHA256::convertToHex() const {
    Array<char,hex_size> retval;
    for (unsigned int i=0;i<static_size;++i) {
        retval[i*2]=numToHex(mData[i]/16);
        retval[i*2+1]=numToHex(mData[i]%16);        
    }
    return retval;
}
SHA256 SHA256::convertFromHex(const char*data){
    SHA256 retval;
    for (unsigned int i=0;i<static_size;++i) {
        retval.mData[i]=numFromHex(data[i*2])*16+numFromHex(data[i*2+1]);
    }
    return retval;
}
SHA256 SHA256::convertFromHex(const std::string&data){
    if (data.length()!=64)
        throw std::invalid_argument("input string was the wrong length for a SHA256 Digest");
    return convertFromHex(data.data());
}
SHA256 SHA256::computeDigest(const void*data, size_t length) {
    SHA256 retval;
    Iridium::Util::Internal::SHA256_CTX context;
    Iridium::Util::Internal::SHA256_Init(&context);
    Iridium::Util::Internal::SHA256_Update(&context, (const unsigned char*)data, length);
    Iridium::Util::Internal::SHA256_Final(retval.mData.data(), &context);
    return retval;
}
SHA256 SHA256::computeDigest(const std::string&data) {
    SHA256 retval;
    Iridium::Util::Internal::SHA256_CTX context;
    Iridium::Util::Internal::SHA256_Init(&context);
    Iridium::Util::Internal::SHA256_Update(&context, (const unsigned char*)data.data(), data.length());
    Iridium::Util::Internal::SHA256_Final(retval.mData.data(), &context);
    return retval;
}
}
