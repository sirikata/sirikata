/**
 Duration 30s
 Scene.db:
    *Ent 1: Anything.

 Tests:
   -std.http.basicGet
   -timeout


 If download is successful for all of them, prints success message.
 Otherwise, prints error message.

Stage 1:
  Issue http gets for google, stanfod, sirikata.com, and google with
 query parameters.

<wait 25 seconds >

Stage 2: 
 Check if all downloads are complete.  And are successful.  If we got information from all of them, then we 
 
 */

system.require('std/http/http.em');
system.require('helperLibs/util.em');
mTest = new UnitTest('timeoutTest.em');

successObject = {
    'google':false,
    'stanford': false,
    'sirikata': false,
    'googleQueryParams':false
};

var callbacks_left = 0;

//system.onPresenceConnected(stage1);
stage1();

function stage1()
{
    system.timeout(30,finish);

    //issue query to google
    url = 'http://www.google.com';
    std.http.basicGet(url,callbackFunctionFactory('google'));

    url = 'http://www.stanford.edu/';
    std.http.basicGet(url,callbackFunctionFactory('stanford'));

    url = 'http://www.sirikata.com/blog/';
    std.http.basicGet(url,callbackFunctionFactory('sirikata'));

    url = 'http://www.google.com/search?client=ubuntu&channel=fs&q=emerson+check+test&ie=utf-8&oe=utf-8';
    std.http.basicGet(url,callbackFunctionFactory('googleQueryParams'));
}

function callbackFunctionFactory(callbackSite)
{
    callbacks_left++;
    var returner = function (success,data)
    {
        var worked = true;
        if (!success)
        {
            successObject[callbackSite] = false;
            mTest.fail('Callback failed on ' + callbackSite);
            worked = false;
        }
        else if (data.statusCode != 200)
        {
            system.__debugPrint('\n\nstatus code: ' + data.statusCode.toString() + '\n\n');
            successObject[callbackSite] = false;
            mTest.fail('Callback did not receive http 200 for ' + callbackSite);
        }
        else if (data.data.length <=0)
        {
            successObject[callbackSite] = false;
            mTest.fail('Callback did not receive http 200 for ' + callbackSite);
        }

        if (worked)
            successObject[callbackSite] = true;

        callbacks_left--;
        if (callbacks_left == 0)
            finish();
    };
    return returner;
}

function finish()
{
    printSuccessObject(successObject,mTest);
    system.killEntity();
}