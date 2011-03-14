function printer(a,b)
{
    if ( a )
    {
        print("\n\n");
        print("\nExecuting IF\n");
        print("\n\n");
    }
    else if( b )
    {
        print("\n\n");
        print("\nExecuting IF ELSE\n");
        print("\n\n");
    }
    else
    {
        print("\n\n");
        print("\nExecuting ELSE\n");
        print("\n\n");
    }
}

printer(true,true);
printer(true,false);
printer(false,false);
printer(false,true);

