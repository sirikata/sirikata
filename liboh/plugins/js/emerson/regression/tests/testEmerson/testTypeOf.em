 // testing the typeof operator
x = new Object();
x.a = 2;
print(x);

 a = typeof(x) ;

print(a);


function funcObj()
{
    print("\n\nGot into funcObj\n");
    this.z = 1;
    this.y = new Object();
    this.y.z = 2;
    print(this.y.z);
    
}

b = new funcObj();

print(b);
print("\n\n");
print(b.z);
print("\n\n");
print(b.y)
print("\n\n");
print(b.y.z);
print("\n\n");

z = typeof(z);

print(z);
