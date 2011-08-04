/* axes.em
 * 
 * Author: Zhihong Xu
 *
 * Restricts an object's motion to certain axes when dragging/rotating
 */

(function() {
    system.require('std/core/namespace.em');
    system.require('std/graphics/drawing.em');
    
    var ax = Namespace('std.graphics.axes');
    
    ax.Axes = system.Class.extend({
        init: function(simulator, obj, inheritOrient, state) {
            this._sim = simulator;
            this._obj = obj;
            this._mesh = this._obj.mesh;
            if (typeof(inheritOrient) !== "boolean") {
                inheritOrient = true;
            }
            this._inheritOrient = [inheritOrient, inheritOrient, inheritOrient];
            this._state = state || [true, true, true];
            this._drawings = new Array();
            for (var i = 0; i < 3; i++) {
                this._drawings[i] = new std.graphics.Drawing(simulator, obj, inheritOrient, true, this._state[i]);
                this._redraw(i);
            }
        },
        
        /** @function
          * @description return the state of the axes as an array of booleans corresponding to
          *              the 3 axes; true indicates that the axis is on.
          */
        state: function() {
            return this._state;
        },
        
        /** @function
          * @description return the inherit orientation status of the axes as an array of booleans
          *              corresponding to the 3 axes; true indicates that the axis is in local
          *              coordinate space and false means that the axis is in global coordinate
          *              space
          */
        inheritOrient: function() {
            return this._inheritOrient;
        },
        
        /** @function
          * @description reset the axes. Call this when the mesh of the object changes
          */
        reset: function() {
            for (var i = 0; i < 3; i++) {
                this._drawings[i].setInheritScale(false);
                this._redraw(i);
                this._drawings[i].setInheritScale(true);
                this._mesh = this._obj.mesh;
            }
        },
         
       
        setInheritOrientAll: function(inheritOrient) {
            for (var i = 0; i < 3; i++) {
                this.setInheritOrient(i, inheritOrient);
            }
        },
         
        setInheritOrient: function(axis, inheritOrient) {
            if (typeof(axis) === "string") {
                if (axis === 'x' || axis === 'X') {
                    axis = 0;
                } else if (axis === 'y' || axis === 'Y') {
                    axis = 1;
                } else if (axis === 'z' || axis === 'Z') {
                    axis = 2;
                }
            } else if (typeof(axis) !== "number") {
                return;
            }   
            
            if (this._inheritOrient[axis] !== inheritOrient) {
                this._inheritOrient[axis] = inheritOrient;
                this._drawings[axis].setInheritOrient(inheritOrient);
            }
        },
        
        setVisibleAll: function(visible) {
            for (var i = 0; i < 3; i++) {
                this.setVisible(i, visible);
            }
        },
        
        setVisible: function(axis, visible) {
            if (typeof(axis) === "string") {
                if (axis === 'x' || axis === 'X') {
                    axis = 0;
                } else if (axis === 'y' || axis === 'Y') {
                    axis = 1;
                } else if (axis === 'z' || axis === 'Z') {
                    axis = 2;
                }
            } else if (typeof(axis) !== "number") {
                return;
            }   
            
            if (visible && !this._state[axis]) {
                if (this._mesh !== this._obj.mesh) {
                    this.reset();
                }
                this._drawings[axis].setVisible(true);
                this._state[axis] = true;
            } else if (!visible && this._state[axis]) {
                this._drawings[axis].setVisible(false);
                this._state[axis] = false;
            }
        },
        
        /** @function
          * @description return the index of the axis (0, 1 or 2) clicked on by mouse;
          *              return -1 if no axis is selected
          */
        pick: function(x, y) {
            var cam = this._sim.camera();
            var aspectRatio = cam.aspectRatio;
            var camPos = <cam.position.x, cam.position.y, cam.position.z>;
            var scale = this._obj.scale * ax.Axes.LEN_SCALE;
            
            var bestAxis = -1;
            var bestDistToCam = Infinity;
            
            var sO = this._sim.world2Screen(this._obj.position);
            sO = <sO.x * aspectRatio, sO.y, 0>;
            var cursor = <x * aspectRatio, y, 0>.sub(sO);
            
            for (var i = 0; i < 3; i++) {
                if (!this._state[i]) {
                    continue;
                }
            
                var wrEnd = ax.Axes.AXES[i].dir.scale(scale);
                if (this._inheritOrient[i]) {
                    wrEnd = this._obj.orientation.mul(wrEnd);
                }
                var wEnd = wrEnd.add(this._obj.position);
                var sEnd = this._sim.world2Screen(wEnd);
                sEnd = <sEnd.x * aspectRatio, sEnd.y, 0>;
                var srEnd = sEnd.sub(sO);
                var srLen = srEnd.length();
                var sDir = srEnd.div(srLen);
                
                var projLen = cursor.dot(sDir);
                if (projLen < 0) {
                    wrEnd = wrEnd.neg();
                    wEnd = wrEnd.add(this._obj.position);
                    sEnd = this._sim.world2Screen(wEnd);
                    sEnd = <sEnd.x * aspectRatio, sEnd.y, 0>;
                    srEnd = sEnd.sub(sO);
                    srLen = srEnd.length();
                    sDir = srEnd.div(srLen);
                    projLen = cursor.dot(sDir);
                }
                
                
                var dist = cursor.sub(sDir.scale(projLen)).length();
                
                if (dist > 0.03) {
                    continue;
                }
                
                
                if (projLen > srLen) {
                    continue;
                }
                
                var hitPoint;
                if (util.abs(srLen) < 1e-8) {
                    hitPoint = this._obj.position;
                } else {
                    hitPoint = wrEnd.scale(projLen / srLen).add(this._obj.position);
                }
                
                var distToCam = hitPoint.sub(camPos).length();
                
                if (distToCam < bestDistToCam) {
                    bestAxis = i;
                    bestDistToCam = distToCam;
                }
                
            }
            
            return bestAxis;
        },
        
        _redraw: function(i) {
            var scale = ax.Axes.LEN_SCALE * this._obj.scale;
            var dir = ax.Axes.AXES[i].dir.scale(scale);
            var tip = dir.scale(0.9);
            var dir1 = ax.Axes.AXES[(i+1)%3].dir.scale(scale * 0.05);
            var dir2 = ax.Axes.AXES[(i+2)%3].dir.scale(scale * 0.05);
            var posColor = ax.Axes.AXES[i].posColor;
            var negColor = ax.Axes.AXES[i].negColor;
            this._drawings[i].setColor.apply(this._drawings[i], negColor);
            this._drawings[i].lineList([dir.neg(), <0, 0, 0>], true);
            this._drawings[i].setColor.apply(this._drawings[i], posColor);
            this._drawings[i].lineList([<0, 0, 0>, dir]);
            this._drawings[i].lineStrip([tip.add(dir1), dir, tip.sub(dir1)]);
            this._drawings[i].lineStrip([tip.add(dir2), dir, tip.sub(dir2)]);
        }
        
        
    });
    
    ax.Axes.LEN_SCALE = 3;
    ax.Axes.AXES = [{posColor: [128, 0, 0], negColor: [255, 100, 100], dir: <1, 0, 0>},
                    {posColor: [0, 0, 128], negColor: [100, 100, 255], dir: <0, 1, 0>},
                    {posColor: [0, 128, 0], negColor: [100, 255, 100], dir: <0, 0, 1>}];
    
    ax._map = new Object();
    
    ax.init = function(simulator) {
        ax._sim = simulator;
    };
    
    ax.getAxes = function(obj) {
        var key = obj.toString();
        return ax._map[key];
    };
    
    ax.setVisible = function(obj, axis, visible) {
        var key = obj.toString();
        
        if (visible) {
            if (!ax._map[key]) {
                ax._map[key] = new ax.Axes(ax._sim, obj, true, [false, false, false]);
                ax._map[key].setVisible(axis, visible);
            }
        } else if (ax._map[key]) {
            ax._map[key].setVisible(axis, visible);
        }
    };
    
    ax.setInheritOrient = function(obj, axis, inheritOrient) {
        var key = obj.toString();
        if (!ax._map[key]) {
            ax._map[key] = new ax.Axes(ax._sim, obj, true, [false, false, false]);
        }
        ax._map[key].setInheritOrient(axis, inheritOrient);
    };
    
    ax.setVisibleAll = function(obj, visible) {
        var key = obj.toString();
        
        if (visible) {
            if (!ax._map[key]) {
                ax._map[key] = new ax.Axes(ax._sim, obj);
                return;
            }
            ax._map[key].setVisibleAll(visible);
        } else if (ax._map[key]) {
            ax._map[key].setVisibleAll(visible);
        }
    };
    
    ax.setInheritOrientAll = function(obj, inheritOrient) {
        var key = obj.toString();
        if (!ax._map[key]) {
            ax._map[key] = new ax.Axes(ax._sim, obj, inheritOrient, [false, false, false]);
        } else {
            ax._map[key].setInheritOrientAll(inheritOrient);
        }
    };
    
    ax.reset = function(obj) {
        var key = obj.toString();
        if (!ax._map[key]) {
            return;
        }
        ax._map[key].reset();
    };
    
    ax.pick = function(obj, x, y) {
        var key = obj.toString();
        if (!ax._map[key]) {
            return;
        }
        return ax._map[key].pick(x, y);
    };
    
    
})();
