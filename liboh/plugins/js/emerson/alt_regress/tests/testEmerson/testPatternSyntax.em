
function handler(msg, sender)
{
    print('\n\nGot into handler\n\n');
}


handler << { 'a': : };
handler << {::};
handler << [{::},{'a':'b':'c'}];
handler << { 'm':  : 'wo'};


