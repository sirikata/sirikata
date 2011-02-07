

system.print("\n\nTesting when function\n\n");


var x = util.create_watched();
x.health = 22;

//1: a predicate function to check
//2: a callback function to callback
//3: null or a sampling period
//4-end: list of watchable args this depends on
function sometimesTrue()
{
    system.print("\n\nThis is x.health\n");
    system.print(x.health);
    system.print("\n\n");
    
    if (x.health < 3)
    {
        return true;            
    }
    
    return false;
}
function whenCallback()
{
    system.print("\n\n\nGot into print function\n\n");
}

util.create_when(
    sometimesTrue,
    whenCallback,
    10,
    x);








