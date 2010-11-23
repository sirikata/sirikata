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
        var fRoot = sqrt(m[i][i]-m[j][j]-m[k][k] + 1.0);
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







function forwardToQuaternion(ev) {
    var l=system.sqrt(ev.x*ev.x+ev.y*ev.y+ev.z*ev.z);
    var z=[ev.x/l,ev.y/l,ev.z/l];
    var x=[1,0,0];
    if (ev.y==0&&ev.z==0) {
        x=[0,1,0];
    }
    function cross (a, b) {
        return [a[1] * b[2] - a[2] * b[1],
                a[2] * b[0] - a[0] * b[2],
                a[0] * b[1] - a[1] * b[0]];
    }
    var y=cross(x,z);
    x=cross(z,y);
    var q = int_RotationToQuaternion([x,y,z]);//fixme: is this x,y,z or is this transpose of that [[x[0],y[0],z[0]][x[1],y[1],z[1]],[x[2],y[2],z[2]]]
    return new system.Quaternion(q[0],q[1],q[2],q[3]);
}

function quaternionToCardinal(q,ind) {
    var r=int_quaternionToRotation([q.x,q.y,q.z,q.w]);
    return new system.Vector3(r[ind][0],r[ind][1],r[ind][2]);//similar fixme from above applies  is it [ind][0] or [0][ind]
}
function quaternionToForward(q) {
    return quaternionToCardinal(q,2);
}function quaternionToUp(q) {
    return quaternionToCardinal(q,1);
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
