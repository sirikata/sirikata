#include "Array.hpp"
namespace Iridium {
class SHA256 {
public:
    enum {static_size=32,hex_size=64};
private:
    Array<unsigned char,static_size> mData;
public:
    unsigned int size()const{
        return static_size;
    }
    /**
     * Allocates a std::string of size 64
     * \returns the std::string filled with mData converted to hex
     */
    std::string convertToHexString()const;
    /**
     * \returns an array filled with mData converted to hex
     */
    Array<char,hex_size> convertToHex()const;
    /**
     * \returns raw SHA256sum
     */
    const Array<unsigned char, static_size> &rawData()const {
        return mData;
    }

	bool operator==(const SHA256& other) const {
        return mData==other.mData;
    }
	bool operator!=(const SHA256& other) const {
        return !(mData==other.mData);
    }
	bool operator<(const SHA256& other) const{
        return mData<other.mData;
    }
    /**
     * Looks at the first 64 characters and pairs them into 32 bytes
     * Creates a shasum with these 32 raw bytes
     * \param digest must be 64 elements of hex digest
     * \throws std::invalid_argument if nonhex character encountered
     */
    static SHA256 convertFromHex(const char*digest);
    /**
     * Looks at the first 64 characters and pairs them into 32 bytes
     * \param digest must be a string of length 64
     * \throws std::invalid_argument if nonhex character encountered 
     */
    static SHA256 convertFromHex(const std::string&digest);
    /**
     * Creates a shasum with these 32 raw bytes
     * \param digest holds the array of bytes
     */
    static SHA256 convertFromBinary(const Array<unsigned char,static_size>&digest) {
        return convertFromBinary(digest.data());
    }
    /**
     * Looks at the first 32 bytes
     * Creates a shasum with these 32 raw bytes
     * \param digest must be exactly 32 bytes long
     */    
    static SHA256 convertFromBinary(const void*digest){
        SHA256 retval;
        memcpy(retval.mData.data(),digest,static_size);
        return retval;
    }
    /**
     * Computes SHA256 digest from data.
     * \param data contains data to be hashed
     * \param length is the length of the data to be hashed
     * \returns SHASum digest
     */
    static SHA256 computeDigest(const void *data, size_t length);
    /**
     * Computes SHA256 digest from data.
     * \param data contains data to be hashed
     * \returns SHASum digest
     */
    static SHA256 computeDigest(const std::string&data);
    /**
     * Fills the SHA256 with array of entirely 0's.
     */
    const SHA256&nil(){
        static SHA256 nil;
        static char empty_array[static_size]={0};
        static void* result=memcpy(nil.mData.data(),empty_array,static_size);
        return nil;
    }
    /**
     * Computes the digest of an empty file and returns that digest
     */
    const SHA256&emptyDigest() {
        static SHA256 empty=computeDigest(NULL,0);
        return empty;
    }
    
};
}
