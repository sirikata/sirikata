var minDist = 6;
var howFastToEvadeConst = 3;
var evading = 0;
var howLongToFlee = 2;
var orientationCallbackResolution = .2; //will blend quaternions every .2 seconds
var blendQuaternion_factor = .7; //how much of goal quaternion vs how much of current orientation. (high is more of goal).

var runOrientationCallback = true;
//var runOrientationCallback = false;

var runDartingAboutCallback = true;
var scaleRandomness = 3.0;

var realCenter= new system.Vec3(0,0,-12);
var maxRadiusFromCenter = 3;
var dartingAboutCallbackPeriod = 1; //how long to go before calling darting about callback
var minVelocitySquared = 1.0;
var constBirdVelocity = .7;

 var returner = system.sqrt(diffX*diffX + diffY*diffY + diffZ*diffZ);



// array declaration

var z =  [
    [(qWqW + qXqX - qYqY - qZqZ) / d,
     2 * (qWqZ + qXqY) / d,
     2 * (qXqZ - qWqY) / d, 0],
    [2 * (qXqY - qWqZ) / d,
     (qWqW - qXqX + qYqY - qZqZ) / d,
     2 * (qWqX + qYqZ) / d, 0],
    [2 * (qWqY + qXqZ) / d,
     2 * (qYqZ - qWqX) / d,
     (qWqW - qXqX - qYqY + qZqZ) / d, 0],
    [0, 0, 0, 1]];


var z = { field1:2, field2:"field2"};


