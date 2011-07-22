
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

    this.add = function(rhs)
    {
        return new util.Vec3(this.x + rhs.x, this.y + rhs.y, this.z + rhs.z);
    };

    this.sub = function(rhs)
    {
        return new util.Vec3(this.x - rhs.x, this.y - rhs.y, this.z - rhs.z);
    };

    this.scale = function(rhs) {
        return new util.Vec3(this.x * rhs, this.y * rhs, this.z * rhs);
    };

    
    this.mul = function(rhs) {
        if (typeof(rhs) === "number")
            return this.scale(rhs);
        return this.componentMultiply(rhs);
    };

    this.div = function(rhs) {
        return new util.Vec3(this.x / rhs, this.y / rhs, this.z / rhs);
    };


    this.equal = function(rhs){
        if (rhs == null)
            return false;

        //want to prevent general case of saying that a vector is equal to a quaternion.
        if (typeof (rhs.w) != 'undefined')
            return false;
    
        return ((this.x === rhs.x) && (this.y === rhs.y) && (this.z === rhs.z));
    };
    
};


/**
     Overloads the '+' operator.

     Checks if lhs has a plus function defined on it.  If it does, then
     calls lhs.plus(rhs).  If it doesn't, call javascript version of +
     */
    util.plus = function (lhs, rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.add) == 'function')
                return lhs.add(rhs);
        }

        return lhs + rhs;
    };


    /**
     Overloads the '-' operator.
     
     Checks if lhs has a sub function defined on it.  If it does, then
     calls lhs.sub(rhs).  If it doesn't, javascript version of -.
     */
    util.sub = function (lhs,rhs)
    {
        if ((lhs != null)&&(typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.sub) == 'function')
                return lhs.sub(rhs);
        }
        
        return lhs - rhs;
    };

    /**
     Overloads the '/' operator.

     Checks if lhs has a div function defined on it.  If it does, then
     returns lhs.div(rhs).  Otherwise, returns normal js division.
     */
    util.div = function (lhs,rhs)
    {
        if ((lhs != null)&&(typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.div) == 'function')
                return lhs.div(rhs);
        }
        
        return lhs/rhs;
    };

    /**
     Overloads the '*' operator.

     Checks if lhs has a mul function defined on it.  If it does, then
     returns lhs.mul(rhs).  Otherwise, returns normal js *.
     */
    util.mul = function (lhs,rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.mul) == 'function')
                return lhs.mul(rhs);
        }
        
        return lhs* rhs;
    };

    /**
     Overloads the '%' operator.

     Checks if lhs has a mod function defined on it.  If it does, then
     returns lhs.mod(rhs).  Otherwise, returns normal js modulo.
     */
    util.mod = function (lhs,rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.mod) == 'function')
                return lhs.mod(rhs);
        }
        
        return lhs % rhs;
    };


    util.equal = function (lhs,rhs)
    {
        if ((lhs !=null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.equal)  =='function')
                return lhs.equal(rhs);
        }
        
        return (lhs == rhs);
    };

    util.notEqual = function (lhs,rhs)
    {
        if ((lhs != null)&& (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.notEqual) == 'function')
                return lhs.notEqual(rhs);
        }
        
        return (lhs != rhs);
    };

    util.identical = function (lhs,rhs)
    {
        if ((lhs !== null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.identical) == 'function')
                return lhs.identical(rhs);
        }
        
        return (lhs === rhs);
    };

    util.notIdentical = function (lhs,rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.notIdentical) == 'function')
                return lhs.notIdentical(rhs);
        }
        
        return (lhs !== rhs);
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

system.__isKilling = function()
{
    return false;
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


std.messaging.SenderMessagePair = function (sender,msgObj)
{
    print('\n\nInside of senderMessagePair\n');
    print(sender);
    print(msgObj);
    print('\n\n\n');
    return sender;
};