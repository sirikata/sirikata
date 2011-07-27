/*  Sirikata
 *  dragControls.em
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

std.graphics.DragControls = system.Class.extend({
    init: function(graphics, callback) {
        this._graphics = graphics;
        this._graphics._simulator.addGUIModule(
                'DragControls',
                'drag/dragControls.js',
                std.core.bind(function(ui) {
                    this._ui = ui;
                    this._ui.bind('moveMode',
                                  std.core.bind(this.moveMode, this));
                    this._ui.bind('rotateMode',
                                  std.core.bind(this.rotateMode, this));
                    this._ui.bind('offMode',
                                  std.core.bind(this.offMode, this));
                    this._ui.bind('snapLocal',
                                  std.core.bind(this.snapLocal, this));
                    this._ui.bind('snapGlobal',
                                  std.core.bind(this.snapGlobal, this));
                    this._ui.bind('orientDefault',
                                  std.core.bind(this.orientDefault, this));

                    if(callback)
                        callback();
                }, this)
        );
    },

    onReset: function(callback) {
        this._graphics._simulator.addGUIModule(
                'DragControls',
                'drag/dragControls.js',
                std.core.bind(function(ui) {
                    if(callback)
                        callback();
                }, this)
        );
    },

    toggle: function() {
        this._ui.call('DragControls.toggleVisible');
    },

    rotateMode: function() {
        this._graphics.axesRotateMode();
    },

    moveMode: function() {
        this._graphics.axesMoveMode();
    },

    offMode: function() {
        this._graphics.axesOffMode();
    },

    snapGlobal: function() {
        this._graphics.axesSnapGlobal();
    },

    snapLocal: function() {
        this._graphics.axesSnapLocal();
    },

    orientDefault: function() {
        this._graphics.orientDefault();
    }
});