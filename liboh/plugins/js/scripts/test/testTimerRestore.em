
function funcCallback()
{
    system.print('\n\nI got timed out\n\n');
}

var a = system.timeout(5,funcCallback);

var aAllData = a.getAllData();



a.clear();


function recreateA()
{
    system.print('\n');
    system.print(aAllData);
    system.print('\n');
    // system.print(aAllData.period);
    // system.print('\n');
    // system.print(aAllData.contextId);
    // system.print('\n');
    // system.print(aAllData.timeRemaining);
    // system.print('\n');
    // system.print(aAllData.isSuspended);
    // system.print('\n');
    // system.print(aAllData.isCleared);
    // system.print('\n');
    
    var b = system.timeout(aAllData.period,
                           aAllData.cb,
                           aAllData.contextId,
                           aAllData.timeRemaining,
                           aAllData.isSuspended,
                           aAllData.isCleared
                          );
}


system.timeout(5, recreateA);



