/* This script is meant to stress test a bunch of features of
 * presences (and system). It executes a wide variety of functions and
 * does so very quickly. Ideally this allows you to find:
 *
 * 1. Relatively rare errors that only occur through a lot of
 * repetition.
 *
 * 2. Shutdown bugs. If we don't clean up properly in the C++ code,
 * this script runs stuff fast enough to make it likely that we'll
 * cause real errors.
 *
 * BE CAREFUL with this script. It will likely cause memory usage to
 * continuously grow because of how fast it does things -- it is best
 * to start it up, let it run for a very short time and then kill the
 * object host.
 */

system.require('std/core/repeatingTimer.em');

// Emitting events
function eventCallback() {
    system.event( function() { return; } );
};

// Presence properties:
// Position
(function() {
     var settings = [<0, 0, 0>, <0, 5, 0>];
     var curSetting = 0;

     setPositionCallback = function() {
         curSetting = (curSetting + 1) % settings.length;
         system.self.position = settings[curSetting];
     }
 })();
// Velocity
(function() {
     var settings = [<0, -5, 0>, <0, 5, 0>];
     var curSetting = 0;

     setVelocityCallback = function() {
         curSetting = (curSetting + 1) % settings.length;
         system.self.velocity = settings[curSetting];
     }
 })();
// Orientation
(function() {
     var settings = [
         new util.Quaternion(<0, 1, 0>, 0),
         new util.Quaternion(<0, 1, 0>, 3.14159/2),
         new util.Quaternion(<0, 1, 0>, 3.14159),
         new util.Quaternion(<0, 1, 0>, 3.14159*3/2)
     ];
     var curSetting = 0;

     setOrientationCallback = function() {
         curSetting = (curSetting + 1) % settings.length;
         system.self.orientation = settings[curSetting];
     }
 })();

// OrientationVel
(function() {
     var settings = [
         new util.Quaternion(<0, 1, 0>, -1),
         new util.Quaternion(<0, 1, 0>, 1)
     ];
     var curSetting = 0;

     setOrientationVelCallback = function() {
         curSetting = (curSetting + 1) % settings.length;
         system.self.orientationVel = settings[curSetting];
     }
 })();

// Mesh
(function() {
     var settings = [
         'meerkat:///jterrace/multimtl.dae/optimized/0/multimtl.dae',
         'meerkat:///jterrace/duck_triangulate.dae/optimized/0/duck_triangulate.dae'
     ];
     var curSetting = 0;

     setMeshCallback = function() {
         curSetting = (curSetting + 1) % settings.length;
         system.self.mesh = settings[curSetting];
     }
 })();

function initTimers() {
    timers = {
        'events' : new std.core.RepeatingTimer(.01, eventCallback),
        'setPosition' : new std.core.RepeatingTimer(.01, setPositionCallback),
        'setVelocity' : new std.core.RepeatingTimer(.01, setVelocityCallback),
        'setOrientation' : new std.core.RepeatingTimer(.01, setOrientationCallback),
        'setOrientationVel' : new std.core.RepeatingTimer(.01, setOrientationVelCallback),
        'setMesh' : new std.core.RepeatingTimer(.01, setMeshCallback)
    };
}

system.onPresenceConnected(initTimers);
if (system.self !== system) initTimers();
