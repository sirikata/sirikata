/*  Sirikata Utilities -- Message Packet Header Parser
 *  RoutableMessageBody.hpp
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
    uint32 mDestinationPort;
    uint32 mSourcePort;
    bool mHasDestinationObject;
    bool mHasSourceObject;
    bool mHasDestinationSpace;
    bool mHasSourceSpace;
    std::string mData;
    static unsigned char translateKey(int key, int message_type) {
        return key*8+message_type;//(2=length delimited)
    }
public:
    RoutableMessageHeader() {
        mDestinationPort=0;
        mSourcePort=0;
        mHasDestinationObject=mHasSourceObject=mHasDestinationSpace=mHasSourceSpace=false;
    }
    RoutableMessageHeader(const ObjectReference &destinationObject,
                          const ObjectReference &sourceObject):mDestinationObject(destinationObject),mSourceObject(sourceObject) {
        mDestinationPort=0;
        mSourcePort=0;
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
                  {
                      uint64 len=parseLength(curInput,size);
                      if (key==Sirikata::Protocol::MessageHeader::destination_port_field_tag) {
                          mDestinationPort=key;
                      }
                      if (key==Sirikata::Protocol::MessageHeader::source_port_field_tag) {
                          mSourcePort=key;
                      }
                  }
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
    static unsigned int getSize(uint64 uuid, uint32 field_tag) {
        unsigned int retval=2;
        field_tag/=16;
        uuid/=128;
        while(field_tag) {
            retval++;
            field_tag/=128;
        }
        while(uuid) {
            retval++;
            uuid/=128;
        }
        return retval;
    }
    static unsigned int getSize(const UUID&uuid, uint32 field_tag) {
        UUID::Data::const_iterator i,ib=uuid.getArray().begin(),ie=uuid.getArray().end();
        unsigned int retval=UUID::Data::static_size;
        for (i=ie;i!=ib;) {
            --i;
            if (*i)
                break;
            else
                --retval;
        }
        retval+=2;
        field_tag/=16;
        while(field_tag) {
            ++retval;
            field_tag/=128;
        }
        return retval;
    }    
    std::string::iterator copyKey(std::string&s,std::string::iterator output, unsigned int protoNum, unsigned int size, unsigned int message_type=2) const{
        if (size>=2) {
            --size;
            *output=translateKey(protoNum%16,message_type);
            if (protoNum/16) {
                *output=(unsigned char)(128+(unsigned char)*output);
                ++output;    
                *output=(protoNum/16)%128;
                if (protoNum/16/128) {
                    *output=(unsigned char)(128+(unsigned char)*output);
                    ++output;
                    *output=(protoNum/16/128)%128;
                    if (protoNum/16/128/128) {
                        *output=(unsigned char)(128+(unsigned char)*output);
                        ++output;
                        *output=(protoNum/16/128/128)%128;
                        if (protoNum/16/128/128/128) {
                            *output=(unsigned char)(128+(unsigned char)*output);                            
                            ++output;
                            *output=(protoNum/16/128/128/128)%128;
                        }
                        ++output;
                    }else {
                        ++output;
                    }
                }else {
                    ++output;
                }
            }else {
                ++output;
            }
        }
        return output;
    }
    std::string::iterator copyInt(std::string&s,std::string::iterator start_output, unsigned int protoNum, uint64 uuid, unsigned int size) const{
        std::string::iterator output=copyKey(s,start_output,protoNum,size,0);
        while (uuid) {
            *output=uuid%128+(uuid/128?128:0);
            uuid/=128;
            ++output;
        }
        return output;
    }
    std::string::iterator copyItem(std::string&s,std::string::iterator start_output, unsigned int protoNum, const UUID&uuid, unsigned int size) const{
        std::string::iterator output=copyKey(s,start_output,protoNum,size);
        if (output!=start_output) {
            size-=(output-start_output);
            *output=size;
            ++output;
            s.replace(output,output+size,(const char*)uuid.getArray().begin(),size);
            return output+size;
        }
        output+=size;
        return output;
    }
    bool AppendOrSerializeToString(std::string*s,size_t slength)const {
        size_t original_size=slength;
        size_t total_size=original_size+mData.length();
        size_t destinationObjectSize=0;
        size_t destinationSpaceSize=0;
        size_t sourceObjectSize=0;
        size_t sourceSpaceSize=0;
        unsigned int sourcePortSize=0;
        unsigned int destinationPortSize=0;
        if (mHasDestinationObject)total_size+=(destinationObjectSize=getSize(mDestinationObject.getAsUUID(),Sirikata::Protocol::MessageHeader::source_object_field_tag));
        if (mHasSourceObject)total_size+=(sourceObjectSize=getSize(mSourceObject.getAsUUID(),Sirikata::Protocol::MessageHeader::destination_object_field_tag));
        if (mDestinationPort)total_size+=(destinationPortSize=getSize(mDestinationPort,Sirikata::Protocol::MessageHeader::destination_port_field_tag));
        if (mSourcePort)total_size+=(sourcePortSize=getSize(mSourcePort,Sirikata::Protocol::MessageHeader::source_port_field_tag));
        if (mHasDestinationSpace)total_size+=(destinationSpaceSize=getSize(mDestinationSpace.getAsUUID(),Sirikata::Protocol::MessageHeader::source_space_field_tag));
        if (mHasSourceSpace)total_size+=(sourceSpaceSize=getSize(mSourceSpace.getAsUUID(),Sirikata::Protocol::MessageHeader::destination_space_field_tag));
        s->resize(total_size);
        std::string::iterator output=s->begin();
        output+=original_size;
        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::source_object_field_tag,mSourceObject.getAsUUID(),sourceObjectSize);
        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::destination_object_field_tag,mDestinationObject.getAsUUID(),destinationObjectSize);


        if (mSourcePort) {
            output=copyInt(*s,output,Sirikata::Protocol::MessageHeader::source_port_field_tag,mSourcePort,sourcePortSize);
        }
        if (mDestinationPort) {
            output=copyInt(*s,output,Sirikata::Protocol::MessageHeader::destination_port_field_tag,mDestinationPort,destinationPortSize);
        }
        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::source_space_field_tag,mSourceSpace.getAsUUID(),sourceSpaceSize);
        output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::destination_space_field_tag,mDestinationSpace.getAsUUID(),destinationSpaceSize);
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
    inline uint32 source_port() const{
        return mSourcePort;
    }
    inline void set_source_port(uint32 port) {
        mSourcePort=port;
    }
    inline uint32 destination_port() const{
        return mDestinationPort;
    }
    inline void set_destination_port(uint32 port) {
        mDestinationPort=port;
    }
};
}
#endif
