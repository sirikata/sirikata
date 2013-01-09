// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

system.require('std/core/namespace.em');

(
function() {

    var ns = Namespace('std.clutter');

    /** @class ClutterRenderer is a 2D renderer for Clutter data associated with
     *  objects.
     */
    ns.ClutterRenderer = function(pres) {
        this._sim = pres.runSimulation('clutter');
    };

    ns.ClutterRenderer.prototype.invoke = function() {
        return this._sim.invoke.apply(this._sim, arguments);
    };

    ns.ClutterRenderer.prototype.help = function() {
        return this.invoke('help');
    };

})();
