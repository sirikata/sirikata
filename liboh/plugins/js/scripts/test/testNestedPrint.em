var a = {
    b : "oo"
};

a.c = a;

system.print(a);

system.print('\n');

var a = {
    'b': { 'c': 'c', 'd':'d'},
    'e': 'e'
};

a.b.c = a;

system.print(a);