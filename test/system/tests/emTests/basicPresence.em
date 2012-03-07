system.require('helperLibs/util.em');

/**
 Just creates a singly-presenced entity that kills itslef after 10
 seconds, printing a success function before shutting down.
 */

mTest = new UnitTest('basicPresence');

function onKill()
{
    mTest.success('about to kill.');
}
killAfterTime(1,onKill);
