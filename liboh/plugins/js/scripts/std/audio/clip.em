// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

system.require('std/core/namespace.em');

(
function() {

    var ns = Namespace('std.audio');

    /** Represents a single audio track to be played. */
    ns.Clip = function(parent, url, volume, loop) {
        this._parent = parent;
        this._url = url;
        this._volume = (volume === undefined || volume === null) ? 1.0 : volume;
        this._loop = (loop === undefined || loop === null) ? false : loop;

        // Handle for the underlying system clip value. Combined with the
        // parent, this gives a unique handle to the clip. However, it isn't set
        // until it gets allocated by calling play
        this._handle = undefined;
    };

    /** Precache the sound so it can be played quickly on demand. This
     *  may or may not actually download and cache the file: it is
     *  just a hint to the system that you might want to play the
     *  sound soon.
     */
    ns.Clip.prototype.precache = function() {
        // FIXME currently does nothing
    };

    /** Load and play the clip. */
    ns.Clip.prototype.play = function() {
        this._handle = this._parent.invoke('play', this._url, this._volume, this._loop);
        var success = (this._handle !== undefined);
        return success;
    };

    /** Pause the clip. */
    ns.Clip.prototype.pause = function() {
        if (this._handle === undefined) return;
        this._parent.invoke('pause', this._handle);
    };

    /** Resume the clip. */
    ns.Clip.prototype.resume = function() {
        if (this._handle === undefined) return;
        this._parent.invoke('resume', this._handle);
    };

    /** Stop playing the clip. */
    ns.Clip.prototype.stop = function() {
        if (this._handle === undefined) return;
        this._parent.invoke('stop', this._handle);
    };

    /** Change the volume of the clip. */
    ns.Clip.prototype.volume = function(v) {
        if (this._handle === undefined) return;
        this._parent.invoke('volume', this._handle, v);
    };

    /** Change whether the clip loops. */
    ns.Clip.prototype.loop = function(loop) {
        if (this._handle === undefined) return;
        this._parent.invoke('loop', this._handle, loop);
    };

})();
