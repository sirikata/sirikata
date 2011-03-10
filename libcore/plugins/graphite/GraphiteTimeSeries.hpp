/*  Sirikata
 *  GraphiteTimeSeries.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_GRAPHITE_TIME_SERIES_HPP_
#define _SIRIKATA_GRAPHITE_TIME_SERIES_HPP_

#include <sirikata/core/trace/TimeSeries.hpp>
#include <sirikata/core/network/Asio.hpp>

namespace Sirikata {
namespace Trace {

/** An implementation of TimeSeries which reports data to Graphite (actually to
 *  graphites underlying storage carbon + whisper). Graphite provides storage of
 *  numeric time-series data where resolution is reduced with age, allowing for
 *  fixed total storage. It also provides nice graphing utilities for exploring
 *  and inspecting the reported data.  See http://graphite.wikidot.com.
 */
class GraphiteTimeSeries : public TimeSeries {
  public:
    GraphiteTimeSeries(Context* ctx, const String& host, uint16 port);
    virtual ~GraphiteTimeSeries();

    virtual void report(const String& name, float64 val);

  private:
    void connect();

    void handleResolve(const boost::system::error_code& err, Network::TCPResolver::iterator endpoint_iterator);
    void tryConnect(Network::TCPResolver::iterator endpoint_iterator);
    void handleConnect(const boost::system::error_code& err, Network::TCPResolver::iterator endpoint_iterator);

    void startSend();
    void handleSent(const boost::system::error_code& err);

    void cleanup();

    String mHost;
    uint16 mPort;

    // Data is reported to graphite using a simple text based format sent over a
    // TCP connection
    Network::TCPResolver* mResolver;
    Network::TCPSocket* mSocket;
    bool mConnecting;
    bool mTransmitting;

    std::queue<String> mUpdates;
    String mCurrentUpdate; // To avoid realloc issues with the queue during writes
}; // class GraphiteTimeSeries

} // namespace Trace
} // namespace Sirikata

#endif //_SIRIKATA_TIME_SERIES_HPP_
