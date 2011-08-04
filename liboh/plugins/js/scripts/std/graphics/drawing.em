/* drawing.em
 *
 * Author: Zhihong Xu
 *
 * A utility to create drawings in the local client
 */

(function() {
    system.require('std/core/namespace.em');
    var gr = Namespace('std.graphics');
    
    gr.Drawing = system.Class.extend({
        LINE_LIST: 2,
        LINE_STRIP: 3,
    
        /** @function
          * @description create a new drawing. This essentially creates a new node in the scene graph
          * @param simulator a std.graphics.Graphics object
          * @param obj the object to attach the axes to; axes are attached to the root scene node if omitted
          * @param inheritOrient a boolean to specify whether to inherit orientation from the parent scene node; true by default
          * @param inheritScale a boolean to specify whether to inherit scale from the parent scene node; true by default
          * @param visible a boolean to specify whether the scene node should be visible; true by default
          */
        init: function(simulator, obj, inheritOrient, inheritScale, visible) {
            this._sim = simulator;
            if (!gr.Drawing.SN) {
                gr.Drawing.SN = 1;
            } else {
                gr.Drawing.SN++;
            }
            this._name = gr.Drawing.SN.toString() + "std.graphics.Drawing";
            this._sim.newDrawing(this._name, obj, inheritOrient, inheritScale, visible); 
        },
        
        /** @function
          * @description set emission to (r,g,b); r,g,b are integers from 0 to 255
          */
        setColor: function(r, g, b) {
            this._mat = this.setMat(r, g, b);
        },
        
        /** @function
          * @description set the current material (emission, diffuse, specular and ambient),
          * all parameters are integer from 0 to 255
          */
        setMat: function(er, eg, eb, dr, dg, db, sr, sg, sb, ar, ag, ab) {
            this._mat = this._sim.setMat(er, eg, eb, dr, dg, db, sr, sg, sb, ar, ag, ab);
        },
        
        /** @function
          * @description set visibility of the drawing
          */
        setVisible: function(visible) {
            this._sim.setVisible(this._name, visible);
        },
        
        /** @function
          * @description set whether to inherit orientation from parent scene node
          */
        setInheritOrient: function(inheritOrient) {
            this._sim.setInheritOrient(this._name, inheritOrient);
        },
        
        /** @function
          * @description set whether to inherit scale from parent scene node
          */
        setInheritScale: function(inheritScale) {
            this._sim.setInheritScale(this._name, inheritScale);
        },
        
        /** @function
          * @description draw a shape
          */
        shape: function(type, points, clear) {
            this._sim.shape(this._name, clear, type, points);
        },
        
        /** @function
          * @description draw lines
          * @param points an array of vectors, the first 2 specify the 1st line, 
                          the 2nd 2 specify the 2nd and so on
          * @param clear a boolean to specify whether to clear the current drawing 
                         before drawing the lines
          */
        lineList: function(points, clear) {
            this.shape(this.LINE_LIST, points, clear);
        },
        
        /** @function
          * @description draw lines
          * @param points an array of vectors, the first 2 specify the 1st line, 
          *               the 2nd and the 3rd specify the 2nd line and so on
          * @param clear a boolean to specify whether to clear the current drawing 
          *               before drawing the lines
          */
        lineStrip: function(points, clear) {
            this.shape(this.LINE_STRIP, points, clear);
        },
        
        /** @function
          * @description clear the drawing
          */
        clear: function() {
            this.shape(null, null, true);
        },
        
        toString: function() {
            return this._name;
        }
    });
    
})();
