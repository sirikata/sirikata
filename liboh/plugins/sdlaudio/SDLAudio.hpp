// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SDLAUDIO_SDLAUDIO_HPP_
#define _SIRIKATA_SDLAUDIO_SDLAUDIO_HPP_

#include <sirikata/oh/Simulation.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

namespace Sirikata {
namespace SDL {

class FFmpegAudioStream;
typedef std::tr1::shared_ptr<FFmpegAudioStream> FFmpegAudioStreamPtr;

class AudioSimulation : public Simulation,
                        public Liveness
                        
{
public:
    AudioSimulation(Context* ctx, Network::IOStrandPtr aStrand);
    virtual ~AudioSimulation();

    
    // Service Interface
    virtual void start();
    virtual void stop();

    // Invokable Interface
    virtual boost::any invoke(std::vector<boost::any>& params);

    
    
    // Mixing interface, public for the mixing callback function
    void mix(uint8* raw_stream, int32 len);

private:
    void iStart(Liveness::Token lt);
    void iStop(Liveness::Token lt);

    bool initialized;
    
    // Indicates whether basic initialization was successful, i.e. whether we're
    // going to be able to do any operations.
    bool ready() const;

    void handleFinishedDownload(
        Transfer::ChunkRequestPtr request,
        Transfer::DenseDataPtr response);
    
    void iHandleFinishedDownload(
        Liveness::Token lt,
        Transfer::ChunkRequestPtr request,
        Transfer::DenseDataPtr response);

    Network::IOStrandPtr audioStrand;
    
    
    Context* mContext;
    bool mInitializedAudio;
    bool mOpenedAudio;

    typedef uint32 ClipHandle;
    ClipHandle mClipHandleSource;

    // Mixing callbacks come from SDL's thread, download handlers come from
    // transfer thread, so we need to be careful about managing access.
    typedef boost::mutex Mutex;
    typedef boost::unique_lock<Mutex> Lock;
    Mutex mMutex;

    Transfer::TransferPoolPtr mTransferPool;
    struct DownloadTask {
        Transfer::ResourceDownloadTaskPtr task;
        std::set<ClipHandle> waiting;
    };
    typedef std::map<Transfer::URI, DownloadTask> DownloadTaskMap;
    DownloadTaskMap mDownloads;

    // ClipHandles are used to uniquely identify playing clips
    struct Clip {
        FFmpegAudioStreamPtr stream;
        bool paused;
        float32 volume;
        bool loop;
    };
    typedef std::map<ClipHandle, Clip> ClipMap;
    ClipMap mClips;

    bool mPlaying;
};

} //namespace SDL
} //namespace Sirikata

#endif //_SIRIKATA_SDLAUDIO_SDLAUDIO_HPP_
