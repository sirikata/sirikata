// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

system.require('std/core/namespace.em');
system.require('std/audio/clip.em');

(
function() {

    var ns = Namespace('std.audio');

    /** The audio 'simulation', which allows you to play
     *  clips. Individual clips are controlled by the std.audio.Clip
     *  class.
     */
    ns.Audio = function(pres, name) {
        this._sim = pres.runSimulation(name);
    };

    ns.Audio.prototype.invoke = function() {
        return this._sim.invoke.apply(this._sim, arguments);
    };

    /** Allocate a clip using the given URL. This doesn't play the
     *  clip, just allocates the object that allows you to control it.
     */
    ns.Audio.prototype.clip = function(url, volume, loop) {
        return new ns.Clip(this, url, volume, loop);
    };

    ns.Audio.prototype.play = function(url, volume, loop) {
        var clip = this.clip(url, volume, loop);
        clip.play();
        return clip;
    };

})();
