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

system.import('std/core/bind.js');

(
function() {

    var ns = std.graphics;

    /** InputHandler makes setting up callbacks for input events, or a
     *  subset of them, easier. It looks a lot like browser events do.
     *  Allocate one with a simulation and it will register itself as
     *  a listener. Then, add your own listeners which will be
     *  delegated to, e.g. set
     *
     *   handler.onButtonPressed = function() { .... }
     */
    ns.InputHandler = function(sim) {
        sim.invoke("setInputHandler", std.core.bind(this._handle, this) );
    };

    ns.InputHandler.prototype._handle = function(evt) {
        if (evt.msg == 'button-pressed' && 'onButtonPressed' in this)
            this.onButtonPressed(evt);
        if (evt.msg == 'button-released' && 'onButtonReleased' in this)
            this.onButtonReleased(evt);
        if (evt.msg == 'button-down' && 'onButtonDown' in this)
            this.onButtonDown(evt);
        if (evt.msg == 'axis' && 'onAxis' in this)
            this.onAxis(evt);
        if (evt.msg == 'text' && 'onText' in this)
            this.onText(evt);
        if (evt.msg == 'mouse-hover' && 'onMouseHover' in this)
            this.onMouseHover(evt);
        if (evt.msg == 'mouse-click' && 'onMouseClick' in this)
            this.onMouseClick(evt);
        if (evt.msg == 'mouse-drag' && 'onMouseDrag' in this)
            this.onMouseDrag(evt);
        if (evt.msg == 'dragdrop' && 'onDragAndDrop' in this)
            this.onDragAndDrop(evt);
        if (evt.msg == 'webview' && 'onWebView' in this)
            this.onWebView(evt);
    };

})();