
function okayRot()
{
    var axis = new util.Vec3(0, 1, 0);
    var okayRotation = new util.Quaternion(axis, Math.PI * 1.9);
    system.self.setOrientationVel(okayRotation.scale(1));
}

function stopRot()
{
    var axis = new util.Vec3(0, 1, 0);
    var weirdRotation = new util.Quaternion(axis, 0);
    system.self.setOrientationVel(weirdRotation);
}

function fastRot()
{
    var axis = new util.Vec3(0, 1, 0);
    var weirdRotation = new util.Quaternion(axis, Math.PI*1.9);
    system.self.setOrientationVel(weirdRotation.scale(50));
}


