
var minDist = 6;
var howFastToEvadeConst = 3;
var evading = false;
var howLongToFlee = 2;
var orientationCallbackResolution = .5; //will blend quaternions every .2 seconds
var blendQuaternion_factor = .5; //how much of goal quaternion vs how much of current orientation.
var runOrientationCallback = true;


//this function returns an object that has converted the relevant position fields from strings to a vec3
function parseLocationFromObject(object)
{
    var returner = new system.Vec3(parseFloat(object.positionX),parseFloat(object.positionY),parseFloat(object.positionZ));
    return returner;
}

function distance(vecA,vecB)
{
    var diffX = vecA.x - vecB.x;
    var diffY = vecA.y - vecB.y;
    var diffZ = vecA.z - vecB.z;
    
    var returner = system.sqrt(diffX*diffX + diffY*diffY + diffZ*diffZ);

    return returner;
}

function cameraLocationCallback(object, sender)
{
    var camLocation = parseLocationFromObject(object);
    var curPosition = system.presences[0].getPosition();

    if (distance(camLocation,curPosition) < minDist)
    {
        system.print("\n\n\nSQUAWK!  SQUAWK!  Evasive maneuvers\n\n");
        evasiveManeuvers(camLocation,curPosition);
    }
}

//this function takes in two position vectors associated with the camera location and current position.
//it sets velocity to be in the opposite direction of the camera
function evasiveManeuvers(camLocation,curPosition)
{
    evading = true;
    var velocityDirection = new system.Vec3(curPosition.x-camLocation.x,curPosition.y-camLocation.y, curPosition.z-camLocation.z);
    var newVel = scalarMultVec(howFastToEvadeConst, (normalizeVector(velocityDirection)));
    system.presences[0].setVelocity(newVel);
    system.timeout(howLongToFlee,null,regularOperationCallback);
    
}

function normalizeVector(vec)
{
    var normConst = system.sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
    return scalarMultVec(1.0/normConst,vec);
}

function scalarMultVec(scalar, vec)
{
    return new system.Vec3(scalar*vec.x, scalar*vec.y, scalar*vec.z);
}


function regularOperationCallback()
{
    evading = false;
    system.presences[0].setVelocity(new system.Vec3(0,0,0));
    
}

//listenging for camera location
var cameraLocationPattern = new system.Pattern("command","locationCamera");
var cameraLocationHandler = system.registerHandler(cameraLocationPattern,null,cameraLocationCallback,null);


//orientation library

function int_RotationToQuaternion(m) {
    /* Shoemake SIGGRAPH 1987 algorithm */
    var fTrace = m[0][0]+m[1][1]+m[2][2];
    if (fTrace == 3.0 ) 
    {
        return [0,0,0,1];//optional: identify identity as a common case
    }
    if ( fTrace > 0.0 )
    {
        // |w| > 1/2, may as well choose w > 1/2
        var fRoot = system.sqrt(fTrace + 1.0);  // 2w
        var ifRoot=0.5/fRoot;// 1/(4w)
        return [(m[2][1]-m[1][2])*ifRoot,
                (m[0][2]-m[2][0])*ifRoot,
                (m[1][0]-m[0][1])*ifRoot,
                0.5*fRoot];  
                
    }
    else
    {
        // |w| <= 1/2
        var s_iNext=[ 1, 2, 0 ];
        var i = 0;
        if ( m[1][1] > m[0][0] )
            i = 1;
        if ( m[2][2] > m[i][i] )
            i = 2;
        var j = s_iNext[i];
        var k = s_iNext[j];
        var fRoot = system.sqrt(m[i][i]-m[j][j]-m[k][k] + 1.0);
        var ifRoot=0.5/fRoot;
        var q=[0,0,0,(m[k][j]-m[j][k])*ifRoot];
        q[i] = 0.5*fRoot;
        q[j] = (m[j][i]+m[i][j])*ifRoot;
        q[k] = (m[k][i]+m[i][k])*ifRoot;
        return q;
    }
}

