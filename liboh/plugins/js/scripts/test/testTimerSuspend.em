//if timer suspension works correctly, then you shouldn't see the callback being printed.
//After a while, at the prompt, type times.reset.  10 seconds after you do this, you should
//see the callback message.  if you type times.reset after seeing the callback, you should
//see the callback message another 10 seconds later.  See what happens if type times.reset
//multiple times.

function callback()
{
    system.print('\n\nGot into callback\n\n');
}

var times = system.timeout(10,null,callback);

times.suspend();