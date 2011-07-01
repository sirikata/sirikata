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

// This provides a dummy implementation of the basic utilities. This is
// incredibly useful for debugging -- it allows you to run within a normal
// browser and use their development tools.
if (typeof(sirikata) === "undefined") {
    sirikata = {};
    __sirikata = sirikata;
    sirikata.version = {
        major: 0,
        minor: 0,
        revision: 0,
        commit: 'a000000000000000000000',
        string: '0.0.0'
    };
    sirikata.__event = function() {
        if (console && console.log && typeof(console.log) === "function") {
            console.log.apply(console, arguments);
        }
    };
    sirikata.event = function() {
        var args = [];
        for(var i = 0; i < arguments.length; i++) { args.push(arguments[i]); }
        sirikata.__event.apply(this, args);
    };
}

(function() {
     var stringify = function(x) {
         if (typeof(x) === "string")
             return x;
         else if (x === null)
             return 'null';
         else if (x === undefined)
             return 'undefined';
         else if (typeof(x) === "number" ||
                  typeof(x) === "boolean")
             return x.toString();
         else
             return JSON.stringify(x);
     };
/** Log data to the console. The first parameter should be a log
 * level, i.e. 'fatal', 'error', 'warn', 'debug', 'info', etc. The
 * remaining arguments are converted to strings and printed, separated
 * by spaces.
 */
sirikata.log = function() {
    var args = [];
    for(var i = 0; i < arguments.length; i++) { args.push( stringify(arguments[i]) ); }
    sirikata.event.apply(this, ['__log'].concat(args));
};

/** Allocate a web browser with the given URL. Width and height are optional. */
sirikata.openBrowser = function(name, url, width, height) {
    if (width && height)
        sirikata.event.apply(this, ['__openBrowser', stringify(name), stringify(url), stringify(width), stringify(height)]);
    else
        sirikata.event.apply(this, ['__openBrowser', stringify(name), stringify(url)]);
};


sirikata.__listenToBrowserHandlers = {};
var chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXTZabcdefghiklmnopqrstuvwxyz";
/** Listen for events from the browser. Generally, the callback should
 *  be of the form
 *    callback(event_name, ...)
 *  Currently supported callbacks:
 *    callback('navigate', url) // The page navigated to the given url.
 */
sirikata.listenToBrowser = function(name, cb) {
    // To deal with the fact that we currently require the callback
    // specified down to C++ to be a string name of a global function,
    // we just build up a dispatch table using random keys to identify
    // the listener

    // Random key
    var rand_key = '';
    for (var i=0; i < 8; i++) {
	var rnum = Math.floor(Math.random() * chars.length);
        rand_key += chars.substring(rnum,rnum+1);
    }

    // Register in global to make accessible
    sirikata.__listenToBrowserHandlers[rand_key] = function() {
        cb.apply(this, arguments);
    };

    // Build and register string name of handler
    var handler_name = 'sirikata.__listenToBrowserHandlers["' + rand_key + '"]';
    sirikata.event.apply(this, ['__listenToBrowser', stringify(name), handler_name]);
};

/** Close the browser with the given name. */
sirikata.closeBrowser = function(name) {
    sirikata.event.apply(this, ['__closeBrowser', stringify(name)]);
};

})();

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
    real_params.startDockUpdate = function() {
        sirikata.ui.window._recomputeViewport();
    };
    real_params.resize = function() {
        sirikata.ui.window._recomputeViewport();
    };
    this._ui = win_data.dockablewindow(real_params);
};

// Internal helper method. Recomputes and informs the host C++ code
// what the size of the valid 'viewport' currently is, effectively the
// rectangular region not covered by UI elements. This obviously does
// not include floating elements, only the menu + docks.
sirikata.ui.window._recomputeViewport = function(added, removed) {
    var dock_wid = 0;
    // FIXME the update logic is off here. We should use
    // $('#left-dock').children(':visible') but in order to get this
    // right we need to deal with the added/removed docked item. So
    // instead, we just look for any docked items and have the
    // dockable windows invoke startDockUpdate after setting up the
    // classes so that all the docked windows will be labelled. Note
    // that this requires that there only be ONE dock.
    $('.ui-dockablewindow-docked :visible').each(function() {
                                           if ($(this).width() > dock_wid && this !== removed) dock_wid = $(this).width();
                                       });
    if (added && $(added).width() > dock_wid) dock_wid = $(added).width();

    var tot_wid = $(window).width();
    var tot_height = $(window).height();
    // FIXME get rid of menu height as well as the dock
    // setViewport(left, top, right, bottom)
    sirikata.event('__setViewport', dock_wid.toString(), (0).toString(), tot_wid.toString(), tot_height.toString());

    // Also set the dock's width since we can't easily get it to hide
    // with nothing in it but cover the full height of the screen when
    // something is in it using only CSS. Or at least I can't figure
    // out how...
    $('#left-dock').width(dock_wid);
};

$(document).ready(
    function() {
        $(window).resize(function() {
                             sirikata.ui.window._recomputeViewport();
                         }
                        );

        $('#left-dock').height( $(window).height() - 20);
        $('#left-dock').width(0);
    }
);

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
