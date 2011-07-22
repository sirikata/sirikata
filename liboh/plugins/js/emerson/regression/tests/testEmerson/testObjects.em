// testing object literals and the array literals




//object

var x = {};


var x = {a:1}

print(x);
print("\n\n");
x = { a:1, b:2};

print(x.a);
print("\n\n");

x = {a:1, b:2 , c:3 }

print(x.c);
print("\n\n");


// arrays

var y = [];

y = [1, 2];

print(y);
print("\n\n");

y = [1, 2, 3];

print(y);
print("\n\n");


// array of arrays for bug 118

z = [ [1, 2], [3, 4], [ [1, 2], [5, 6] ] ]


print(z);
print("\n\n");
