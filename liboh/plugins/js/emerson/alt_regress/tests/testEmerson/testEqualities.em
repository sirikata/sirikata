// Testing the equality operations

function testFunc(a, b)
{
    if( a == b)
    {
        print("\n\n");
        print("a == b");
        print("\n\n");
    }
    else if(a === b)
    {
        print("\n\n");
        print("a === b");
        print("\n\n");
    }
    else if(a != b)
    {
        print("\n\n");
        print("a != b");
        print("\n\n");
    }
    else if(a !== b)
    {
        print("\n\n");
        print(" a !== b");
        print("\n\n");
    }
}


testFunc(1,3);
testFunc(3,3);
testFunc(66,1);
var mObj = new Object();
mObj.s = 1;
testFunc(mObj,mObj);

