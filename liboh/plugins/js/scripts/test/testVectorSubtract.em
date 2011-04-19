
//This script is going to test to ensure that vector subtraction works with new overloaded + operator
//Also checks number subtractions still work correctly.  string subtractions should return an error.


var x = <3,1,5>;
var y = <1,3,-1>;

system.print("\n\n");
system.print("should get: <2,-2,6>:");
system.print(x -y);
system.print("\n\n");
system.print("Should get -1: ");
system.print(6 - 7);
system.print("\n\n");
system.print("should get error: ");
system.print('abc' - 'def');
system.print("\n\n");


