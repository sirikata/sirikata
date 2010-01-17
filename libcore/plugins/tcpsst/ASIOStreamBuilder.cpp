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

#include "util/Platform.hpp"
#include "network/Asio.hpp"
#include "TCPStream.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "TCPSetCallbacks.hpp"
#include "TCPStreamListener.hpp"
#include "ASIOReadBuffer.hpp"
namespace Sirikata {
namespace Network {
namespace ASIOStreamBuilder{

typedef Array<uint8,TCPStream::MaxWebSocketHeaderSize> TcpSstHeaderArray;

class IncompleteStreamState {
public:
    int mNumSockets;
    std::vector<TCPSocket*>mSockets;
};

namespace {
typedef std::map<UUID,IncompleteStreamState> IncompleteStreamMap;
std::deque<UUID> sStaleUUIDs;
IncompleteStreamMap sIncompleteStreams;
}

///gets called when a complete 24 byte header is actually received: uses the UUID within to match up appropriate sockets
void buildStream(TcpSstHeaderArray *buffer,
                 TCPSocket *socket,
                 std::tr1::shared_ptr<TCPStreamListener::Data> data,
                 const boost::system::error_code &error,
                 std::size_t bytes_transferred) {
    if (error||bytes_transferred<4||buffer->begin()[0]!='G'||buffer->begin()[1]!='E'||buffer->begin()[2]!='T'||buffer->begin()[3]!=' '||buffer->begin()[4]!='/') {
        SILOG(tcpsst,warning,"Connection received with truncated header: "<<error);
        delete socket;
    }else {
        size_t whereHeaderEnds=3;
        for (;whereHeaderEnds<bytes_transferred;++whereHeaderEnds) {
            if ((*buffer)[whereHeaderEnds]=='\n'&&
                (*buffer)[whereHeaderEnds-1]=='\r'&&
                (*buffer)[whereHeaderEnds-2]=='\n'&&
                (*buffer)[whereHeaderEnds-3]=='\r') {
                break;
            }
        }
        if (whereHeaderEnds==bytes_transferred) {
            SILOG(tcpsst,warning,"Connection received with truncated header "<<std::string((const char*)buffer->begin(),bytes_transferred)<<"lacking CRLF");
            delete socket;
        } else if (whereHeaderEnds+1!=bytes_transferred){
            SILOG(tcpsst,warning,"Connection received with excess header "<<std::string((const char*)buffer->begin(),bytes_transferred)<<"lacking CRLF");
            delete socket;
        }else {
            const uint8* requestEnd=std::find(buffer->begin(),buffer->begin()+whereHeaderEnds,'\r');
            const uint8* requestStart=buffer->begin()+4;
            const uint8*nameValueFieldStart[6];
            const uint8*nameValueFieldEnd[6];
            const char*validBeginnings[6]={"GET ","Upgrade: ","Connection: ","Host: ","Origin: ","WebSocket-Protocol: "};
            nameValueFieldStart[0]=buffer->begin();
            nameValueFieldEnd[0]=requestEnd;
            std::string protocol="wssst1";
            std::string host;
            std::string origin;
            bool headerError=false;
            for (int i=1;i<=5;++i) {
                nameValueFieldStart[i]=nameValueFieldEnd[i-1]+1;
                if (nameValueFieldStart[i]<buffer->begin()+whereHeaderEnds) {
                    nameValueFieldEnd[i]=std::find(nameValueFieldStart[i],(const uint8*)(buffer->begin()+whereHeaderEnds),'\r');
                }else {
                    nameValueFieldEnd[i]=nameValueFieldStart[i];
                }
                if (nameValueFieldEnd[i]-nameValueFieldStart[i]>(int)strlen(validBeginnings[i])&&memcmp(nameValueFieldStart[i],validBeginnings[i],strlen(validBeginnings[i]))==0) {
                    nameValueFieldStart[i]+=strlen(validBeginnings[i]);
                }else {
                    nameValueFieldStart[i]=nameValueFieldEnd[i];
                    if (i!=5) {
                        headerError=true;
                    }
                }
            }
            UUID context (std::string((const char*)requestStart+1,16),UUID::HumanReadable());

            if (headerError) {
                SILOG(tcpsst,warning,"Connection received with header missing required fields "<<std::string((const char*)buffer->begin(),bytes_transferred));
                if (nameValueFieldEnd[3]!=nameValueFieldStart[3]) {
                    host=std::string((const char*)nameValueFieldStart[5],nameValueFieldEnd[5]-nameValueFieldStart[5]);
                }
                if (nameValueFieldEnd[5]!=nameValueFieldStart[5]) {
                    protocol=std::string((const char*)nameValueFieldStart[5],nameValueFieldEnd[5]-nameValueFieldStart[5]);
                }
                if (nameValueFieldEnd[4]!=nameValueFieldStart[4]) {
                    origin=std::string((const char*)nameValueFieldStart[5],nameValueFieldEnd[5]-nameValueFieldStart[5]);
                }
            }
            
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
                if (numConnections==(unsigned int)where->second.mSockets.size()) {
                    MultiplexedSocketPtr shared_socket(
                        MultiplexedSocket::construct<MultiplexedSocket>(&data->ios,context,where->second.mSockets,data->cb,data->mSendBufferSize, base64Stream));
                    std::string port=shared_socket->getASIOSocketWrapper(0).getLocalEndpoint().getService();
                    std::string resource_name=std::string((const char*)requestStart,requestEnd-requestStart);
                    MultiplexedSocket::sendAllProtocolHeaders(shared_socket,origin,host,port,resource_name,protocol);
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
    }
    delete buffer;
}

void beginNewStream(TCPSocket*socket, std::tr1::shared_ptr<TCPStreamListener::Data> data) {

    TcpSstHeaderArray *buffer = new TcpSstHeaderArray;

    boost::asio::async_read(*socket,
                            boost::asio::buffer(buffer->begin(),(int)TCPStream::MaxWebSocketHeaderSize>(int)ASIOReadBuffer::sBufferLength?(int)ASIOReadBuffer::sBufferLength:(int)TCPStream::MaxWebSocketHeaderSize),
                            ASIOSocketWrapper::CheckCRLF (buffer),
                            std::tr1::bind(&ASIOStreamBuilder::buildStream,buffer,socket,data,_1,_2));
}

} // namespace ASIOStreamBuilder
} // namespace Network
} // namespace Sirikata
