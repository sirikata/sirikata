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
         __internalPrint(this.testName,UNIT_TEST_FAILURE_SUCCESS,toPrint);
     };

     
 })();



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