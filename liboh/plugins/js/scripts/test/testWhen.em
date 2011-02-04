

system.print("\n\nTesting when function\n\n");


var x = util.create_watched();
x.health = 22;

//1: a predicate function to check
//2: a callback function to callback
//3: null or a sampling period
//4-end: list of watchable args this depends on

util.create_when(
    function()
    {
        //if (x.health < 3)
        //return true;

        return true;
    },
    function ()
    {
        system.print("\n\nGot into print function\n\n");
    },
    5,
    x);






