// Test whether a call chained immediately after a constructor call
// works properly. Note that this may work with a literal, e.g.
//   var trimmed = '  abc  '.replace(' ', '*', 'g');
//   print(trimmed);
// but not work with an explicit constructor call.

var trimmed = new String('  abc  ').replace(' ', '*', 'g');
print(trimmed);