
print('\n\nTest throwing\n\n');

try
{
    throw 'this is my excption';
    print('should never get here');
}
catch (excep)
{
    print(excep);
    print('\n\ngot here\n\n');
}



print('\n\nTest throwing2\n\n');

try
{
    throw ('this is my excption');
    print('should never get here');
}
catch (excep)
{
    print(excep);
    print('\n\ngot here\n\n');
}



print('\n\nTest throwing3\n\n');

try
{
    var func = function()
    {
      return   'this is my excption';
    };
    throw func();
    print('should never get here');
}
catch (excep)
{
    print(excep);
    print('\n\ngot here\n\n');
}



print('\n\nTest throwing3\n\n');

try
{
    var thrower = 'this is my excption';

    throw thrower;
    print('should never get here');
}
catch (excep)
{
    print(excep);
    print('\n\ngot here\n\n');
}

