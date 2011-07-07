// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


(
function() {

    /** DefaultCamera */
    std.graphics.DefaultCamera = system.Class.extend(
        {
            init: function(sim, pres) {
                this._tracking = pres;
                this._mode = 'first';

                this._sim = sim;
                this._sim.onTick(std.core.bind(this.onTick, this));

                this._offset = <0, 0, 0>;

                this._lastPosition = this._tracking.position;
                this._lastOrientation = this._tracking.orientation;

                this.reinit();
            },

            reinit : function() {
                this.setOffset(this._offset);
                this.setMode(this._mode);
            },

            mode : function() {
                 return this._mode;
            },

            setMode : function(mode) {
                this._mode = mode;

                var vis = (mode == 'first' ? false : true);
                this._sim.visible(this._tracking, vis);
            },

            setOffset: function(off) {
                this._offset = off;
            },

            onTick : function(t, dt) {
                var goalPos = this._tracking.position;
                var goalOrient = this._tracking.orientation;

                var pos;
                var orient;

                if (this._mode == 'first') {
                    // In first person mode we are tied tightly to the position of
                    // the object.
                    pos = goalPos;
                    orient = goalOrient;
                }
                else {
                    // In third person mode, the target is offset so we'll be behind and
                    // above ourselves and we need to interpolate to the target.
                    // Offset the goal.
                    // > 1 factor gets us beyond the top of the object
                    goalPos = goalPos + this._offset.mul(this._tracking.scale);
                    // Interpolate from previous positions
                    var toGoal = goalPos-this._lastPosition;
                    var toGoalLen = toGoal.length();
                    if (toGoalLen < 1e-06) {
                        pos = goalPos;
                        orient = goalOrient;
                    } else {
                        var step = util.exp(-dt*2);
                        pos = goalPos - toGoal.div(toGoalLen).mul(toGoalLen*step);
                        orient = (goalOrient.mul(1-step).add(this._lastOrientation.mul(step))).normal();
                    }
                }

                this._sim.setCameraPosition(pos);
                this._sim.setCameraOrientation(orient);

                this._lastPosition = pos;
                this._lastOrientation = orient;
            }

        }
    );

}
)();