try {
    x();
}
catch (ex) {
    print('caught\n');
}



try
{
   print('\n\nsecond try\n\n');
}
finally
{
   print('\n\nsecond finally\n\n');
}


try
{
    print('\n\nthird try\n\n');
    x();
}
catch(ex)
{
    print('\n\nThird catch\n');
}
finally
{
   print('\n\nthird finally\n\n');
}


