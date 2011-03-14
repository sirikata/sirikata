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


var maxRadiusFromCenter = 3;
var dartingAboutCallbackPeriod = 1; //how long to go before calling darting about callback
var minVelocitySquared = 1.0;
var constBirdVelocity = .7;

var diffX = constBirdVelocity * scaleRandomness;
var diffY = diffX* constBirdVelocity;
var diffZ = 12;

var returner = ((diffX*diffX) + diffY*diffY) + diffZ*diffZ;
print(returner);

if (returner > 200)
    print("\nIt's greater\n\n");
else
    print("\n\nIt's not greater\n");


var qWqW = 1.2;
var qXqX = 2.7;
var qWqZ = .1;
var qXqY = 10;
var qXqZ = -.1;
var qWqX = 1;
var qYqZ = 1.4;
var qWqX = -.1;
var qWqY = 11;
var qYqY = -.17;
var qZqZ = 3;
d = 1.2;

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

print(z);

var z = { field1:2, field2:"field2"};

print(z);
