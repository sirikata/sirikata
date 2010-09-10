/*  Sirikata Network Utilities
 *  ASIOStreamBuilder.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Asio.hpp>
#include "TCPStream.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "TCPSetCallbacks.hpp"
#include "TCPStreamListener.hpp"
#include "ASIOReadBuffer.hpp"

#include <boost/lexical_cast.hpp>
#include <openssl/md5.h>

namespace Sirikata {
namespace Network {
namespace ASIOStreamBuilder{

typedef Array<uint8,TCPStream::MaxWebSocketHeaderSize> TcpSstHeaderArray;

class IncompleteStreamState {
public:
    int mNumSockets;
    std::vector<TCPSocket*>mSockets;
    std::map<TCPSocket*, std::string> mWebSocketResponses;
};

namespace {
typedef std::map<UUID,IncompleteStreamState> IncompleteStreamMap;
std::deque<UUID> sStaleUUIDs;
IncompleteStreamMap sIncompleteStreams;

int64 getObscuredNumber(const std::string& key) {
    std::string filtered;
    for(int32 i = 0; i < key.size(); i++)
        if (key[i] >= '0' && key[i] <= '9')
            filtered.push_back(key[i]);
    return boost::lexical_cast<int64>(filtered);
}

int64 getNumSpaces(const std::string& key) {
    int32 result = 0;
    for(int32 i = 0; i < key.size(); i++)
        if (key[i] == ' ') result++;
    return result;
}

std::string getWebSocketSecReply(const std::string& key1, const std::string& key2, const std::string key3) {
    char magic_concat[16];
    int32* magic_1 = (int32*)magic_concat;
    int32* magic_2 = (int32*)(magic_concat + 4);
    char* magic_bytes = magic_concat + 8;

    *magic_1 = htonl(getObscuredNumber(key1) / getNumSpaces(key1));
    *magic_2 = htonl(getObscuredNumber(key2) / getNumSpaces(key2));
    assert(key3.size() == 8);
    memcpy(magic_bytes, &(key3[0]), 8);

    // FIXME md5 hash
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5((unsigned char*) magic_concat, 16, result);

    return std::string((const char*)result, MD5_DIGEST_LENGTH);
}

}

///gets called when a complete 24 byte header is actually received: uses the UUID within to match up appropriate sockets
void buildStream(TcpSstHeaderArray *buffer,
                 TCPSocket *socket,
                 std::tr1::shared_ptr<TCPStreamListener::Data> data,
                 const boost::system::error_code &error,
                 std::size_t bytes_transferred)
{
    // Buffer always needs to be cleaned up when we get out of this method
    std::auto_ptr<TcpSstHeaderArray> buffer_ptr(buffer);

    // Sanity check start
    if (error || bytes_transferred < 5 || std::string((const char*)buffer->begin(), 5) != std::string("GET /")) {
        SILOG(tcpsst,warning,"Connection received with truncated header: "<<error);
        delete socket;
        return;
    }

    // Sanity check end: 8 bytes from WebSocket spec after headers, then
    // \r\n\r\n before that.
    std::string buffer_str((const char*)buffer->begin(), bytes_transferred);
    if (buffer_str[ bytes_transferred - 12] != '\r' ||
        buffer_str[ bytes_transferred - 11] != '\n' ||
        buffer_str[ bytes_transferred - 10] != '\r' ||
        buffer_str[ bytes_transferred - 9] != '\n')
    {
        SILOG(tcpsst,warning,"Request doesn't end properly:\n" << buffer_str << "\n");
        delete socket;
        return;
    }

    std::string headers_str = buffer_str.substr(0, bytes_transferred - 10);
    // Parse headers
    UUID context;
    std::map<std::string, std::string> headers;
    int offset = 0;
    while(offset < (int)headers_str.size()) {
        int last_offset = offset;
        offset = headers_str.find("\r\n", offset);
        if (offset == std::string::npos) {
            SILOG(tcpsst,warning,"Error parsing headers.");
            delete socket;
            return;
        }

        std::string line = headers_str.substr(last_offset, offset - last_offset);

        // Special case the initial GET line
        if (line.substr(0, 5) == "GET /") {
            int uuid_end = line.find(' ', 5);
            if (uuid_end == std::string::npos) {
                SILOG(tcpsst,warning,"Error parsing headers: invalid get line.");
                delete socket;
                return;
            }
            std::string uuid_str = line.substr(5, uuid_end - 5);
            try {
                context = UUID(uuid_str,UUID::HumanReadable());
            } catch(...) {
                SILOG(tcpsst,warning,"Error parsing headers: invalid get uuid.");
                delete socket;
                return;
            }

            offset += 2;
            continue;
        }

        int colon = line.find(":");
        if (colon == std::string::npos) {
            SILOG(tcpsst,warning,"Error parsing headers: missing colon.");
            delete socket;
            return;
        }
        std::string head = line.substr(0, colon);
        std::string val = line.substr(colon+2);

        headers[head] = val;

        // Advance to get past the \r\n
        offset += 2;
    }

    if (headers.find("Host") == headers.end() ||
        headers.find("Origin") == headers.end() ||
        headers.find("Sec-WebSocket-Key1") == headers.end() ||
        headers.find("Sec-WebSocket-Key2") == headers.end())
    {
        SILOG(tcpsst,warning,"Connection request didn't specify all required fields.");
        delete socket;
        return;
    }

    std::string host = headers["Host"];
    std::string origin = headers["Origin"];
    std::string protocol = "wssst1";
    if (headers.find("Sec-WebSocket-Protocol") != headers.end())
        protocol = headers["Sec-WebSocket-Protocol"];
    std::string key1 = headers["Sec-WebSocket-Key1"];
    std::string key2 = headers["Sec-WebSocket-Key2"];
    std::string key3 = buffer_str.substr(bytes_transferred - 8);
    assert(key3.size() == 8);

    std::string reply_str = getWebSocketSecReply(key1, key2, key3);

    bool binaryStream=protocol.find("sst")==0;
    bool base64Stream=!binaryStream;
    boost::asio::ip::tcp::no_delay option(data->mNoDelay);
    socket->set_option(option);
    IncompleteStreamMap::iterator where=sIncompleteStreams.find(context);

    unsigned int numConnections=1;

    for (std::string::iterator i=protocol.begin(),ie=protocol.end();i!=ie;++i) {
        if (*i>='0'&&*i<='9') {
            char* endptr=NULL;
            const char *start=protocol.c_str();
            size_t offset=(i-protocol.begin());
            start+=offset;
            numConnections=strtol(start,&endptr,10);
            size_t numberlen=endptr-start;
            if (numConnections>data->mMaxSimultaneousSockets) {
                numConnections=data->mMaxSimultaneousSockets;
                char numcon[256];
                sprintf(numcon,"%d",numConnections);
                protocol=protocol.substr(0,offset)+numcon+protocol.substr(offset+numberlen);
            }
            break;
        }
    }

    if (where==sIncompleteStreams.end()){
        sIncompleteStreams[context].mNumSockets=numConnections;
        where=sIncompleteStreams.find(context);
        assert(where!=sIncompleteStreams.end());
    }
    if ((int)numConnections!=where->second.mNumSockets) {
        SILOG(tcpsst,warning,"Single client disagrees on number of connections to establish: "<<numConnections<<" != "<<where->second.mNumSockets);
        sIncompleteStreams.erase(where);
    }else {
        where->second.mSockets.push_back(socket);
        where->second.mWebSocketResponses[socket] = reply_str;
        if (numConnections==(unsigned int)where->second.mSockets.size()) {
            MultiplexedSocketPtr shared_socket(
                MultiplexedSocket::construct<MultiplexedSocket>(&data->ios,context,data->cb,base64Stream));
            shared_socket->initFromSockets(where->second.mSockets,data->mSendBufferSize);
            std::string port=shared_socket->getASIOSocketWrapper(0).getLocalEndpoint().getService();
            std::string resource_name='/'+context.toString();
            MultiplexedSocket::sendAllProtocolHeaders(shared_socket,origin,host,port,resource_name,protocol, where->second.mWebSocketResponses);
            sIncompleteStreams.erase(where);


            Stream::StreamID newID=Stream::StreamID(1);
            TCPStream * strm=new TCPStream(shared_socket,newID);

            TCPSetCallbacks setCallbackFunctor(&*shared_socket,strm);
            data->cb(strm,setCallbackFunctor);
            if (setCallbackFunctor.mCallbacks==NULL) {
                SILOG(tcpsst,error,"Client code for stream "<<newID.read()<<" did not set listener on socket");
                shared_socket->closeStream(shared_socket,newID);
            }
        }else{
            sStaleUUIDs.push_back(context);
        }
    }
}

namespace {
class CheckWebSocketRequest {
    const Array<uint8,TCPStream::MaxWebSocketHeaderSize> *mArray;
    typedef boost::system::error_code ErrorCode;
    uint32 mTotal;
public:
    CheckWebSocketRequest(const Array<uint8,TCPStream::MaxWebSocketHeaderSize>*array) {
        mArray=array;
        mTotal = 0;
    }
    size_t operator() (const ErrorCode& error, size_t bytes_transferred) {
        if (error) return 0;

        mTotal += bytes_transferred;

        if (mTotal >= 12 &&
            (*mArray)[mTotal - 12] == '\r' &&
            (*mArray)[mTotal - 11] == '\n' &&
            (*mArray)[mTotal - 10] == '\r' &&
            (*mArray)[mTotal - 9] == '\n')
        {
            return 0;
        }
        return 65536;
    }
};
} // namespace

void beginNewStream(TCPSocket*socket, std::tr1::shared_ptr<TCPStreamListener::Data> data) {

    TcpSstHeaderArray *buffer = new TcpSstHeaderArray;

    boost::asio::async_read(*socket,
                            boost::asio::buffer(buffer->begin(),(int)TCPStream::MaxWebSocketHeaderSize>(int)ASIOReadBuffer::sBufferLength?(int)ASIOReadBuffer::sBufferLength:(int)TCPStream::MaxWebSocketHeaderSize),
                            CheckWebSocketRequest (buffer),
                            std::tr1::bind(&ASIOStreamBuilder::buildStream,buffer,socket,data,_1,_2));
}

} // namespace ASIOStreamBuilder
} // namespace Network
} // namespace Sirikata
