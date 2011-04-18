
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

b = 13;

//comment below out to get reference; comment below,below out to get actual.
var a= <3+1,b*b, 1>;
//var a = new util.Vec3(3+1,b*b,1);

a.print();

print("\n\nAll done\n\n");