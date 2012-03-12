/**
 Run with createPresenceTestListener.em

 scene.db
   Ent 1 (running this file):
     init_pos=<20,0,0>; init_scale=1;

   Ent 2 (running createPresenceTestListener):
     init_pos=<25,0,0>; init_scale=1; init_query_angle=.00001;


Stage1:
 When we get a presence, set a timeout for five seconds;

 <wait 5 seconds>

Stage2:
 
 Create 5 presences, use special information about their scales,
 meshes, positions, orientations, etc.

 <wait 5 seconds>

Stage 3:
 Repeat Stage2 4 times.

 <wait 10 seconds>

Stage 4:
 If didn't get 20 presence callbacks, print error.
 If didn't system.presences vector does not have 21 presences in it (including initially connected presence), print error.
 If created presences don't have the initial information that you thought to designate them with, then print error.
 Otherwise, print success.  (Note that createPresenceTestListener.em will also be printing successes and failures.)
 
 */


system.require('helperLibs/util.em');

//global data needed by functions.
mTest = new UnitTest('createPresence.em');
numPresencesOfEachType = 5;
expectedNumNewPresences = 20;
countNumPresencesRegistered= 0;

//to check positions for
presType1 = {
    'mesh':'meerkat:///kchung25/reddot.dae/optimized/reddot.dae',
    'scale':3,
    'callback': presPosTypeCallbackFactory('type1'),
    'solidAngleQuery': 1.3,
    'pos': new util.Vec3(15,0,23),
    'vel': new util.Vec3(0,0,0),
    'orientVel': new util.Quaternion (0,0,0,1),
    'orient': new util.Quaternion(0,0,0,1)
};


presType2 = {
    'mesh':'meerkat:///kchung25/reddot.dae/optimized/reddot.dae',
    'scale':3,
    'callback': presPosTypeCallbackFactory('type2'),
    'solidAngleQuery': .9,
    'pos': new util.Vec3(-15.2,8,2.5),
    'vel': new util.Vec3(0,0,0),
    'orientVel': new util.Quaternion (0,0,0,1),
    'orient': new util.Quaternion(0,0.44721359,0,.894427)
};



//to check velocities for
presType3 = {
    'mesh':'meerkat:///kittyvision/pencil_eraser_turquoise.dae/optimized/pencil_eraser_turquoise.dae',
    'scale':6.2,
    'callback': presVelTypeCallbackFactory('type3'),
    'solidAngleQuery': .01,
    'pos': new util.Vec3(0,0,0),
    'vel': new util.Vec3(3.5,0,0),
    'orientVel': new util.Quaternion (0,0.44721359,0,.894427),
    'orient': new util.Quaternion(0,0,0,1)
};


presType4 = {
    'mesh':'meerkat:///zhihongmax/brick_rough_tan_2_2_2.dae/optimized/brick_rough_tan_2_2_2.dae',
    'scale':.4,
    'callback': presVelTypeCallbackFactory('type4'),
    'solidAngleQuery': .3,
    'pos': new util.Vec3(0,0,0),
    'vel': new util.Vec3(-1,-1.2,-10),
    'orientVel': new util.Quaternion (0,0.44721359,0,.894427),
    'orient': new util.Quaternion(0,0,0,1)
};


system.onPresenceConnected(stage1);

function stage1(presConn, clearable)
{
    clearable.clear();
    for (var s = 0; s < numPresencesOfEachType; ++s)
    {
        system.createPresence(presType1);
        system.createPresence(presType2);
        system.createPresence(presType3);
        system.createPresence(presType4);
    }
    system.timeout(4,stage4);    
}



function stage4()
{
    if(countNumPresencesRegistered != expectedNumNewPresences)
    {
        mTest.fail('Did not receive callback for enough presences.  Received ' + countNumPresencesRegistered.toString() + '  when expected ' + expectedNumNewPresences.toString());
    }

    if (system.presences.length != expectedNumNewPresences + 1)
    {
        mTest.fail('System.presences vector is not large enough.  Length: ' + system.presences.length.toString() + '.  Expected: ' + (expectedNumNewPresences+1).toString() );
    }


    if (!mTest.hasFailed)
    {
        mTest.success('Success on presence creater side');
    }

    system.killEntity();
}

function testFloatWithinTolerance(lhs,rhs)
{
    if (Math.abs(lhs-rhs) < .001)
        return true;
    
    return false;
}


