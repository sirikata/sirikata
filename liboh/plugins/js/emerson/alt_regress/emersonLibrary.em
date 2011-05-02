
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

system.registerHandler = function (a, b, c, d)
{
    ++this.incrementer;

    print("\nRegistered handler\n");
    print(system.incrementer);
    print(a);
    print(b);
    print(c);
    print(d);
    print("\n\nDone registering handlers\n\n");
};


