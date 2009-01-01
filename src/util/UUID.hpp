/*     Iridium Utilities -- Iridium Synchronization Utilities
 *  UUID.hpp
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
 *  * Neither the name of Iridium nor the names of its contributors may
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
#include "Array.hpp"

namespace boost_{
class uuid;
}

namespace Iridium {
class UUID {
public:
    enum {static_size=16};
    typedef unsigned char byte;
    typedef Array<byte,static_size,true> Data;
    typedef Data::iterator iterator;
    typedef Data::const_iterator const_iterator;    
private:
    Data mData;
public:
    UUID(const boost_::uuid&);
    UUID() {}
    UUID (const byte *data,
          unsigned int length){
        mData.memcpy(data,length);        
    }
    UUID(const Data data):mData(data) {
    }
    /**
     * Interprets the human readable UUID string using boost functions
     */
    UUID(const std::string&);
    class Random{};
    UUID(Random);
    static UUID random();
    const Data& getArray()const{return mData;}
    UUID & operator=(const UUID & other) { mData = other.mData; return *this; }
    UUID & operator=(const Data & other) { mData = other; return *this; }
    bool operator<(const UUID &other) {return mData < other.mData;}
    bool operator==(const UUID &other) {return mData == other.mData;}
    bool isNil()const{return mData==Data::nil();}
    static const UUID& nil(){
        static UUID retval(Data::nil());
        return retval;
    }
    unsigned int hash() const;
    std::string rawHexData()const;
    std::string readableHexData()const;
};

}
std::istream & operator>>(std::istream & is, Iridium::UUID & uuid);
std::ostream & operator<<(std::ostream & os, const Iridium::UUID & uuid);
