/*  Sirikata
 *  input.em
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

if (typeof(std) === "undefined") std = {};
if (typeof(std.graphics) === "undefined") std.graphics = {};

system.require('std/core/bind.em');

(
function() {

    var ns = std.graphics;

    /** @namepsace
     *  InputHandler makes setting up callbacks for input events, or a
     *  subset of them, easier. It looks a lot like browser events do.
     *  Allocate one with a simulation and it will register itself as
     *  a listener. Then, add your own listeners which will be
     *  delegated to, e.g. set
     *
     *   handler.onButtonPressed = function() { .... }
     */
    std.graphics.InputHandler = function(sim) {
        sim.invoke("setInputHandler", std.core.bind(this._handle, this) );
        this._mapping = {
            'button-pressed': 'onButtonPressed',
            'button-repeat': 'onButtonRepeated',
            'button-up': 'onButtonReleased',
            'button-down': 'onButtonDown',
            'axis': 'onAxis',
            'text': 'onText',
            'mouse-hover': 'onMouseHover',
            'mouse-press': 'onMousePress',
            'mouse-release': 'onMouseRelease',
            'mouse-click': 'onMouseClick',
            'mouse-drag': 'onMouseDrag',
            'dragdrop': 'onDragAndDrop',
            'webview': 'onWebView'
        };
    };

    std.graphics.InputHandler.prototype._handle = function(evt) {
        if (evt.msg in this._mapping) {
            var cb = this[ this._mapping[evt.msg] ];
            if (cb) cb(evt);
        }
        if (this.onAnything) this.onAnything(evt);
    };

})();
