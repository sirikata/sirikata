/**
 Duration: 100s
 Scene.db:
    *Ent 1: Anything.

 Tests:
    -onPresenceConnected
    -timers
    -self
    -Date

  
Stage 1

 Wait until our presence connects.  Then, when we get a new
 presence, store its system.self.toString().  Fire a timer 5 seconds
 from now.

 <wait 5 seconds>

Stage 2: Repeat with stage 3 10 times.
 If timer fires and uses same system.self, then good so far.  Log the
 current time using Date.  Set a timer for 4 seconds from now. Also
 check system.self.

 <wait 4 seconds>

Stage 3 
 Log finish Date when timer fires.  If finish date is not within some
 tolerance of 4 seconds, then error.

Stage 4
 Set two timers, one for 1 second
 from now, one for 1.5 seconds from now.  Also check system.self.

 <wait until second timer fires>

Stage 5
 (long timer callback).
 If second timer fired before first, print error message.
 Call stage 6 directly.

Stage 6

 Create two timers, a long one (5s) and a short one(1s).  After
 creating short one, clear it.

 <wait until long timer fires>

Stage 7
 (long timer callback).

 If the short timer fires before this callback happened, print error.
 Set up two more timers, one long (5s) and one short(1s).  Suspend the
 short timer.

 <wait until long timer fires>

Stage 8
 (long timer callback)
 
 If the short timer fired before this callback happened, print error.
 Call reset on suspended short timer.  And set a long timer (5s).

 
 <wait until long timer fires>

Stage 9
 (long timer callback)

 If the short timer did not fire, print error.  Also check success
 object and print success or fail corresponding to it.

 */


system.require('helperLibs/util.em');
mTest = new UnitTest('timeoutTest.em');

system.onPresenceConnected(stage1);


//ugh: I'm having a hard time deciding on exactly how defaults should
//work for this object.  should be false if failed, true if succeeded.
successObject = {
    'firstTimerFired': false,
    'systemSelfMatches': true,
    'timerWithinTolerance': true,
    'shortBeforeLong': false,
    'timerCanceled': false,
    'timerSuspended': false,
    'timerReset': false
};

//timers being measured should fire within .3 seconds of when they were requested to.
timerTolerance = .2;
timerTolerancePeriod = 1;
loggedSystemSelf = null;
timesToRepeatStage2And3 = 10;
countTimesExecutedStage2And3 = 0;
shortExecuted = false;

//entry point.
function stage1()
{
    system.__debugPrint('\nstage1\n');
    loggedSystemSelf = system.self.toString();
    system.timeout(1,stage2);
}

function stage2()
{
    system.__debugPrint('\nstage2\n');
    checkSelf();
    
    stage2Date = new Date();
    successObject['firstTimerFired'] = true;
    if (countTimesExecutedStage2And3 >= timesToRepeatStage2And3)
        stage4();
    else
        system.timeout(timerTolerancePeriod,stage3);
}


function stage3()
{
    system.__debugPrint('\nstage3\n');
    var stage3Date = new Date();
    var diffTimes = (stage3Date - stage2Date)/1000.0;
    if (Math.abs(diffTimes - timerTolerancePeriod) > timerTolerance)
    {
        successObject['timerWithinTolerance'] = false;
        mTest.fail('timer not within tolerance of when it should have been fired.  Tolerance: ' + timerTolerance.toString() + ' actually off by: ' + diffTimes.toString() + ' (in seconds).');            
    }

    ++countTimesExecutedStage2And3;
    checkSelf();
    stage2();
}


function stage4()
{
    system.__debugPrint('\nstage4\n');
    checkSelf();
    system.timeout(1,shortTimeoutCallback);
    system.timeout(2,stage5);
}


function stage5()
{
    system.__debugPrint('\nstage5\n');
    checkSelf();
    if (!shortExecuted)
    {
        mTest.fail('long timeout executed before short');
        successObject['shortBeforeLong'] = false;
    }
    else
        successObject['shortBeforeLong'] = true;

    shortExecuted = false;
    stage6();
}

function stage6()
{
    system.__debugPrint('\nstage6\n');
    checkSelf();
    system.timeout(2, stage7);
    var shortTimeOut = system.timeout(1, shortTimeoutCallback);
    shortTimeOut.clear();
}


function stage7()
{
    system.__debugPrint('\nstage7\n');
    if (shortExecuted)
    {
        mTest.fail('executed on a cleared timer');
        successObject['timerCanceled'] = false;
    }
    else
        successObject['timerCanceled'] = true;

    shortExecuted = false;

    checkSelf();
    system.timeout(2,stage8);
    stage7_shortTimeOut = system.timeout(1,shortTimeoutCallback);
    stage7_shortTimeOut.suspend();
}


function stage8()
{
    system.__debugPrint('\nstage8\n');
    if (shortExecuted)
    {
        mTest.fail('executed on a suspended object');
        successObject['timerSuspended'] = false;
    }
    else
        successObject['timerSuspended'] = true;

    shortExecuted = false;

    checkSelf();
    // reset -> 1 second
    stage7_shortTimeOut.reset();
    system.timeout(2,stage9);
}

function stage9()
{
    system.__debugPrint('\nstage9\n');
    checkSelf();
    if (shortExecuted == false)
    {
        mTest.fail('did not execute from a reset timer');
        successObject['timerReset'] = false;            
    }
    else
        successObject['timerReset'] = true;            

    printSuccessObject(successObject,mTest);
    system.killEntity();
}



function shortTimeoutCallback()
{
    shortExecuted = true;
}



function checkSelf()
{
    if (system.self.toString() !== loggedSystemSelf)
    {
        mTest = new UnitTest('Timer\'s self object does not match.');
        successObject['system self matches'] = false;
    }
    
}