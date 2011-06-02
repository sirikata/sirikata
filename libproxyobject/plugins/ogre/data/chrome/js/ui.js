/*  Sirikata
 *  ui.js
 *
 *  Copyright (c) 2011, Stanford University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* This file contains utility methods for . A few C++ level helpers
 * are generated automatically in WebView.cpp, which this file relies
 * on. Therefore, the sirikata global object already exists.
 */

/** Log data to the console. The first parameter should be a log
 * level, i.e. 'fatal', 'error', 'warn', 'debug', 'info', etc. The
 * remaining arguments are converted to strings and printed, separated
 * by spaces.
 */
sirikata.log = function() {
    var args = [];
    for(var i = 0; i < arguments.length; i++) { args.push(arguments[i]); }
    sirikata.event.apply(this, ['__log'].concat(args));
};

/** A wrapper for UI code which sets up the environment for
 * isolation. You should generally execute all your UI code through
 * this, e.g. your UI script should look like this:
 *
 *   sirikata.ui('my-module', function() {
 *     real ui code
 *   });
 */
sirikata.ui = function(name, ui_code) {
    $(document).ready(
        function() {
            var sirikata = {};
            for(var i in __sirikata) { sirikata[i] = __sirikata[i]; }
            sirikata.event = function() {
                var args = [];
                for(var i = 0; i < arguments.length; i++) { args.push(arguments[i]); }
                return __sirikata.event.apply(this, [name + '-' + args[0]].concat(args.slice(1)) );
            };
            eval('(' + ui_code.toString() + ')()');
            sirikata.event('__ready'); // really name-__ready
        }
    );
};

/** Create a UI window from the given element.
 *  @param selector selector or DOM element to convert into a window
 *  @param params settings for this window. Currently this mainly just
 *         passes through to jQuery UI, but the settings may change in the
 *         future.
 */
sirikata.ui.window = function(selector, params) {
    var win_data = $(selector);
    var real_params = {};
    for(var i in params) {
        if (i == 'modal') continue;
        real_params[i] = params[i];
    }

    real_params.parent = '#left-dock';
    var old_resize = real_params.resize;
    real_params.sizeupdate = function(event, ui) {
        if (old_resize) old_resize(event, ui);
        sirikata.ui.window._recomputeViewport();
    };
    this._ui = win_data.dockablewindow(real_params);
};

// Internal helper method. Recomputes and informs the host C++ code
// what the size of the valid 'viewport' currently is, effectively the
// rectangular region not covered by UI elements. This obviously does
// not include floating elements, only the menu + docks.
sirikata.ui.window._recomputeViewport = function() {
    var dock_wid = 0;
    $('#left-dock').children(':visible').each(function() {
                                        if ($(this).width() > dock_wid) dock_wid = $(this).width();
                                    });
    var tot_wid = $(window).width();
    var tot_height = $(window).height();
    // FIXME get rid of menu height as well as the dock
    // setViewport(left, top, right, bottom)
    sirikata.event('__setViewport', dock_wid.toString(), (0).toString(), tot_wid.toString(), tot_height.toString());
};

sirikata.ui.window.prototype.show = function() {
    this._ui.dockablewindow('open');
    return this;
};

sirikata.ui.window.prototype.hide = function() {
    this._ui.dockablewindow('close');
    return this;
};

sirikata.ui.window.prototype.toggle = function() {
    if (this._ui.dockablewindow('isOpen'))
        this.hide();
    else
        this.show();
    return this;
};

sirikata.ui.window.prototype.enable = function() {
    this._ui.dockablewindow('enable');
    return this;
};

sirikata.ui.window.prototype.disable = function() {
    this._ui.dockablewindow('disable');
    return this;
};

/** Use the given element like a button. Allows you to do things like set a click handler.
 *  @param selector selector or DOM element to use as button.
 */
sirikata.ui.button = function(selector) {
    return $(selector);
};
