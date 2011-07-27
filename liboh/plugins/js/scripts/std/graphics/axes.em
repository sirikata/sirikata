/* axes.em
 *
 * Author: Will Monroe
 *
 * Restrict an object's motion to take place along a certain axis while
 * dragging.
 *
 * [currently: creates a single axis as its own entity, which can be dragged
 * in any direction and is not attached to another presence]
 */
system.require('std/graphics/graphics.em');

(function() {

     var redAxisMesh = 'meerkat:///wmonroe4/red_axis.dae/optimized/red_axis.dae';

     function axesScript(args) {
         system.require('std/movement/movableremote.em');
         system.require('std/movement/units.em');
         system.require('std/core/repeatingTimer.em');

         var moveMeshes = {x: 'meerkat:///wmonroe4/red_axis.dae/optimized/red_axis.dae',
                           y: 'meerkat:///wmonroe4/green_axis.dae/optimized/green_axis.dae',
                           z: 'meerkat:///wmonroe4/blue_axis.dae/optimized/blue_axis.dae'};
         var rotateMeshes = {x: 'meerkat:///wmonroe4/red_gimbal.dae/optimized/red_gimbal.dae',
                           y: 'meerkat:///wmonroe4/green_gimbal.dae/optimized/green_gimbal.dae',
                           z: 'meerkat:///wmonroe4/blue_gimbal.dae/optimized/blue_gimbal.dae'};
         var orientations = {x: new util.Quaternion(<0, 0, -1>, Math.PI / 2),
                             y: new util.Quaternion(),
                             z: new util.Quaternion(<1, 0, 0>, Math.PI / 2)};
         var axisMeshes = moveMeshes;
         var mode = 'move';
         var axes = {};
         var axisIDs = {};
         var movable = null;
         var updateTimer;
         var UPDATE_PERIOD = 1 * u.s;
         var ROTATE_FACTOR = 0.02;

         function onPresenceConnected(presence) {
             if(presence.mesh.slice(-12) != 'red_axis.dae')
                 return;

             setPosition << [{'request':'movable':},
                             {'position'::}];
             setOrientation << [{'request':'movable':},
                                {'orient'::}];
             setScale << [{'request':'movable':},
                          {'scale'::}];

             follow << [{'request':'axes':},
                               {'id'::}];
             toggleRotateMode << [{'request':'axes':},
                                  {'mode':'toggle':}];
             snapLocal << [{'request':'axes':},
                           {'snap':'local':}];
             snapGlobal << [{'request':'axes':},
                           {'snap':'global':}];
             forwardScriptRequest << [{'request':'script':}];
             onRedConnected(presence);
         }

         function onRedConnected(presence) {
             presence.modelOrientation = orientations.x;
             axes.x = presence;
             axisIDs[presence.toString()] = true;
             system.createPresence('meerkat:///wmonroe4/green_axis.dae/optimized/green_axis.dae',
                                   onGreenConnected);
         }

         function onGreenConnected(presence) {
             presence.modelOrientation = orientations.y;
             axes.y = presence;
             axisIDs[presence.toString()] = true;
             system.createPresence('meerkat:///wmonroe4/blue_axis.dae/optimized/blue_axis.dae',
                                   onBlueConnected);
         }

         function onBlueConnected(presence) {
             presence.modelOrientation = orientations.z;
             axes.z = presence;
             axisIDs[presence.toString()] = true;
             follow();
             updateTimer = new std.core.RepeatingTimer(UPDATE_PERIOD,
                                                       function() { follow(); });
             {axes: 'created'} >> args.avatar >> [];
         }

         function setPosition(msg, sender) {
             var axis = system.self.orientation.mul(
                     system.self.modelOrientation.mul(<0, 1, 0>));
             var delta = axis.scale(axis.dot(msg.position - system.self.position));
             if(mode == 'move') {
                 system.self.position = system.self.position + delta;

                 for(var a in axes)
                     axes[a].position = system.self.position;
                 if(movable)
                     movable.setPosition(system.self.position);
             } else /* mode == 'rotate' */ {
                 var quatDelta = new util.Quaternion(delta.normal(),
                                                     delta.length() * ROTATE_FACTOR /
                                                     system.self.scale);

                 system.self.orientation = quatDelta.mul(system.self.orientation);

                 for(var a in axes)
                     axes[a].orientation = system.self.orientation;
                 if(movable)
                     movable.setOrientation(system.self.orientation);
             }
             msg.makeReply({}) >> [];
         }

         function setOrientation(msg, sender) {
             /* Quaternions mess everything up!  By all means, try to fix this
              * code if you're brave.  Right now I've just made moving become
              * rotating when in rotate mode. (See above).
              */
             /*
             var axis = system.self.orientation.mul(
                     system.self.modelOrientation.mul(<0, 1, 0>));
             //system.__debugPrint('axis: ' + std.core.pretty(axis) + '\n');
             var target = msg.orient.mul(<0, 1, 0>);
             //system.__debugPrint('target: ' + std.core.pretty(target) + '\n');
             var corrAxis = target.cross(axis);
             //system.__debugPrint('corr: ' + std.core.pretty(corrAxis) + '\n');
             var correction = (new util.Quaternion(corrAxis.x, corrAxis.y,
                                                  corrAxis.z, 1 +
                                                  target.dot(axis))).normal();
             var newOrient = correction.mul(msg.orient);
             //system.__debugPrint('new y: ' + std.core.pretty(newOrient.mul(<0, 1, 0>)) + '\n\n');
             system.self.orientation = newOrient.mul(system.self.
                                       modelOrientation.inverse());
              */

             system.self.orientation = msg.orient.mul(system.self.
                                       modelOrientation.inverse());
             for(a in axes)
                 axes[a].orientation = system.self.orientation;
             if(movable)
                 movable.setOrientation(system.self.orientation);

             msg.makeReply({}) >> [];
         }

         function setScale(msg, sender) {
             system.self.scale = msg.scale;
             for(var a in axes)
                 axes[a].scale = system.self.scale;
             if(movable)
                 movable.setScale(system.self.scale / 2);
             msg.makeReply({}) >> [];
         }

         function follow(msg, sender) {
             if(typeof(msg) === 'undefined') {
                 if(movable) {
                     msg = {id: movable._remote.toString()};
                 } else {
                     msg = {id: ''};
                 }
             }

             if(msg.id !== '') {
                 if(msg.id in axisIDs)
                     // selecting an axis shouldn't make it follow itself!
                     return;

                 if(!movable || movable._remote.toString() != msg.id)
                     movable = new std.movement.MovableRemote(
                             system.createVisible(msg.id));

                 for(var a in axes) {
                     axes[a].mesh = axisMeshes[a];
                     axes[a].position = movable.getPosition();
                     axes[a].orientation = movable.getOrientation();
                     axes[a].scale = movable.getScale() * 2;
                 }
             } else {
                 movable = null;
                 for(var a in axes) {
                     axes[a].mesh = '';
                 }
             }
         }

         function toggleRotateMode(msg, sender) {
             mode = (mode == 'move' ? 'rotate' : 'move');
             axisMeshes = (mode == 'move' ? moveMeshes : rotateMeshes);
             for(var a in axes)
                 if(axes[a].mesh != '')
                     axes[a].mesh = axisMeshes[a];
         }

         function snapLocal(msg, sender) {
             for(var a in axes)
                 axes[a].modelOrientation = orientations[a];

             follow();
         }

         function snapGlobal(msg, sender) {
             if(!movable)
                 return;

             for(var a in axes)
                 axes[a].modelOrientation = movable.getOrientation().inverse().
                                            mul(orientations[a]);

             follow();
         }

         function forwardScriptRequest(msg, sender) {
             if(!movable)
                 return;

             msg >> movable._remote >> [];
         }

         system.onPresenceConnected(onPresenceConnected);
     }

     std.graphics.Graphics.prototype.onAxesCreated = function(msg, sender) {
         this._axes._axesVisible = sender;
     };

     std.graphics.Graphics.prototype.createAxes = function(self) {
         std.core.bind(this.onAxesCreated, this) << [{'axes':'created':}];

         system.createEntityScript(<0, 0, 0>, axesScript,
                                   {avatar: self},
                                   self.queryAngle, redAxisMesh, 1);
     };

     std.graphics.Graphics.prototype._axes = {};

     std.graphics.Graphics.prototype._axes.follow = function(vis) {
         if (!this._axesVisible) return;
         if(vis)
             {request: 'axes', id: vis.toString()} >> this._axesVisible >> [];
         else
             {request: 'axes', id: ''} >> this._axesVisible >> [];
     };

     std.graphics.Graphics.prototype._axes.toggleRotateMode = function() {
         if (!this._axesVisible) return;
         {request: 'axes', mode: 'toggle'} >> this._axesVisible >> [];
     };

     std.graphics.Graphics.prototype._axes.snapLocal = function() {
         if (!this._axesVisible) return;
         {request: 'axes', snap: 'local'} >> this._axesVisible >> [];
     }

     std.graphics.Graphics.prototype._axes.snapGlobal = function() {
         if (!this._axesVisible) return;
         {request: 'axes', snap: 'global'} >> this._axesVisible >> [];
     }

})();