int_quaternionToRotation = function(q) {
  var qX = q[0];
  var qY = q[1];
  var qZ = q[2];
  var qW = q[3];

  var qWqW = qW * qW;
  var qWqX = qW * qX;
  var qWqY = qW * qY;
  var qWqZ = qW * qZ;
  var qXqW = qX * qW;
  var qXqX = qX * qX;
  var qXqY = qX * qY;
  var qXqZ = qX * qZ;
  var qYqW = qY * qW;
  var qYqX = qY * qX;
  var qYqY = qY * qY;
  var qYqZ = qY * qZ;
  var qZqW = qZ * qW;
  var qZqX = qZ * qX;
  var qZqY = qZ * qY;
  var qZqZ = qZ * qZ;

  var d = qWqW + qXqX + qYqY + qZqZ;
  system.print("IS d zero "+d);
  return [
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
};



function axisRotation (axis, angle) {
  var d = 1 / system.sqrt(axis[0] * axis[0] +
                        axis[1] * axis[1] +
                        axis[2] * axis[2]);
  var sin = system.sin(angle / 2);
  var cos = system.cos(angle / 2);
  return [sin * axis[0] * d, sin * axis[1] * d, sin * axis[2] * d, cos];
};


function forwardToQuaternion(ev) {
    var l=system.sqrt(ev.x*ev.x+ev.y*ev.y+ev.z*ev.z);
    var input=[-ev.x/l,-ev.y/l,-ev.z/l];
    var y=input;
    var up=[0,1,0];
    var x=[1,0,0];
    if (ev.y==0&&ev.z==0) {
        x=[0,1,0];
    }
    function cross (a, b) {
        return [a[1] * b[2] - a[2] * b[1],
                a[2] * b[0] - a[0] * b[2],
                a[0] * b[1] - a[1] * b[0]];
    }
    function len (a) {
       return system.sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    }
    function renorm (a) {
        var l = len(a);
        return [a[0]/l,a[1]/l,a[2]/l];
    }
    var rotationaxis=cross(up,input);
    var sinangle = len(rotationaxis);
    var angle = system.asin(sinangle);
    var q = axisRotation(rotationaxis,angle);
/*
    var mat = [[x[0], y[0], z[0]], [x[1], y[1], z[1]], [x[2], y[2], z[2]]];
    var q = int_RotationToQuaternion(mat);//[x,y,z]fixme: is this x,y,z
    or is this transpose of that 
*/
    system.print("\n\nforwardtoquaternion_out\n\n");
    var sysq = new system.Quaternion(q[0],q[1],q[2],q[3]);
    var test = int_quaternionToRotation(q);
    system.print ("Velocity is ");
    system.print(input);
    system.print("Axis is ");
    system.print(rotationaxis);
    system.print("intermediate Quat is ");
    system.print(q);
    system.print ("Test is ");
    system.print(test);

    return sysq;
}

/* BUSTED!
function forwardToQuaternion(ev) {
    var l=system.sqrt(ev.x*ev.x+ev.y*ev.y+ev.z*ev.z);
    var y=[-ev.x/l,-ev.y/l,-ev.z/l];
    var x=[1,0,0];
    if (ev.y==0&&ev.z==0) {
        x=[0,1,0];
    }
    function cross (a, b) {
        return [a[1] * b[2] - a[2] * b[1],
                a[2] * b[0] - a[0] * b[2],
                a[0] * b[1] - a[1] * b[0]];
    }
    function renorm (a) {
        var l = system.sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
        return [a[0]/l,a[1]/l,a[2]/l];
    }
    var z=renorm(cross(x,y));
    x=renorm(cross(z,y));
    system.print("\n\nforwardtoquaternion\n\n");
    system.print(x);
    system.print(y);
    system.print(z);
    var mat = [[x[0], y[0], z[0]], [x[1], y[1], z[1]], [x[2], y[2], z[2]]];
    var q = int_RotationToQuaternion(mat);//[x,y,z]fixme: is this x,y,z or is this transpose of that 
    system.print("\n\nforwardtoquaternion_out\n\n");
    var sysq = new system.Quaternion(q[0],q[1],q[2],q[3]);
    var test = quaternionToForward(sysq);
    system.print ("Velocity is ");
    system.print(ev);
    system.print(q);
    system.print(test);
    system.print ("Result was");

    return sysq;
}
*/
function quaternionToCardinal(q,ind) {
    var r=int_quaternionToRotation([q.x,q.y,q.z,q.w]);
    system.print("\n\nquaternion to cardinal\n\n");
    system.print(r);
    system.print(r[0]);
    system.print(r[1]);
    system.print(r[2]);
    return new system.Vec3(r[0][ind],r[1][ind],r[2][ind]);//similar fixme from above applies  is it [ind][0] or [0][ind]
}
function quaternionToForward(q) {
    return quaternionToCardinal(q,1);
}function quaternionToUp(q) {
    return quaternionToCardinal(q,2);
}function quaternionToRight(q) {
    return quaternionToCardinal(q,0);
}

function interpolateQuaternion(q1,q2,blend) {
    var oblend=1-blend;
    return new system.Quaternion(q1.x*oblend+q2.x*blend,
                                 q1.y*oblend+q2.y*blend,
                                 q1.z*oblend+q2.z*blend,
                                 q1.w*oblend+q2.w*blend);
}


//end orientation library




//Code to set the orienation of the bird as it's moving
var previousVelocity = system.presences[0].getVelocity();
var previousQuaternionGoal = forwardToQuaternion(previousVelocity);



function orientationCallback()
{
    system.print("\n\nIn orientation callback\n\n");
    var currentVelocity = system.presences[0].getVelocity();
    if ((currentVelocity.x != 0) || (currentVelocity.y != 0) || (currentVelocity.z != 0))
    {
        if (currentVelocity != previousVelocity)
        {
            previousQuaternionGoal = forwardToQuaternion(currentVelocity);
            previousVelocity = currentVelocity;
        }
    }


    var currentQuaternion = system.presences[0].getOrientation();
    var setQuaternion = interpolateQuaternion(currentQuaternion,previousQuaternionGoal ,blendQuaternion_factor);

    system.presences[0].setOrientation(setQuaternion);

    if (runOrientationCallback)
        system.timeout(orientationCallbackResolution,null,orientationCallback);
}

function cancelOrientationCallback()
{
    runOrientationCallback = false;
}

function resumeOrientationCallback()
{
    runOrientationCallback = true;
    orientationCallback();
}

//gets the orientation interpolation callbacks going.
orientationCallback();


