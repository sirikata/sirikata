/*  Sirikata -- Variable Length Serializable Integer
 *  VInt.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


namespace Sirikata {
template <class T> class VInt : public TotallyOrdered<T> {
protected:
    T mData;
public:
    enum MaxSerializedLengthConstraints {
        MAX_SERIALIZED_LENGTH=sizeof(T)+sizeof(T)/8+((sizeof(T)%8==0)?0:1)
    };
    bool odd() const {
        return mData&1;
    }
    T read() const {
        return mData;
    }
    uint64 readRawData()const {
        uint64 ander=0;
        for (size_t i=0;i<sizeof(T);++i) {
            ander<<=8;
            ander|=255;
        }
        uint64 retval=(uint64)mData;
        retval&=ander;
        return retval;
    }
    unsigned int serializedSize() const {
        uint64 tmp=readRawData();
        unsigned int retval=0;
        do {
            retval++;
            tmp/=128;
        }while (tmp);
        return retval;
    }
    unsigned int serialize(uint8 *destination, unsigned int maxsize) const {
        uint64 tmp=readRawData();
        unsigned int retval=0;
        do {
            if (retval>=maxsize) return 0;
            *destination = tmp&127;
            retval++;
            tmp/=128;
        }while (tmp&&(*destination++|=128));
        return retval;
    }
    bool unserialize(const uint8*src, unsigned int&size) {
        if (size==0) return false;
        unsigned int maxsize=size;
        uint64 retval=0;
        uint64 temp=0;
        for (size=0;size<maxsize;++size) {
            temp=src[size];
            bool endLoop=(temp&128)==0;
            temp&=127;
            temp<<=(size*7);
            retval|=temp;
            if (endLoop) {
                mData=(T)retval;
                ++size;
                return true;
            }
        }
        return false;
    }

    VInt(){
        mData=0;
    }
    VInt(T num) {
        mData=num;
    }

    bool operator == (const VInt<T>&other)const {
        return mData==other.mData;
    }
    bool operator != (const VInt<T>&other)const {
        return mData!=other.mData;
    }
    bool operator < (const VInt<T>&other)const {
        return mData<other.mData;
    }    
    /// Hasher functor to be used in a hash_map.
    struct Hasher {
        ///returns a hash of the integer value
        size_t operator() (const VInt<T> &id) const{
            return std::tr1::hash<T>()(id.mData);
        }
    };    
};

}

