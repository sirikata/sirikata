
for (var s = 0; s < 10; ++s)
    print(s+20);

print("\n\n");

var mArray = new Array();

for (var s = 10; s > 3; --s)
{
    print (s*-1);
    if (s%2 == 0)
        mArray.push(s+10);

    print("\n\n");
}

for (var s in mArray)
    print(mArray[s].toString());
 


print("\n\n");