/**
 Duration: 30s
 Scene.db:
 
    *Ent 1: this file; init_pos=<0,0,0>; init_scale=1;
    init_query_angle=12.56;

    *Ent 2: std/default.em; init_pos=<10,0,0>; init_scale=1;
    init_query_angle=12.56;

 Tests:
    -onProxAdded,
    -setQueryAngle,
    -setVelocity,
    -createPresence,
    -getProxSet,
    -timeout,
    -onPresenceConnected
 
Stage 1
 We start by setting on proxAdded callback, which should display
 failure if we see anything besides ourselves.

 <Wait 5 seconds>
 
Stage 2 
 Remove old prox callback, and replace with one that just logs if
 we've found something.  We change our query angle to 1, then start
 moving towards <10,0,0> with velocity <3,0,0>.  Stop when we
 see another presence.

 <Wait 4 seconds>: If our proxAdded does not fire in this time, we
 print error.

Stage 3 
 Remove previous prox added.  Set query angle to 12.56 (shouldn't be
 able to see anything besides self).  Add an onProx callback that
 prints error if see anything besides self.  

 Create presence 30 meters away.

 <wait 4 seconds>

Stage 4 
 Remove previous proxAdded call.  Change it to count the number of
 presences that we can see. Also update query angle to .0001.

 <wait 2 seconds>

Stage 5 
 If we didn't see 2 presences (excluding ourselves since proximity
 results don't return our own presence) in proximity query, print
 error.

 If have not yet printed error, then print success.
 */

system.require('helperLibs/util.em');

//global data needed by functions.
mTest = new UnitTest('proximityAdded.em');
success = true;
stage2proxFound = false;
numPresencesSeen = 0;


system.onPresenceConnected(stage1);


//entry point
function stage1()
{
    system.onPresenceConnected(function(){});
    mTest.print('In stage 1.');
    toClearStage1 = system.self.onProxAdded(proxNotUsCallbackFail,true);
    system.timeout(5, stage2);
}


function stage2()
{
    mTest.print('In stage 2.');
    toClearStage1.clear();
    toClearStage2 = system.self.onProxAdded(proxFoundNotUsStage2);
    system.self.setVelocity(new util.Vec3(4,0,0));
    system.self.setQueryAngle(1);
    system.timeout(4,stage3);
}


function stage3()
{
    mTest.print('In stage 3.');
    toClearStage2.clear();
    if (!stage2proxFound)
        failed('Never discovered other presence in stage 2.  Failed.');

    //stop self.
    system.self.setVelocity(new util.Vec3(0,0,0));
    
    system.self.setQueryAngle(12.56);
    toClearStage3 = system.self.onProxAdded(proxNotUsCallbackFail);

    var newPresArgs = {
        'mesh'  : system.self.getMesh(),
        'pos': system.self.getPosition() + (new util.Vec3(30,0,0))
    };
    system.createPresence(newPresArgs);

    system.timeout(4, stage4);
}

function stage4()
{
    mTest.print('In stage 4.');
    toClearStage3.clear();
    toClearStage4 = system.self.onProxAdded(countNumVisiblesSeenProxCallback,true);
    system.self.setQueryAngle(.0000001);
    system.timeout(4,stage5);
}


function stage5()
{
    mTest.print('In stage 5.');
    if (numPresencesSeen != 2)
        failed('In stage4, did not see 2 visibles from setting prox query.');

    var fullProxSet = system.getProxSet(system.self);
    var count =0;
    for (var s in fullProxSet)
        ++count;


    if (count != 2)
        failed('In stage 5, prox result set should contain 2 visibles');            

    
    if (success)
        mTest.success('All checks passed');

    system.__debugPrint('\n\n\n\n');
    system.killEntity();
}


/*************Test helper functions ****************/


//whenever what this returns gets called, we know that there's been an
//error, and print it.
function failed(reason)
{
    success= false;
    mTest.fail(reason);
}


//proxAddedCallback for if we see anything in our proxResultSet that
//isn't us, then fail.
function proxNotUsCallbackFail(newlyVisible)
{
    if (newlyVisible.toString() != system.self.toString())
        failed("Got a proximity callback for a visible that wasn't us when our query angle should have been too small to see it.");
}


function proxFoundNotUsStage2(newlyVisible)
{
    if (newlyVisible.toString() != system.self.toString())
    {
        if (stage2proxFound)
            failed('In stage 2, found two visibles that were not me when should have only found one.');
        
        stage2proxFound = true;            
    }
}

function countNumVisiblesSeenProxCallback(visible)
{
    ++numPresencesSeen;
}
