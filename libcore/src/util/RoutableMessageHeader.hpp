/*  Sirikata Utilities -- Message Packet Header Parser
 *  RoutableMessageHeader.hpp
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
#define _SIRIKATA_ROUTABLE_MESSAGE_HEADER_HPP_
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

    uint64 mMessageId;
    uint64 mMessageReplyId;
    uint32 mReturnStatus;
    bool mHasMessageId;
    bool mHasMessageReplyId;

    std::string mData;
public:
    typedef Protocol::MessageHeader::ReturnStatus ReturnStatus;
    static const ReturnStatus SUCCESS=Protocol::MessageHeader::SUCCESS;
    static const ReturnStatus NETWORK_FAILURE=Protocol::MessageHeader::NETWORK_FAILURE;
    static const ReturnStatus TIMEOUT_FAILURE=Protocol::MessageHeader::TIMEOUT_FAILURE;
    static const ReturnStatus PROTOCOL_ERROR=Protocol::MessageHeader::PROTOCOL_ERROR;
    static const ReturnStatus PORT_FAILURE=Protocol::MessageHeader::PORT_FAILURE;
    static const ReturnStatus UNKNOWN_OBJECT=Protocol::MessageHeader::UNKNOWN_OBJECT;

    RoutableMessageHeader() {
        mDestinationPort=0;
        mSourcePort=0;
        mHasDestinationObject=mHasSourceObject=mHasDestinationSpace=mHasSourceSpace=false;
        mHasMessageId=mHasMessageReplyId=false;
        mReturnStatus = SUCCESS;
    }
    RoutableMessageHeader(const ObjectReference &destinationObject,
                          const ObjectReference &sourceObject):mDestinationObject(destinationObject),mSourceObject(sourceObject) {
        mDestinationPort=0;
        mSourcePort=0;
        mHasDestinationObject=mHasSourceObject=true;
        mHasDestinationSpace=mHasSourceSpace=false;
        mHasMessageId=mHasMessageReplyId=false;
        mReturnStatus = SUCCESS;
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
        if (len<=size) {
            unsigned char uuidArray[UUID::static_size]={0};
            memcpy(uuidArray,input,len<UUID::static_size?len:UUID::static_size);
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
                //std::cout << "Key "<<key<<" is not reserved. size="<<size<<", pos="<<(curInput-(const unsigned char *)input)<<"\n";
                curInput=dataStart;
                size=oldSize;
                break;
            }
            unsigned int type=keyType%8;
            switch(type) {
              case 0:
                  {
                      uint64 value=parseLength(curInput,size);
                      if (key==Sirikata::Protocol::MessageHeader::destination_port_field_tag) {
                          mDestinationPort=value;
                          continue;
                      }
                      if (key==Sirikata::Protocol::MessageHeader::source_port_field_tag) {
                          mSourcePort=value;
                          continue;
                      }
                      if (key==Sirikata::Protocol::MessageHeader::id_field_tag) {
                          mHasMessageId=true;
                          mMessageId=value;
                          continue;
                      }
                      if (key==Sirikata::Protocol::MessageHeader::reply_id_field_tag) {
                          mHasMessageReplyId=true;
                          mMessageReplyId=value;
                          continue;
                      }
                      if (key==Sirikata::Protocol::MessageHeader::return_status_field_tag) {
                          mReturnStatus=value;
                          continue;
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
    static unsigned int getIntSize(uint32 myint) {
        unsigned int retval = 0;
        do {
            retval++;
            myint/=128;
        } while(myint);
        return retval;
    }
    static unsigned int getFieldSize(uint32 field_tag) {
        return getIntSize(field_tag*8);
    }
    static unsigned int getSize(uint64 uuid, uint32 field_tag) {
        return getIntSize(uuid) + getFieldSize(field_tag);
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
        retval++; // UUIDs are smaller than 127, length fits in one byte.
        retval += getFieldSize(field_tag);
        return retval;
    }

    std::string::iterator copyIntValue(std::string&s,std::string::iterator output, unsigned int value, unsigned int &size) const{
        do {
            assert(size >= 2);
            --size;
            *output = (value & 127) + (value/128?128:0);
            value >>= 7;
            ++output;
        } while (value);
        return output;
    }
    inline std::string::iterator copyKey(std::string&s,std::string::iterator output, unsigned int protoNum, unsigned int size, unsigned int message_type=2) const{
        return copyIntValue(s, output, (protoNum << 3) | message_type, size);
    }
    std::string::iterator copyInt(std::string&s,std::string::iterator start_output, unsigned int protoNum, uint64 uuid, unsigned int size) const{
        std::string::iterator output=copyKey(s,start_output,protoNum,size,0);
        return copyIntValue(s, output, uuid, size);
    }
    std::string::iterator copyItem(std::string&s,std::string::iterator start_output, unsigned int protoNum, const UUID&uuid, unsigned int size) const{
        std::string::iterator output=copyKey(s,start_output,protoNum,size,2);
        assert (output!=start_output);

        size-=(output-start_output);
        assert(size>0 && size<=17);
        size--;
        *output=size;
        ++output;
        s.replace(output,output+size,(const char*)uuid.getArray().begin(),size);
        return output+size;
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
        unsigned int sendidSize=0, replyidSize=0, retstatusSize=0;
        if (mHasDestinationObject)total_size+=(destinationObjectSize=getSize(mDestinationObject.getAsUUID(),Sirikata::Protocol::MessageHeader::source_object_field_tag));
        if (mHasSourceObject)total_size+=(sourceObjectSize=getSize(mSourceObject.getAsUUID(),Sirikata::Protocol::MessageHeader::destination_object_field_tag));
        if (mDestinationPort)total_size+=(destinationPortSize=getSize(mDestinationPort,Sirikata::Protocol::MessageHeader::destination_port_field_tag));
        if (mSourcePort)total_size+=(sourcePortSize=getSize(mSourcePort,Sirikata::Protocol::MessageHeader::source_port_field_tag));
        if (mHasDestinationSpace)total_size+=(destinationSpaceSize=getSize(mDestinationSpace.getAsUUID(),Sirikata::Protocol::MessageHeader::source_space_field_tag));
        if (mHasSourceSpace)total_size+=(sourceSpaceSize=getSize(mSourceSpace.getAsUUID(),Sirikata::Protocol::MessageHeader::destination_space_field_tag));
        if (mHasMessageId)total_size+=(sendidSize=getSize(mMessageId,Sirikata::Protocol::MessageHeader::id_field_tag));
        if (mHasMessageReplyId)total_size+=(replyidSize=getSize(mMessageReplyId,Sirikata::Protocol::MessageHeader::reply_id_field_tag));
        if (mReturnStatus!=SUCCESS)total_size+=(retstatusSize=getSize(mReturnStatus,Sirikata::Protocol::MessageHeader::return_status_field_tag));
        s->resize(total_size);
        std::string::iterator output=s->begin();
        output+=original_size;

        if (mHasSourceObject) {
            output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::source_object_field_tag,mSourceObject.getAsUUID(),sourceObjectSize);
        }
        if (mHasDestinationObject) {
            output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::destination_object_field_tag,mDestinationObject.getAsUUID(),destinationObjectSize);
        }
        if (mSourcePort) {
            output=copyInt(*s,output,Sirikata::Protocol::MessageHeader::source_port_field_tag,mSourcePort,sourcePortSize);
        }
        if (mDestinationPort) {
            output=copyInt(*s,output,Sirikata::Protocol::MessageHeader::destination_port_field_tag,mDestinationPort,destinationPortSize);
        }
        if (mHasSourceSpace) {
            output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::source_space_field_tag,mSourceSpace.getAsUUID(),sourceSpaceSize);
        }
        if (mHasDestinationSpace) {
            output=copyItem(*s,output,Sirikata::Protocol::MessageHeader::destination_space_field_tag,mDestinationSpace.getAsUUID(),destinationSpaceSize);
        }
        if (mHasMessageId) {
            output=copyInt(*s,output,Sirikata::Protocol::MessageHeader::id_field_tag,mMessageId,sendidSize);
        }
        if (mHasMessageReplyId) {
            output=copyInt(*s,output,Sirikata::Protocol::MessageHeader::reply_id_field_tag,mMessageReplyId,replyidSize);
        }
        if (mReturnStatus!=SUCCESS) {
            output=copyInt(*s,output,Sirikata::Protocol::MessageHeader::return_status_field_tag,mReturnStatus,retstatusSize);
        }
        if (output!=s->end()) {
            assert(s->end()-output==(ptrdiff_t)mData.size());
            s->replace(output,s->end(),mData);
        }
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
    inline const ObjectReference& destination_object() const {
        assert(mHasDestinationObject);
        return mDestinationObject;
    }
    inline void set_destination_object(const ObjectReference &value) {
        mDestinationObject=value;
        mHasDestinationObject=true;
    }
    inline void clear_destination_space() {mHasDestinationSpace=false;}
    inline bool has_destination_space() const {return mHasDestinationSpace;}
    inline const SpaceID& destination_space() const {
        assert(mHasDestinationSpace);
        return mDestinationSpace;
    }
    inline void set_destination_space(const SpaceID &value) {
        mDestinationSpace=value;
        mHasDestinationSpace=true;
    }

    void swap_source_and_destination() {
        std::swap(mHasSourceObject,mHasDestinationObject);
        std::swap(mSourceObject,mDestinationObject);
        std::swap(mHasSourceSpace,mHasDestinationSpace);
        std::swap(mSourceSpace,mDestinationSpace);
        //std::swap(mHasSourcePort,mHasDestinationPort);
        std::swap(mSourcePort,mDestinationPort);

        mHasMessageReplyId = mHasMessageId;
        mMessageReplyId = mMessageId;
        mHasMessageId = false;
    }
    inline void clear_source_object() {mHasSourceObject=false;}
    inline bool has_source_object() const {return mHasSourceObject;}
    inline const ObjectReference& source_object() const {
        assert(mHasSourceObject);
        return mSourceObject;
    }
    inline void set_source_object(const ObjectReference &value) {
        mSourceObject=value;
        mHasSourceObject=true;
    }
    inline void clear_source_space() {mHasSourceSpace=false;}
    inline bool has_source_space() const {return mHasSourceSpace;}
    inline const SpaceID& source_space() const {
        assert(mHasSourceSpace);
        return mSourceSpace;
    }
    inline void set_source_space(const SpaceID &value) {
        mSourceSpace=value;
        mHasSourceSpace=true;
    }
    inline MessagePort source_port() const{
        return mSourcePort;
    }
    inline void set_source_port(MessagePort port) {
        mSourcePort=port;
    }
    inline MessagePort destination_port() const{
        return mDestinationPort;
    }
    inline void set_destination_port(MessagePort port) {
        mDestinationPort=port;
    }

    inline int64 id() const{
        assert(mHasMessageId);
        return mMessageId;
    }
    inline bool has_id() const{
        return mHasMessageId;
    }
    inline void set_id(int64 id) {
        mMessageId=id;
        mHasMessageId = true;
    }
    inline void clear_id() {
        mHasMessageId = false;
    }

    inline int64 reply_id() const{
        assert(mHasMessageReplyId);
        return mMessageReplyId;
    }
    inline bool has_reply_id() const{
        return mHasMessageReplyId;
    }
    inline void set_reply_id(int64 id) {
        mMessageReplyId=id;
        mHasMessageReplyId = true;
    }
    inline void clear_reply_id() {
        mHasMessageReplyId = false;
    }

    inline ReturnStatus return_status() const{
        return (ReturnStatus)mReturnStatus;
    }
    inline bool has_return_status() const{
        return mReturnStatus != SUCCESS;
    }
    inline void set_return_status(int32 status) {
        mReturnStatus=status;
    }
    inline void set_return_status(ReturnStatus status) {
        mReturnStatus=(int32)status;
    }

};

typedef RoutableMessageHeader::ReturnStatus ReturnStatus;

}
#endif
