var a = "a vector";
var i = 3;
function m(num)
{
    return a;
}

var b = "ooooo";
var c = new Object();

j = 32;

function test()
{
  a << b;
  
  print("\nPassed 1\n");
  
  m(i) << b;
  
  print("\nPassed 2\n");

  a << b << c;

  print("\nPassed 3\n");

  m(i) << b << c;

  print("\nPassed 4\n");

  a << b << m(i);
  
  print("\nPassed 5\n");

  m(i) << b << m(j);

  print("\nPassed 6\n");

}

test();
