//This function is supposed to check whether clearing timers work.
//What should happen is you shouldn't see the callback fire.  And an error
//should throw if you type at the prompt times.suspend() or times.resume().
//(It should say something like, error: can't call suspend/resume on an already-cleared
//timer.)

function callback()
{
    system.print('\n\nGot into callback\n\n');
}

var times = system.timeout(5,callback);

times.clear();

