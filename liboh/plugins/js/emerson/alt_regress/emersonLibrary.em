
function __checkResources8_8_3_1__()
{
    return true;
}



util = {    };

util.Vec3 = function (x,y,z)
{
    this.x = x;
    this.y = y;
    this.z = z;
    this.print = function()
    {
        print("\n\nPrinting\n\n");
        print(this.x);
        print(this.y);
        print(this.z);
        print("\n\n");
    };
};

util.minus = function (a,b)
{
    //super primitive checking to see if a is a vector
    if (typeof(a) == 'object')
    {
        //comment out next line and uncomment out line after when running through emerson.  Uncomment next line and uncomment next if running rhino
        return new util.Vec3(a.x-b.x, a.y-b.y, a.z-b.z);            
        //return <a.x-b.x, a.y-b.y, a.z-b.z>;            
    }

    return a-b;
};


util.plus = function(a,b)
{
    //super primitive checking to see if a is a vector
    if (typeof(a) == 'object')
    {
        //comment out next line and uncomment out line after when running through emerson.  Uncomment next line and uncomment next if running rhino
        return new util.Vec3(a.x+b.x,  a.y+b.y,a.z+b.z);            
        //return <a.x+b.x,  a.y+b.y,a.z+b.z>;            
    }

    return a+b;
};

util.patInc = 0;

util.Pattern = function(name,value,proto)
{
    ++util.patInc;
    print('\nIn util.pattern\n');
    print(util.patInc);
    print(name);
    print(value);
    print(proto);
    print('\ndone\n');
};


system = {    };

system.incrementer = 0;

system.registerHandler = function (a, b, c)
{
    ++this.incrementer;

    print("\nRegistered handler\n");
    print(system.incrementer);
    print(a);
    print(b);
    print(c);
    print("\n\nDone registering handlers\n\n");
};

system.self = 'I am Self!';

system.__isResetting = function()
{
    return false;
};

system.sendMessage = function(sender, msgObj, receiver)
{
    print('\nIn system.sendMessage\n');
    print('sender:    ' + sender.toString());
    print('msgObj:    ' + msgObj.toString());
    print('receiver:  ' + receiver.toString());
};



std = {
    };

std.messaging = {
    };

std.messaging.counter = 0;
std.messaging.parseTriple = function(a,b,c)
{
    ++std.messaging.counter;
    print('In parseTriple.\n');
    print('a' + a.toString() + '\n');
    print('b' + b.toString() + '\n');
    print('c' + c.toString() + '\n');
    return std.messaging.counter;
};

std.messaging.sendSyntax = function (a, b)
{
    ++std.messaging.counter;
    print('sendSyntax\n');
    print('a' + a.toString() + '\n');
    print('b' + b.toString() + '\n');
    return std.messaging.counter;
};
  

std.messaging.MessageReceiverSender = function(a,b)
{
    ++std.messaging.counter;
    print('\nMessage receiver sender\n');
    print('a' + a.toString() + '\n');
    print('b' + b.toString() + '\n');
    return std.messaging.counter;
};
