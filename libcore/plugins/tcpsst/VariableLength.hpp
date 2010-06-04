/*  Sirikata -- Variable Length Serializable Integer With 0xff delimiter
 *  VariableLength.hpp
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
class VariableLength : protected vuint32 {
    uint8 mDelimiter;
public:
    enum {
        MAX_SERIALIZED_LENGTH=vuint32::MAX_SERIALIZED_LENGTH+1
    };

    bool operator == (const VariableLength&other)const {
        return mData==other.mData;
    }
    bool operator != (const VariableLength&other)const {
        return mData!=other.mData;
    }
    bool operator < (const VariableLength&other)const {
        return mData<other.mData;
    }
    uint32 read() {
        return vuint32::read();
    }
    unsigned int serializedSize() const {
        return vuint32::serializedSize()+1;
    }
    unsigned int serialize(uint8 *destination, unsigned int maxsize) const {
        if (maxsize>=1) {
            *destination=mDelimiter;
            return vuint32::serialize(destination+1,maxsize-1)+1;
        }else return false;
    }
    bool unserialize(const uint8*src, unsigned int&size) {
        if (size==0) return false;
        if (*src<128) return false;
        mDelimiter=*src;
        bool retval= vuint32::unserialize(src+1,size);
        size++;
        return retval;
    }

    VariableLength(){
        mData=0;
        mDelimiter=0xff;
    }
    VariableLength(uint32 num) {
        mData=num;
        mDelimiter=0xff;
    }
    struct Hasher:public vuint32::Hasher {

    };
};
}
