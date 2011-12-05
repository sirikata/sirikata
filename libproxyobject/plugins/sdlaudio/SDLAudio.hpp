// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SDLAUDIO_SDLAUDIO_HPP_
#define _SIRIKATA_SDLAUDIO_SDLAUDIO_HPP_

#include <sirikata/proxyobject/Simulation.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

namespace Sirikata {
namespace SDL {

class FFmpegAudioStream;
typedef std::tr1::shared_ptr<FFmpegAudioStream> FFmpegAudioStreamPtr;

class AudioSimulation : public Simulation {
public:
    AudioSimulation(Context* ctx);

    // Service Interface
    virtual void start();
    virtual void stop();

    // Invokable Interface
    virtual boost::any invoke(std::vector<boost::any>& params);

    // Mixing interface, public for the mixing callback function
    void mix(uint8* raw_stream, int32 len);

private:
    // Indicates whether basic initialization was successful, i.e. whether we're
    // going to be able to do any operations.
    bool ready() const;

    void handleFinishedDownload(Transfer::ChunkRequestPtr request, Transfer::DenseDataPtr response);

    Context* mContext;
    bool mInitializedAudio;
    bool mOpenedAudio;

    Transfer::TransferPoolPtr mTransferPool;
    typedef std::map<Transfer::URI, Transfer::ResourceDownloadTaskPtr> DownloadTaskMap;
    DownloadTaskMap mDownloads;

    // Mixing callbacks come from SDL's thread, so we need to be careful about
    // managing the audio stream set.
    typedef boost::mutex Mutex;
    typedef boost::unique_lock<Mutex> Lock;
    Mutex mStreamsMutex;

    typedef std::vector<FFmpegAudioStreamPtr> AudioStreamSet;
    AudioStreamSet mStreams;
};

} //namespace SDL
} //namespace Sirikata

#endif //_SIRIKATA_SDLAUDIO_SDLAUDIO_HPP_
