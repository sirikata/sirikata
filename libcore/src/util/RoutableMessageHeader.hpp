/*  Sirikata Utilities -- Message Packet Header Parser
 *  Plugin.hpp
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
#ifndef _SIRIKATA_ROUTABLE_MESSAGE_HEADER_HPP_
#define _SIRIKATA_ROUTABLE_MESSAGE_HEADER_HPP_2
#include "SpaceObjectReference.hpp"
#include "Protocol_MessageHeader.pbj.hpp"
namespace Sirikata {
class RoutableMessageHeader {
    ObjectReference mDestinationObject;
    ObjectReference mSourceObject;
    SpaceID mDestinationSpace;
    SpaceID mSourceSpace;
    bool mHasDestinationObject;
    bool mHasSourceObject;
    bool mHasDestinationSpace;
    bool mHasSourceSpace;
    std::string mData;
    static unsigned char translateKey(int key) {
        return key*8+2;//(2=length delimited)
    }
public:
    RoutableMessageHeader() {
        mHasDestinationObject=mHasSourceObject=mHasDestinationSpace=mHasSourceSpace=false;
    }
    RoutableMessageHeader(const ObjectReference &destinationObject,
                          const ObjectReference &sourceObject):mDestinationObject(destinationObject),mSourceObject(sourceObject) {
        mHasDestinationObject=mHasSourceObject=true;
        mHasDestinationSpace=mHasSourceSpace=false;
    }
private:
    size_t parseLength(const unsigned char*&input, size_t&size) {
        size_t retval=0;
        size_t offset=1;
        while (size) {
            unsigned char cur=*input;
            input++;
            --size;
            size_t digit=(cur&127);
            retval+=digit*offset;
            if ((cur&128)==0)break;
            offset*=128;
        }
        return retval;
    }
    UUID parseUUID(const unsigned char*&input, size_t&size) {
        size_t len=parseLength(input,size);
        if (len>=size) {
            unsigned char uuidArray[UUID::static_size]={0};
            memcpy(uuidArray,input,len);
            input+=len;
            size-=len;
            return UUID(uuidArray,UUID::static_size);
        } else {
            input+=size;
            size=0;
        }
        return UUID::null();
    }
    bool isReserved(size_t key) {
        return Sirikata::Protocol::MessageHeader::within_reserved_field_tag_range(key);
    }
public:
    MemoryReference ParseFromArray(const void *input, size_t size) {
        const unsigned char*curInput=(const unsigned char*)input;
        while (size) {
            const unsigned char *dataStart=curInput;
            size_t oldSize=size;
            size_t keyType=parseLength(curInput,size);
            size_t key=(keyType/8);
            if (!isReserved(key)) {
                curInput=dataStart;
                size=oldSize;
                break;
            }
            unsigned int type=keyType%8;
            switch(type) {
              case 0:
                parseLength(curInput,size);
                break;
              case 1:
                if (size>=8) {
                    curInput+=8;
                    size-=8;
                } else {
                    curInput+=size;
                    size=0;
                }
                break;
              case 2:
                switch(key) {
                  case Sirikata::Protocol::MessageHeader::source_object_field_tag:
                    mHasSourceObject=true;
                    mSourceObject=ObjectReference(parseUUID(curInput,size));
                    continue;
                  case Sirikata::Protocol::MessageHeader::destination_object_field_tag:                  
                    mHasDestinationObject=true;
                    mDestinationObject=ObjectReference(parseUUID(curInput,size));
                    continue;
                  case Sirikata::Protocol::MessageHeader::source_space_field_tag:
                    mHasSourceSpace=true;
                    mSourceSpace=SpaceID(parseUUID(curInput,size));
                    continue;
                  case Sirikata::Protocol::MessageHeader::destination_space_field_tag:
                    mHasDestinationSpace=true;
                    mDestinationSpace=SpaceID(parseUUID(curInput,size));
                    continue;
                  default: {
                      size_t len=parseLength(curInput,size);
                      if (len>=size) {
                          size-=len;
                          curInput+=len;
                      }
                  }
                }
                break;
              case 5:
                if (size>=4) {
                    curInput+=4;
                    size-=4;
                }else {
                    curInput+=size;
                    size=0;
                }
                break;
            }
            if (curInput!=dataStart) {
                mData+=std::string((const char*)dataStart,(curInput-dataStart));//header fields go here
            }
        }
        return MemoryReference(curInput,size);
    }
    MemoryReference ParseFromString(const std::string& size){
        return ParseFromArray(size.data(),size.size());
    }
private:
    static unsigned int getSize(const UUID&uuid) {
        UUID::Data::const_iterator i,ib=uuid.getArray().begin(),ie=uuid.getArray().end();
        unsigned int retval=UUID::Data::static_size;
        for (i=ie;i!=ib;) {
            --i;
            if (*i)
                break;
            else
                --retval;
        }
        if (retval)
            retval+=2;//need field header if this UUID has any data
        return retval;
    }    
    static unsigned int getSize(const ObjectReference&uuid) {
        return getSize(uuid.getAsUUID());
    }
    static unsigned int getSize(const SpaceID&uuid) {
        return 1+getSize(uuid.getAsUUID());
    }
    std::string::iterator copyItem(std::string&s,std::string::iterator output, unsigned int protoNum, const UUID&uuid, unsigned int size) const{
        if (size>=2) {
            --size;
            *output=translateKey(protoNum);
            ++output;

            --size;
            *output=size;
            ++output;

            s.replace(output,output+size,(const char*)uuid.getArray().begin(),size);
            return output+size;
        }
        output+=size;
        return output;
    }
    std::string::iterator copyItem(std::string&s,std::string::iterator  output, unsigned int protoNum, const ObjectReference &uuid, unsigned int size)const{
        return copyItem(s,output,protoNum,uuid.getAsUUID(),size);
    }
    std::string::iterator copySpaceItem(std::string&s,std::string::iterator  output, unsigned int protoNum, const UUID&uuid, unsigned int size) const{
        if (size>=3) {
            --size;
            *output=128+translateKey(protoNum);
            ++output;

            --size;
            *output=protoNum/16;
            ++output;

            --size;
            *output=size;
            ++output;

            s.replace(output,output+size,(const char*)uuid.getArray().begin(),size);
            return output+size;
        }
        output+=size;
        return output;
    }
    std::string::iterator copyItem(std::string&s,std::string::iterator  output, unsigned int protoNum, const SpaceID &uuid, unsigned int size) const{
        return copySpaceItem(s,output,protoNum,uuid.getAsUUID(),size);
    }
    bool AppendOrSerializeToString(std::string*s,size_t slength)const {
        size_t original_size=slength;
        size_t total_size=original_size+mData.length();
        size_t destinationObjectSize=0;
        size_t destinationSpaceSize=0;
        size_t sourceObjectSize=0;
        size_t sourceSpaceSize=0;
        if (mHasDestinationObject)total_size+=(destinationObjectSize=getSize(mDestinationObject));
        if (mHasSourceObject)total_size+=(sourceObjectSize=getSize(mSourceObject));
        if (mHasDestinationSpace)total_size+=(destinationSpaceSize=getSize(mDestinationSpace));
        if (mHasSourceSpace)total_size+=(sourceSpaceSize=getSize(mSourceSpace));
        s->resize(total_size);
        std::string::iterator output=s->begin();
        output+=original_size;
        int source_object_field_tag_must_be_one_byte[32-1-2*Sirikata::Protocol::MessageHeader::source_object_field_tag];
        int destination_object_field_tag_must_be_one_byte[32-1-2*Sirikata::Protocol::MessageHeader::destination_object_field_tag];
        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::source_object_field_tag,mSourceObject,sourceObjectSize);
        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::destination_object_field_tag,mDestinationObject,destinationObjectSize);
        int source_object_field_tag_must_be_two_bytes[128*32-1-2*Sirikata::Protocol::MessageHeader::source_space_field_tag];
        int destination_object_field_tag_must_be_two_bytes[128*32-1-2*Sirikata::Protocol::MessageHeader::destination_space_field_tag];

        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::source_space_field_tag,mSourceSpace,sourceSpaceSize);
        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::destination_space_field_tag,mDestinationSpace,destinationSpaceSize);
        s->replace(output,s->end(),mData);
        return true;
    }
public:
    inline bool SerializeToString(std::string*s)const {
        return AppendOrSerializeToString(s,0);
    }
    inline bool AppendToString(std::string*s)const {
        return AppendOrSerializeToString(s,s->length());
    }
    inline void clear_destination_object() {mHasDestinationObject=false;}
    inline bool has_destination_object() const {return mHasDestinationObject;}
    inline const ObjectReference& destination_object() const {return mDestinationObject;}
    inline void set_destination_object(const ObjectReference &value) {
        mDestinationObject=value;
        mHasDestinationObject=true;
    }
    inline void clear_destination_space() {mHasDestinationSpace=false;}
    inline bool has_destination_space() const {return mHasDestinationSpace;}
    inline const SpaceID& destination_space() const {return mDestinationSpace;}
    inline void set_destination_space(const SpaceID &value) {
        mDestinationSpace=value;
        mHasDestinationSpace=true;
    }


    inline void clear_source_object() {mHasSourceObject=false;}
    inline bool has_source_object() const {return mHasSourceObject;}
    inline const ObjectReference& source_object() const {return mSourceObject;}
    inline void set_source_object(const ObjectReference &value) {
        mSourceObject=value;
        mHasSourceObject=true;
    }
    inline void clear_source_space() {mHasSourceSpace=false;}
    inline bool has_source_space() const {return mHasSourceSpace;}
    inline const SpaceID& source_space() const {return mSourceSpace;}
    inline void set_source_space(const SpaceID &value) {
        mSourceSpace=value;
        mHasSourceSpace=true;
    }

};
}
#endif
