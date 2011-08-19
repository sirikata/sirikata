(function ()
 {
     var UNIT_TEST_HEADER_BEGIN   = 'UNIT_TEST_BEGIN';
     var UNIT_TEST_HEADER_PRINT   = 'UNIT_TEST_PRINT';
     var UNIT_TEST_HEADER_SUCCESS = 'UNIT_TEST_SUCCESS';
     var UNIT_TEST_HEADER_FAILURE = 'UNIT_TEST_FAIL';
     
     /**
      Should only be called from within util.em
      */
     var __internalPrint = function(testName, header, toPrint)
     {
         system.__debugPrint('\n');
         system.__debugPrint(header + ' testName: ' + testName + ' ' + toPrint);
         system.__debugPrint('\n');
     };

     var __beginTestPrint = function (testName)
     {
         __internalPrint(testName, UNIT_TEST_HEADER_BEGIN, '');
     };
     
     
     UnitTest = function (testName)
     {
         this.testName = testName;
         __beginTestPrint(this.testName);
         this.hasFailed = false;
     };


     UnitTest.prototype.print = function (toPrint)
     {
         __internalPrint(this.testName,UNIT_TEST_HEADER_PRINT,toPrint);
     };

     
     UnitTest.prototype.success = function(toPrint)
     {
         __internalPrint(this.testName,UNIT_TEST_HEADER_SUCCESS,toPrint);
     };

     UnitTest.prototype.fail = function(toPrint)
     {
         __internalPrint(this.testName,UNIT_TEST_HEADER_FAILURE,toPrint);
         this.hasFailed = true;
     };

     
 })();


/**
 Waits timeToWait seconds, then executes beforeKillFunc, then kills
 whichever entity called killAfterTime.
 */
function killAfterTime(timeToWait, beforeKillFunc)
{
    var onTimeout = function()
    {
        if (typeof(beforeKillFunc) == 'function')
            beforeKillFunc();

        system.killEntity();  
    };
    system.timeout(timeToWait,onTimeout);
}


/**
 
 @param {object} sucObj An object with string indices and boolean
 values.  If one of the values is false, then we use unitTestObj to
 print a failure message with the index string as the failure message.
 If *all* items in sucObj are true, then prints success message.

 @param {UnitTest object} unitTestObj.  @see UnitTest.

 */
function printSuccessObject(sucObj, unitTestObj)
{
    var success = true;
    for (var s in sucObj)
    {
        if (!sucObj[s])
        {
            success= false;
            unitTestObj.fail(s);
        }
    }
    if (success)
        unitTestObj.success(' succeeded!');
}