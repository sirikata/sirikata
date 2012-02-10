// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "SDLAudio.hpp"

#include <sirikata/core/transfer/AggregatedTransferPool.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

#include <sirikata/sdl/SDL.hpp>
#include "SDL_audio.h"

#include "FFmpegGlue.hpp"
#include "FFmpegMemoryProtocol.hpp"
#include "FFmpegStream.hpp"
#include "FFmpegAudioStream.hpp"

#define AUDIO_LOG(lvl, msg) SILOG(sdl-audio, lvl, msg)

namespace Sirikata {
namespace SDL {

namespace {

AudioSimulation* ToSimulation(void* data) {
    return reinterpret_cast<AudioSimulation*>(data);
}

extern void mixaudio(void* _sim, Uint8* raw_stream, int raw_len) {
    AUDIO_LOG(insane, "Mixing audio samples");

    AudioSimulation* sim = ToSimulation(_sim);
    sim->mix(raw_stream, raw_len);
}

}

AudioSimulation::AudioSimulation(Context* ctx,Network::IOStrandPtr ptr)
 : audioStrand(ptr),
   mContext(ctx),
   mInitializedAudio(false),
   mOpenedAudio(false),
   mClipHandleSource(0),
   mPlaying(false)
{}


AudioSimulation::~AudioSimulation()
{
    Liveness::letDie();
}

void AudioSimulation::start()
{
    audioStrand->post(
        std::tr1::bind(&AudioSimulation::iStart, this,livenessToken()),
        "AudioSimulation::iStart"
    );
}

void AudioSimulation::iStart(Liveness::Token lt)
{
    if (!lt) return;
    Liveness::Lock locked(lt);
    if (!locked)
    {
        AUDIO_LOG(warn,"Did not finish internal start "<<\
            "of AudioSimulation: expired audiosim.");
        return;
    }

    AUDIO_LOG(detailed, "Starting SDLAudio");

    if (SDL::InitializeSubsystem(SDL::Subsystem::Audio) != 0)
        return;

    mInitializedAudio = true;

    SDL_AudioSpec fmt;

    /* Set 16-bit stereo audio at 44Khz */
    fmt.freq = 44100;
    fmt.format = AUDIO_S16;
    fmt.channels = 2;
    fmt.samples = 2048;
    fmt.callback = mixaudio;
    fmt.userdata = this;

    /* Open the audio device and start playing sound! */
    if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
        AUDIO_LOG(error, "Unable to open audio: " << SDL_GetError());
        return;
    }

    mOpenedAudio = true;

    FFmpegGlue::getSingleton().ref();

