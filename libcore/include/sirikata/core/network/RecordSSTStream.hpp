// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_RECORD_SST_STREAM_HPP_
#define _SIRIKATA_CORE_RECORD_SST_STREAM_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Frame.hpp>
#include <sirikata/core/util/Liveness.hpp>
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

/** RecordSSTStream wraps a regular SST stream, turning it into a record-based
 *  stream, i.e. performing buffering and triggering callbacks for each record
 *  rather than for each set of received bytes. This is useful for reliable,
 *  message-oriented protocols. The user submits individual records to be sent
 *  and it uses simple framing to identify the records at the receiver.
 */
template<typename StreamPtrType>
class RecordSSTStream : public Liveness {
public:
    typedef std::tr1::function<void(MemoryReference)> RecordCallback;

    RecordSSTStream() {}
    ~RecordSSTStream() {
        destroy();
    }

    void initialize(StreamPtrType stream, RecordCallback cb) {
        mStream = stream;
        mCB = cb;

        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        assert(mStream);
        mStream->registerReadCallback(
            std::tr1::bind(
                &RecordSSTStream::handleRead, this,
                _1, _2
            )
        );
    }

    void write(const MemoryReference& data) {
        outstanding.push(Network::Frame::write(data.begin(), data.size()));
        writeSomeData(livenessToken());
    }

    void destroy() {
        letDie();

        if (!mStream) return;
        mStream->registerReadCallback(0);
    }
private:
    void handleRead(uint8* data, int size) {
        partial_frame.append((const char*)data, size);
        while(true) {
            String parsed = Network::Frame::parse(partial_frame);
            if (parsed.empty()) return;
            mCB( MemoryReference(parsed) );
        }
    }

    void writeSomeData(Liveness::Token alive) {
        if (!alive) return;

        static Duration retry_rate = Duration::milliseconds((int64)1);

        writing = true;

        if (!mStream) {
            // We're still waiting on the stream. Initialization should trigger
            // writing if it gets a stream and there's data waiting.
            writing = false;
            return;
        }

        // Otherwise, keep sending until we run out or
        while(!outstanding.empty()) {
            std::string& framed_msg = outstanding.front();
            int bytes_written = mStream->write((const uint8*)framed_msg.data(), framed_msg.size());
            if (bytes_written < 0) {
                // FIXME
                break;
            }
            else if (bytes_written < (int)framed_msg.size()) {
                framed_msg = framed_msg.substr(bytes_written);
                break;
            }
            else {
                outstanding.pop();
            }
        }

        if (outstanding.empty())
            writing = false;
        else
            mStream->getContext()->mainStrand->post(
                retry_rate,
                std::tr1::bind(&RecordSSTStream::writeSomeData, this, alive),
                "RecordSSTStream::writeSomeData"
            );
    }

    StreamPtrType mStream;
    RecordCallback mCB;

    // Outstanding data to be sent. FIXME efficiency
    std::queue<std::string> outstanding;
    // If writing is currently in progress
    bool writing;

    // Backlog of data, i.e. incomplete frame
    String partial_frame;
}; // class RecordSSTStream

} // namespace Sirikata

#endif //_SIRIKATA_CORE_RECORD_SST_STREAM_HPP_
