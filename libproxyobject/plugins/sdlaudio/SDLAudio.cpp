// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "SDLAudio.hpp"
#include <sirikata/sdl/SDL.hpp>
#include "SDL_audio.h"

#define AUDIO_LOG(lvl, msg) SILOG(sdl-audio, lvl, msg)

namespace Sirikata {
namespace SDL {

namespace {

extern void mixaudio(void *unused, Uint8 *raw_stream, int _len) {
    AUDIO_LOG(insane, "Mixing audio samples");

    int16* stream = (int16*)raw_stream;
    uint32 stream_len = _len / sizeof(int16);

    for(int i = 0; i < stream_len; i++)
        stream[i] = rand() % 8192;
}

}

AudioSimulation::AudioSimulation(Context* ctx)
 : mContext(ctx),
   mInitializedAudio(false),
   mOpenedAudio(false)
{}

void AudioSimulation::start() {
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
    fmt.userdata = NULL;

    /* Open the audio device and start playing sound! */
    if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
        AUDIO_LOG(error, "Unable to open audio: " << SDL_GetError());
        return;
    }

    mOpenedAudio = true;
    SDL_PauseAudio(0);
}

void AudioSimulation::stop() {
    AUDIO_LOG(detailed, "Stopping SDLAudio");

    if (!mInitializedAudio)
        return;

    if (mOpenedAudio) {
        SDL_PauseAudio(1);
        SDL_CloseAudio();
    }

    SDL::QuitSubsystem(SDL::Subsystem::Audio);
    mInitializedAudio = false;
}

boost::any AudioSimulation::invoke(std::vector<boost::any>& params) {
    return boost::any();
}


} //namespace SDL
} //namespace Sirikata
