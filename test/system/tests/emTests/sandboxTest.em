/**
 sandboxTest -- Tests isolation and capability checks for sandboxes.
 
  Scene.db:
  * Ent 1 : Anything

  Duration 7s
 
 */

system.require('helperLibs/util.em');
mTest = new UnitTest('sandboxTest');

system.onPresenceConnected(runTests);

function runTests()
{
    testIsolation();
    testTimeoutInSandbox();
    testPassedThroughSelf();
    testTryExceedCaps();

    //lkjs;
    // testTryReceiveMessage();
    // testProximityCallbacks();

    //print success or fail, and killEntity.
    endTests();
}

//writes a global variable.  Then creates a new sandbox.  If this new
//sandbox can access the global variable set by the parent, then throw
//failure.  It then creates a variable with the same name as the
//global variable, and writes to it.  After execution of the sandbox,
//check to ensure that global variable's original value is maintained.
//If not, throw error.
function testIsolation()
{
    if (typeof(x) !== 'undefined')
        mTest.fail('Error in *construction* of test for sandboxTest.  require that x not have been previously defined in global scope to avoid confusion.');
    
    x = 5;
    var caps = new util.Capabilities(util.Capabilities.IMPORT);
    var whichPresence = system.self;
    var newSandbox = caps.createSandbox(whichPresence,null);
    newSandbox.execute(
        function()
        {
            system.require('helperLibs/util.em');
            mTest = new UnitTest('sandboxTest');

            if (typeof(x) !== 'undefined')
                mTest.fail('Failure in testIsolation: variable that was defined in parent sandbox visible to child.');

            x = 3;
        }
    );


    if (x !== 5)
        mTest.fail('Failure in testIsolation: changing variable in child sandbox affected value outside of sandbox.');

    delete x;
}

//checks to ensure that data persists between execute calls and that
//callbacks actually occur in sandboxes and they maintain isolation.
function testTimeoutInSandbox()
{
    var caps = new util.Capabilities(util.Capabilities.IMPORT);
    var newSandbox = caps.createSandbox(system.self,null);
    newSandbox.execute(
        function()
        {
            haveExecutedTimeout = false;
            system.timeout(1,
                           function()
                           {
                               haveExecutedTimeout = true;
                               if (typeof(mTest) != 'undefined')
                                   mTest.fail('Failure in timeout in sandbox.  Had access to data from parent sandbox when exectued function.');
                           }
                          );
        });

    system.timeout(1.5,
                   function()
                   {
                       newSandbox.execute(
                           function()
                           {
                               if (typeof(haveExecutedTimeout) == 'undefined')
                               {
                                   system.require('helperLibs/util.em');
                                   mTest = new UnitTest('sandboxTest');
                                   mTest.fail('Failure when calling timeout from sandbox.  Data does not persist between sandbox executions.');
                               }
                               else if (!haveExecutedTimeout)
                               {
                                   system.require('helperLibs/util.em');
                                   mTest = new UnitTest('sandboxTest');
                                   mTest.fail('Failure when calling timeout from sandbox.  Timeout event never got called.');
                               }
                           }
                       );
                       
                   });
}



//checks that the presence that's used as self in new sandbox is the
//presence that was used when creating the sandbox.
function testPassedThroughSelf()
{
    var caps = new util.Capabilities(util.Capabilities.IMPORT);
    system.self.setPosition(new util.Vec3(-10,0,1));
    var newSandbox = caps.createSandbox(system.self,null);
    newSandbox.execute(
        function()
        {
            if (system.self.getPosition() != (new util.Vec3(-10,0,1)))
            {
                system.print('\n\n');
                system.print(system.self.position.x);
                system.print('  ');
                system.print(system.self.position.y);
                system.print('  ');
                system.print(system.self.position.z);
                system.print('\n\n');
                
                system.require('helperLibs/util.em');
                mTest = new UnitTest('sandboxTest');
                mTest.fail('Error in testPassedThroughSelf.  self inside of sandbox should be identical to presence passed through.');
            }
        });
}


//Create a sandbox that tries to execute a bunch of calls that it
//shouldn't have the capability to execute.  If any of them do not
//throw an error, then fail unit test.

