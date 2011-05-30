

function switcher(arg)
{
    switch (arg)
    {
    case 'a':
    case 'b':
        print('\nPrinting a, b\n');
        break;
    case 'c':
        print('\nPrinting c\n');
        break;
    case 9:
        print('\nPrinting 9\n');
        break;
    default:
        print('\nPrinting default\n');        
    }
}



switcher('a');
switcher('b');
switcher('d');
switcher('c');
switcher('9');
switcher('m');
switcher(9);

