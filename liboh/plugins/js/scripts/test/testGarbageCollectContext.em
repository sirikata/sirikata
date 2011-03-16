
var whichPresence = system.presences[0];
var newContext = system.create_context(whichPresence,null,true,true,true,true);


function testContext()
{
    var x = new Object();
    x.x = 5;
    newContext.execute(toExecute,x,system);
    newContext.execute(toExecute,x,system);
}


function toExecute(argPassedIn, argPassedIn2)
{
    x = 7;
    
    bind = function(func, object) {
        if (typeof(func) === "undefined")
            throw new TypeError("Tried to bind undefined function.");
        if (arguments.length==2) {
            delete arguments;
            return function() {
                return func.apply(object, arguments);
            };
        } else {
            var args = new Array(arguments.length-2);
            for (var i = 2; i < arguments.length; ++i) {
                args[i-2]=arguments[i];
            }
            delete arguments;
            return function () {
                var argLen = arguments.length;
                var newarglist = new Array(argLen);
                for (var i = 0; i < argLen; ++i) {
                    newarglist[i]=arguments[i];
                }
                return func.apply(object,args.concat(newarglist));
            };
        }
    };
    
    RepeatingTimer = function(period,callback)
    {
        var callbackHelper = function()
        {
            this.userCallback();
            this.timer.reset();
        };
    
        this.period       = period;
        this.userCallback = callback;
        this.callback     = bind(callbackHelper,this);
        this.timer        = system.timeout(this.period,null,this.callback);
        this.suspend      = function ()
        {
            return this.timer.suspend();
        };
        this.reset        = function ()
        {
            return this.timer.reset();  
        };
        this.isSuspended  = function ()
        {
            return this.timer.isSuspended();
        };
    };


    
    var funcToReExec = function()
    {
        argPassedIn2.print("Inside of toExecute\n");
        var argAsString = argPassedIn.x.toString();
        argPassedIn2.print("This was argPassedIn: " + argAsString + "\n");
        argAsString = x.toString();
        argPassedIn2.print("This is x: "+ argAsString + "\n");        
    };
    
    
    var tmpFunc = function()
    {
        system.print("\n\nAm executing\n\n");
    };
    var repTimer = new RepeatingTimer(3,funcToReExec);
    //var repTimer = new RepeatingTimer(3,tmpFunc);
};



// function printXOnce()
// {
//     var xAsString = x.toString();
//     system.print("This is x: " + xAsString + "\n" );
// }


// function printXMultiple()
// {
//     var xAsString = x.toString();
//     system.print("This is x: " + xAsString + "\n");
//     system.timeout(5, null, printXMultiple);
// }


// system.timeout(5, null, printXMultiple);
testContext();