    mTransferPool = Transfer::TransferMediator::getSingleton().registerClient<Transfer::AggregatedTransferPool>("SDLAudio");
}

bool AudioSimulation::ready() const {
    return (mInitializedAudio && mOpenedAudio && mTransferPool);
}

void AudioSimulation::stop()
{
    audioStrand->post(
        std::tr1::bind(&AudioSimulation::iStop,this,livenessToken()),
        "AudioSimulation::iStop"
    );
}

void AudioSimulation::iStop(Liveness::Token lt)
{
    if (!lt) return;
    Liveness::Lock locked(lt);
    if (!locked)
    {
        AUDIO_LOG(warn,"Did not finish internal stop "<<\
            "of AudioSimulation: expired audiosim.");
        return;
    }

    AUDIO_LOG(detailed, "Stopping SDLAudio");

    mTransferPool.reset();
    mDownloads.clear();

    if (!mInitializedAudio)
        return;

    if (mOpenedAudio) {
        SDL_PauseAudio(1);
        SDL_CloseAudio();

        FFmpegGlue::getSingleton().unref();
    }

    SDL::QuitSubsystem(SDL::Subsystem::Audio);
    mInitializedAudio = false;
}

boost::any AudioSimulation::invoke(std::vector<boost::any>& params) {
    // Decode the command. First argument is the "function name"
    if (params.empty() || !Invokable::anyIsString(params[0]))
        return boost::any();

    std::string name = Invokable::anyAsString(params[0]);
    AUDIO_LOG(detailed, "Invoking the function " << name);

    if (name == "play") {
        // Ignore if we didn't initialize properly
        if (!ready())
            return boost::any();

        // URL
        if (params.size() < 2 || !Invokable::anyIsString(params[1]))
            return boost::any();
        String sound_url_str = Invokable::anyAsString(params[1]);
        Transfer::URI sound_url(sound_url_str);
        if (sound_url.empty()) return boost::any();
        // Volume
        float32 volume = 1.f;
        if (params.size() >= 3 && Invokable::anyIsNumeric(params[2]))
            volume = (float32)Invokable::anyAsNumeric(params[2]);
        // Looping
        bool looped = false;
        if (params.size() >= 4 && Invokable::anyIsBoolean(params[3]))
            looped = Invokable::anyAsBoolean(params[3]);

        Lock lck(mMutex);

        ClipHandle id = mClipHandleSource++;
        AUDIO_LOG(detailed, "Play request for " << sound_url.toString() << " assigned ID " << id);
        // Save info immediately so we don't have to track it through download
        // process and so we can make adjustments while it's still downloading.
        Clip clip;
        clip.stream.reset();
        clip.paused = false;
        clip.volume = volume;
        clip.loop = looped;
        mClips[id] = clip;

        DownloadTaskMap::iterator task_it = mDownloads.find(sound_url);
        // If we're already working on it, we don't need to do
        // anything.  TODO(ewencp) actually we should track the number
        // of requests and play it that many times when it completes...
        if (task_it != mDownloads.end()) {
            AUDIO_LOG(insane, "Already downloading " << sound_url.toString());
            task_it->second.waiting.insert(id);
            return Invokable::asAny(id);
        }

        AUDIO_LOG(insane, "Issuing download request for " << sound_url.toString());
        Transfer::ResourceDownloadTaskPtr dl = Transfer::ResourceDownloadTask::construct(
            sound_url, mTransferPool,
            1.0,
            std::tr1::bind(&AudioSimulation::handleFinishedDownload, this, _1, _2, _3)
        );
        mDownloads[sound_url].task = dl;
        mDownloads[sound_url].waiting.insert(id);
        dl->start();

        return Invokable::asAny(id);
    }
    else if (name == "stop") {
        if (params.size() < 2 || !Invokable::anyIsNumeric(params[1]))
            return boost::any();
        ClipHandle id = Invokable::anyAsNumeric(params[1]);

        AUDIO_LOG(detailed, "Stop request for ID " << id);

        Lock lck(mMutex);

        // Clear out from playing streams
        ClipMap::iterator clip_it = mClips.find(id);
        if (clip_it != mClips.end()) {
            AUDIO_LOG(insane, "Stopping actively playing clip ID " << id);
            mClips.erase(clip_it);
        }

        // Clear out from downloads if it's still downloading
        // Not particularly efficient, but hopefully this isn't a very big map
        for(DownloadTaskMap::iterator down_it = mDownloads.begin(); down_it != mDownloads.end(); down_it++) {
            if (down_it->second.waiting.find(id) != down_it->second.waiting.end()) {
                AUDIO_LOG(insane, "Stopping downloading clip ID " << id);
                down_it->second.waiting.erase(id);
                break;
            }
        }
    }
    else if (name == "volume") {
        if (params.size() < 2 || !Invokable::anyIsNumeric(params[1]))
            return boost::any();
        ClipHandle id = Invokable::anyAsNumeric(params[1]);

        if (params.size() < 3 || !Invokable::anyIsNumeric(params[2]))
            return boost::any();
        float32 volume = (float32)Invokable::anyAsNumeric(params[2]);

        Lock lck(mMutex);
        if (mClips.find(id) == mClips.end()) return Invokable::asAny(false);
        mClips[id].volume = volume;

        return Invokable::asAny(true);
    }
    else if (name == "pause" || name == "resume") {
        if (params.size() < 2 || !Invokable::anyIsNumeric(params[1]))
            return boost::any();
        ClipHandle id = Invokable::anyAsNumeric(params[1]);

        bool paused = (name == "pause");
        AUDIO_LOG(detailed, name << " request for ID " << id);

        Lock lck(mMutex);

        ClipMap::iterator clip_it = mClips.find(id);
        if (clip_it != mClips.end()) {
            clip_it->second.paused = paused;
            return Invokable::asAny(true);
        }
        else {
            return Invokable::asAny(false);
        }
    }
    else if (name == "loop") {
        if (params.size() < 2 || !Invokable::anyIsNumeric(params[1]))
            return boost::any();
        ClipHandle id = Invokable::anyAsNumeric(params[1]);

        if (params.size() < 3 || !Invokable::anyIsBoolean(params[2]))
            return boost::any();
        bool looped = Invokable::anyAsBoolean(params[2]);

        Lock lck(mMutex);
        if (mClips.find(id) == mClips.end()) return Invokable::asAny(false);
        mClips[id].loop = looped;

        return Invokable::asAny(true);
    }
    else {
        AUDIO_LOG(warn, "Function " << name << " was invoked but this function was not found.");
    }

    return boost::any();
}

void AudioSimulation::handleFinishedDownload(
    Transfer::ResourceDownloadTaskPtr taskptr,
    Transfer::TransferRequestPtr request,
    Transfer::DenseDataPtr response)
{
    audioStrand->post(
        std::tr1::bind(&AudioSimulation::iHandleFinishedDownload,this,
            livenessToken(),taskptr,request,response),
        "AudioSimulation::iHandleFinishedDownload"
    );
}

void AudioSimulation::iHandleFinishedDownload(
    Liveness::Token lt,
    Transfer::ResourceDownloadTaskPtr taskptr,
    Transfer::TransferRequestPtr request,
    Transfer::DenseDataPtr response)
{
    if (!lt) return;
    Liveness::Lock locked(lt);
    if (!locked)
    {
        AUDIO_LOG(warn, "Aborting finished download: "<<\
            "audio sim no longer live.");
        return;
    }


    Lock lck(mMutex);

    Transfer::MetadataRequestPtr uriRequest = std::tr1::dynamic_pointer_cast<Transfer::MetadataRequest>(request);
    assert(uriRequest && "Got unexpected TransferRequest subclass.");
    const Transfer::URI& sound_url = uriRequest->getURI();

    // We may have stopped the simulation and then gotten the callback. Ignore
    // in this case.
    if (mDownloads.find(sound_url) == mDownloads.end()) return;

    // Otherwise remove the record, saving the waiting clips
    std::set<ClipHandle> waiting = mDownloads[sound_url].waiting;
    mDownloads.erase(sound_url);

    // If the download failed, just log it
    if (!response) {
        AUDIO_LOG(error, "Failed to download " << sound_url << " sound file.");
        return;
    }

    if (response->size() == 0) {
        AUDIO_LOG(error, "Got zero sized audio file download for " << sound_url);
        return;
    }

    AUDIO_LOG(detailed, "Finished download for audio file " << sound_url << ": " << response->size() << " bytes");

    for(std::set<ClipHandle>::iterator id_it = waiting.begin(); id_it != waiting.end(); id_it++) {
        FFmpegMemoryProtocol* dataSource = new FFmpegMemoryProtocol(sound_url.toString(), response);
        FFmpegStreamPtr stream(FFmpegStream::construct<FFmpegStream>(static_cast<FFmpegURLProtocol*>(dataSource)));
        if (stream->numAudioStreams() == 0) {
            AUDIO_LOG(error, "Found zero audio streams in " << sound_url << ", ignoring");
            return;
        }
        if (stream->numAudioStreams() > 1)
            AUDIO_LOG(detailed, "Found more than one audio stream in " << sound_url << ", only playing first");
        FFmpegAudioStreamPtr audio_stream = stream->getAudioStream(0, 2);

        // Might have been removed already
        if (mClips.find(*id_it) == mClips.end()) continue;

        mClips[*id_it].stream = audio_stream;
    }

    // Enable playback if we didn't have any active streams before
    if (!mPlaying) {
        SDL_PauseAudio(0);
        mPlaying = true;
    }
}

void AudioSimulation::mix(uint8* raw_stream, int32 raw_len) {
    int16* stream = (int16*)raw_stream;
    // Length in individual samples
    int32 stream_len = raw_len / sizeof(int16);
    // Length in samples for all channels
#define MAX_CHANNELS 6
    int32 nchannels = 2; // Assuming stereo, see SDL audio setup
    int32 samples_len = stream_len / nchannels;

    Lock lck(mMutex);

    for(int i = 0; i < samples_len; i++) {
        int32 mixed[MAX_CHANNELS];
        for(int c = 0; c < nchannels; c++)
            mixed[c] = 0;

        for(ClipMap::iterator st_it = mClips.begin(); st_it != mClips.end(); st_it++) {
            // We might still be downloading it or it might be paused
            if (!st_it->second.stream ||
                st_it->second.paused)
                continue;

            int16 samples[MAX_CHANNELS];
            st_it->second.stream->samples(samples, st_it->second.loop);

            for(int c = 0; c < nchannels; c++)
                mixed[c] += samples[c] * st_it->second.volume;
        }

        for(int c = 0; c < nchannels; c++)
            stream[i*nchannels + c] = (int16)std::min(std::max(mixed[c], -32768), 32767);
    }

    // Clean out streams that have finished
    for(ClipMap::iterator st_it = mClips.begin(); st_it != mClips.end(); ) {
        ClipMap::iterator del_it = st_it;
        if (del_it->second.stream->finished()) mClips.erase(del_it);
        st_it++;
    }

    // Disable playback if we've run out of sounds
    if (mClips.empty()) {
        SDL_PauseAudio(1);
        mPlaying = false;
    }
}

} //namespace SDL
} //namespace Sirikata