//DOES NOT TEST:
//skip receive message for not.
//skip import (must import to register error).
//skip prox callbacks
function testTryExceedCaps()
{
    var caps = new util.Capabilities(util.Capabilities.IMPORT);
    var newSandbox = caps.createSandbox(system.self,null);
    newSandbox.execute(
        function()
        {
            //import failure code
            system.require('helperLibs/util.em');
            mTest = new UnitTest('sandboxTest');
            
            //send message
            try
            {
                {'a':'b'} >> system.self >> [];
                mTest.fail('Error in testTryExceedCaps.  Could send a message when should not have been able to.');
            }
            catch(excep)
            { }

            
            //createPresence
            try
            {
                system.createPresence();
                mTest.fail('Error in testTryExceedCaps.  Could create a presence when should not have been able to.');               
            }
            catch (excep)
            { }

            //createEntity
            try
            {
                system.createEntityScript(
                    system.self.position,
                    function(){ },
                    null,
                    3,
                    system.self.getMesh()
                );

                mTest.fail('Error in testTryExceedCaps.  Could create an entity when should not have been able to.');
            }
            catch (excep)
            { }

            //eval
            try
            {
                eval('');
                mTest.fail('Error in testTryExceedCaps.  Could call eval when should not have been able to.');
            }
            catch (excep)
            { }

            //prox queries
            try
            {
                system.self.setQueryAngle(1);
                mTest.fail('Error in testTryExceedCaps.  Could change query angle when should not have been able to.');
            }
            catch(excep)
            {}

            //create sandbox
            try
            {
                var caps = new util.Capabilities();
                var newSandbox = caps.createSandbox(system.self,null);
                mTest.fail('Error in testTryExceedCaps.  Should not have been able to create a sandbox.');
            }
            catch(excep)
            {}

            //GUI
            try
            {
                system.require('std/client/default.em');
                simulator = new std.client.Default(system.self, function(){});

                mTest.fail('Error in testTryExceedCaps.  Should not have been able to perform a gui command.');
            }
            catch(excep)
            {}

            //HTTP
            try
            {
                system.require('std/http/http.em');
                var url ='http://open3dhub.com/api/browse/';
                std.http.basicGet(url,function(){});
                mTest.fail('Error in testTryExceedCaps.  Should not have been able to issue http get.');
            }
            catch(excep)
            {}

            //movement position
            try
            {
                system.self.setPosition(new util.Vec3(1,0,0));
                mTest.fail('Error in testTryExceedCaps.  Should not have been able to change position.');
            }
            catch (excep)
            {}

            //movement velocity
            try
            {
                system.self.setVelocity(new util.Vec3(1,0,0));
                mTest.fail('Error in testTryExceedCaps.  Should not have been able to change velocity.');
            }
            catch (excep)
            {}

            //movement orientation
            try
            {
                system.self.setOrientation(new util.Quaternion(1,0,0,0));
                mTest.fail('Error in testTryExceedCaps.  Should not have been able to change orientation.');
            }
            catch (excep)
            {}
            
            //movement orientationVel
            try
            {
                system.self.setOrientationVel(new util.Quaternion(1,0,0,0));
                mTest.fail('Error in testTryExceedCaps.  Should not have been able to change orientation velocity.');
            }
            catch (excep)
            {}

            //mesh
            try
            {
                system.self.setMesh('');
                mTest.fail('Error in testTryExceedCaps.  Should not have been able to change mesh.');
            }
            catch (excep)
            {}

            //should still be able to read position, orientation, velocities, mesh, query angle.
            try
            {
                system.print('\nBefore checking what is readable.\n');
                system.self.getPosition();
                system.print('\nPosition\n');
                system.self.getOrientation();
                system.print('\nOrientation\n');
                system.self.getVelocity();
                system.print('\nVelocity\n');
                system.self.getOrientationVel();
                system.print('\nOrientationVel\n');
                system.self.getMesh();
                system.print('\ngetMesh\n');
                system.self.getQueryAngle();
                system.print('\ngetQueryAngle\n');
            }
            catch(excep)
            {
                mTest.fail("Error in testTryExceedCaps.  Should have been able to read position, orientation, velocities, mesh, and query angle, but couldn\'t.");
            }
        });
}


function endTests()
{
    if (!mTest.hasFailed)
        mTest.success('Tests complete in sandbox tests.');

    system.killEntity();
}
