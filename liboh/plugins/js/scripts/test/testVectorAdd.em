
//This script is going to test to ensure that vector addition works with new overloaded + operator
//Also checks string addition and number addition to be sure.


var x = <3,1,5>;
var y = <1,3,-1>;

system.print("\n\n");
system.print("should get: <4,4,4>:");
var z =x + y;
system.print(z);
system.print("\n\n");
system.print("Should get 13: ");
system.print(6 + 7);
system.print("\n\n");
system.print("should get 'abcdef': ");
system.print('abc' + 'def');
system.print("\n\n");