function checkBasicProperties(toCheck,pres)
{
    ++countNumPresencesRegistered;    
    if ((toCheck['mesh'] !== system.self.getMesh()) || (toCheck['mesh']  !== pres.getMesh()))
    {
        mTest.fail('meshes of new presences did not match with expected');
    }

    if (!testFloatWithinTolerance(toCheck['scale'] ,system.self.getScale()) || !testFloatWithinTolerance(toCheck['scale'], pres.getScale()))
    {
        mTest.fail('scales of new presences did not match with expected.  Expected: ' + system.self.getScale().toString() + '.  Acutal: ' + system.self.getScale() + '.');
    }


    if (!testFloatWithinTolerance(toCheck['solidAngleQuery'],system.self.getQueryAngle()) ||  !testFloatWithinTolerance(toCheck['solidAngleQuery'], pres.getQueryAngle()))
    {
        mTest.fail('query angles of new presences did not match with expected');
    }
}


function vecCompareWithinTolerance(vec1,vec2)
{
    var tolerance = .1;
    if (Math.abs(vec1.x - vec2.x) > tolerance)
        return false;
    if (Math.abs(vec1.y - vec2.y) > tolerance)
        return false;
    if (Math.abs(vec1.z - vec2.z) > tolerance)
        return false;
    
    return true;
}

function quatCompareWithinTolerance(quat1,quat2)
{
    var tolerance = .1;
    if (Math.abs(quat1.x - quat2.x) > tolerance)
        return false;
    if (Math.abs(quat1.y - quat2.y) > tolerance)
        return false;
    if (Math.abs(quat1.z - quat2.z) > tolerance)
        return false;

    if (Math.abs(quat1.w - quat2.w) > tolerance)
        return false;
    
    return true;
}


function presPosTypeCallbackFactory(typeName)
{
    var returner = function (pres)
    {
        var toCheck = null;
        if (typeName == 'type1')
            toCheck = presType1;
        else
            toCheck = presType2;
        
        checkBasicProperties(toCheck,pres);
//        if ((toCheck['pos'] !== system.self.getPosition()) || (toCheck['pos'] !== pres.getPosition() ))
        if (!vecCompareWithinTolerance(toCheck['pos'],system.self.getPosition()) || !vecCompareWithinTolerance(toCheck['pos'],pres.getPosition() ))
        {
            mTest.fail('Created presence did not have expected position.  Expected: <' + toCheck['pos'].x.toString() + ',' + toCheck['pos'].y.toString() + ',' + toCheck['pos'].z.toString()+ '.  Got: <' + system.self.getPosition().x.toString() + ',' + system.self.getPosition().y.toString()  + ',' + system.self.getPosition().z.toString() + '>');
        }


        if (!quatCompareWithinTolerance(toCheck['orient'] ,system.self.getOrientation()) || !quatCompareWithinTolerance(toCheck['orient'], pres.getOrientation()) )
        {
            mTest.fail('Created presence did not have expected orientation.   Expected: <' + toCheck['orient'].x.toString() + ',' + toCheck['orient'].y.toString() + ',' + toCheck['orient'].z.toString() +',' + toCheck['orient'].w.toString() + '>.  Got: <' + system.self.getOrientation().x.toString() + ',' + system.self.getOrientation().y.toString() + ',' + system.self.getOrientation().z.toString() + ',' + system.self.getOrientation().w.toString() + '>');
        }

            
    };
    return returner;
}


function presVelTypeCallbackFactory(typeName)
{
    var returner = function (pres)
    {
        var toCheck = null;
        if (typeName == 'type3')
            toCheck = presType3;
        else
            toCheck = presType4;


        checkBasicProperties(toCheck,pres);

        if (!vecCompareWithinTolerance(toCheck['vel'],system.self.getVelocity()) ||  !vecCompareWithinTolerance(toCheck['vel'], pres.getVelocity() ))
        {
            
            mTest.fail('Created presence did not have expected velocity.  Expected: <' + toCheck['vel'].x.toString() + ','+ toCheck['vel'].y.toString() + ',' + toCheck['vel'].z.toString() + '>' + '.   Actual: ');
        }

        if (!quatCompareWithinTolerance(toCheck['orientVel'],system.self.getOrientationVel()) || !quatCompareWithinTolerance(toCheck['orientVel'], pres.getOrientationVel() ))
        {
            mTest.fail('Created presence did not have expected orientation velocity');
        }

            
    };
    return returner;
}
