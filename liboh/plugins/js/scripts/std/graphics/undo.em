/* undo.em
 *
 * Author: Will Monroe
 *
 * An extension to the graphics simulator allowing infinite levels of undo for
 * common in-world tasks such as dragging.
 */
system.require('std/graphics/graphics.em');

(function() {

     std.graphics.Graphics.prototype.undo = function() {
         if(!this._undoStack || this._undoStack.length == 0)
             return;

         var toUndo = this._undoStack.pop();
         if(typeof(toUndo.system) !== 'object' ||
                 !('undo' in toUndo.system))
             throw('Could not undo "' + std.core.pretty(toUndo.action) +
                      '": system ' + std.core.pretty(toUndo.system) +
                      ' has no undo method');

         toUndo.system.undo(toUndo.action);
         this._redoStack.push(toUndo);
     };

     std.graphics.Graphics.prototype.redo = function() {
         if(!this._redoStack || this._redoStack.length == 0)
             return;

         var toRedo = this._redoStack.pop();
         if(typeof(toRedo.system) !== 'object' ||
                 !('redo' in toRedo.system))
             throw('Could not redo "' + std.core.pretty(toRedo.action) +
                      '": system ' + std.core.pretty(toRedo.system) +
                      ' has no redo method');

         toRedo.system.redo(toRedo.action);
         this._undoStack.push(toRedo);
     };

     std.graphics.Graphics.prototype.addUndoAction = function(action, system) {
         if(typeof(this._undoStack) === 'undefined')
             this._undoStack = [];

         this._undoStack.push({action: action, system: system});
         this._redoStack = [];
     };

})();