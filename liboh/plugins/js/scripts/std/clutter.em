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

    var create_wrapped_fn = function(fn_name, invoke_name) {
        if (invoke_name === undefined) invoke_name = fn_name;
        ns.ClutterRenderer.prototype[fn_name] = function() {
            var args = [invoke_name];
            for (var i in arguments)
                args.push(arguments[i]);
            return this.invoke.apply(this, args);
        };
    };

    create_wrapped_fn('help');
    create_wrapped_fn('stage_set_size');
    create_wrapped_fn('stage_set_color');
    create_wrapped_fn('actor_set_size');
    create_wrapped_fn('actor_set_position');
    create_wrapped_fn('actor_show');
    create_wrapped_fn('actor_destroy');
    create_wrapped_fn('rectangle_create');
    create_wrapped_fn('rectangle_set_color');
    create_wrapped_fn('text_create');
    create_wrapped_fn('text_set_color');
    create_wrapped_fn('text_set_text');
    create_wrapped_fn('text_set_font');

})();
