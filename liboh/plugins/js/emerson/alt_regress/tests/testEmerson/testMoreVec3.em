(<3,1,2> + <2,4,6> + <1,9,1> - <4,2,6>).print();

function getVal()
{
    return 1;
}

function getOtherVal()
{
    return {'val' : 10};
}


x = < 1>2? 2 : 1, 3, 5> 1? 2:6>;
y =  < getVal(), getOtherVal().val, getOtherVal()['val']>;

print('\n\n');
x.print();

print('\n\n');
y.print();


//there be dragons here.  technically valid to interpret it the way you wouldn't think to.
////x = < 1>2? 2 : 1, 3, 5> 1? 2:6> +  < getVal(), getOtherVal().val, getOtherVal()['val']>;
x = < 1,2,3 >+  < getVal(), getOtherVal().val, getOtherVal()['val']>;
print('\n\n');
x.print();



z = <1,2,3> - <4,5,6>;

print('\n\n');
x.print();


y = < getVal(), getOtherVal().val, getOtherVal()['val']>;

print('\n\n');

y.print();
    

y = < getVal(), getOtherVal().val, getOtherVal()['val'] > 1? 2:1>;

print('\n\n');

y.print();

print('\n\n');