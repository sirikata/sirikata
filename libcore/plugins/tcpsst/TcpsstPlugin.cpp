/*  Sirikata SQLite Plugin
 *  TcpsstPlugin.cpp
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
#include <boost/thread.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include "TCPStream.hpp"
#include "TCPStreamListener.hpp"
#include <sirikata/core/options/Options.hpp>

static int core_plugin_refcount = 0;

namespace Sirikata {
static OptionSet*optionParser(const String&str) {
    OptionValue *numSockets=new OptionValue("parallel-sockets","3",OptionValueType<unsigned int>(),"Number of parallel streams used to avoid head of line blocking");
    OptionValue *maxSockets=new OptionValue("max-parallel-sockets","8",OptionValueType<unsigned int>(),"Maximum number of parallel streams accepted used to avoid head of line blocking");
    OptionValue *sendBufferSize=new OptionValue("send-buffer-size","0",OptionValueType<unsigned int>(),"Size of send buffer used to accumulate packets during an outgoing send. 0 for unlimited buffer.");
    OptionValue *kSendBufferSize=new OptionValue("ksend-buffer-size","0",OptionValueType<unsigned int>(),"Size of kernel TCP send buffer used to accumulate packets during an outgoing send. 0 for system default buffer.");
    OptionValue *kReceiveBufferSize=new OptionValue("kreceive-buffer-size","0",OptionValueType<unsigned int>(),"Size of kernel TCP receive buffer used to accumulate packets during an outgoing send. 0 for system default buffer.");
    OptionValue *noDelay=new OptionValue("no-delay","false",OptionValueType<bool>(),"Whether the no-delay option is set on the socket");
    OptionValue *zeroDelim=new OptionValue("base64","false",OptionValueType<bool>(),"Whether the stream should be base64 and zero delimited (eg websocket compat)");

    InitializeClassOptions("tcpsstoptions",numSockets,
                     numSockets,
                     maxSockets,
                     sendBufferSize,
                     noDelay,
                     zeroDelim,
                     kSendBufferSize,
                     kReceiveBufferSize,
                     NULL);
    OptionSet*retval=OptionSet::getOptions("tcpsstoptions",numSockets);
    retval->parse(str);
    return retval;
}
}
SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (core_plugin_refcount==0) {
        using std::tr1::placeholders::_1;

        Sirikata::Network::StreamFactory::getSingleton()
            .registerConstructor("tcpsst",
                                 &Network::TCPStream::construct,
                                 &Sirikata::optionParser,
                                 true);
        Sirikata::Network::StreamListenerFactory::getSingleton()
            .registerConstructor("tcpsst",
                                 &Network::TCPStreamListener::construct,
                                 &Sirikata::optionParser,
                                 true);

    }
    core_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++core_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(core_plugin_refcount>0);
    return --core_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (core_plugin_refcount==0) {
        Sirikata::Network::StreamListenerFactory::getSingleton().unregisterConstructor("tcpsst");
        Sirikata::Network::StreamFactory::getSingleton().unregisterConstructor("tcpsst");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "tcpsst";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}
