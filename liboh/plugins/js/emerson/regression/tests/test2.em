// This is testing the function declaration/invocation feature of emerson

function distance(vecA,vecB)
{
    var diffX = vecA.x - vecB.x;
    var diffY = vecA.y - vecB.y;
    var diffZ = vecA.z - vecB.z;
    
    var returner = system.sqrt(diffX*diffX + diffY*diffY + diffZ*diffZ);

    return returner;
}


// function call inside function decl
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


// function expressions

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
 };


// function decl inside function decl

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



