// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SDLAUDIO_FFMPEG_GLUE_HPP_
#define _SIRIKATA_SDLAUDIO_FFMPEG_GLUE_HPP_

#include <sirikata/core/util/Singleton.hpp>
#include <boost/thread.hpp>

namespace Sirikata {
namespace SDL {

// This borrows heavily from Chromium's code:
//
//  Copyright (c) 2011 The Chromium Authors. All rights reserved.
//  Use of this source code is governed by a BSD-style license that can be
//  found in the LICENSE file.
//
// See chromium's src/media/filters/ffmpeg_glue*.

class FFmpegGlue;
class FFmpegURLProtocol;

/** Glue layer for FFmpeg. Deals with initialization, cleanup, and providing an
 *  easy way to load from in-memory data.
 */
class FFmpegGlue : public AutoSingleton<FFmpegGlue> {
public:
    // Reference counting to know when it's safe to clean this up
    void ref();
    void unref();

    // Adds a FFmpegProtocol to the FFmpeg glue layer and returns a unique string
    // that can be passed to FFmpeg to identify the data source.
    String addProtocol(FFmpegURLProtocol* protocol);

    // Removes a FFmpegProtocol from the FFmpeg glue layer.  Using strings from
    // previously added FFmpegProtocols will no longer work.
    void removeProtocol(FFmpegURLProtocol* protocol);

    // Assigns the FFmpegProtocol identified with by the given key to
    // |protocol|, or assigns NULL if no such FFmpegProtocol could be found.
    void getProtocol(const String& key,
        FFmpegURLProtocol** protocol);
protected:
    FFmpegGlue();
    ~FFmpegGlue();
    friend class AutoSingleton<FFmpegGlue>;
    friend std::auto_ptr<FFmpegGlue>::~auto_ptr();
    friend void std::auto_ptr<FFmpegGlue>::reset(FFmpegGlue*);

    // Returns the unique key for this data source, which can be passed to
    // av_open_input_file as the filename.
    String getProtocolKey(FFmpegURLProtocol* protocol);

    int32 mRefCount;

    typedef boost::mutex Mutex;
    typedef boost::lock_guard<Mutex> Lock;
    Mutex mMutex;
    // Map between keys and FFmpegProtocol references.
    typedef std::map<String, FFmpegURLProtocol*> ProtocolMap;
    ProtocolMap mProtocols;
};

class FFmpegURLProtocol {
public:
    FFmpegURLProtocol() {}
    virtual ~FFmpegURLProtocol() {}

    // Get a user-readable name for this, e.g. the filename or original URL.
    virtual String name() const = 0;

    // Read the given amount of bytes into data, returns the number of bytes read
    // if successful, kReadError otherwise.
    virtual size_t read(size_t size, uint8* data) = 0;

    // Returns true and the current file position for this file, false if the
    // file position could not be retrieved.
    virtual bool getPosition(int64* position_out) = 0;

    // Returns true if the file position could be set, false otherwise.
    virtual bool setPosition(int64 position) = 0;

    // Returns true and the file size, false if the file size could not be
    // retrieved.
    virtual bool getSize(int64* size_out) = 0;

    // Returns false if this protocol supports random seeking.
    virtual bool isStreaming() = 0;

private:
    FFmpegURLProtocol(const FFmpegURLProtocol&);
    FFmpegURLProtocol& operator=(const FFmpegURLProtocol&);
};

} // namespace SDL
} // namespace Sirikata

#endif //_SIRIKATA_SDLAUDIO_FFMPEG_GLUE_HPP_
