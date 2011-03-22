//tests timeout

system.print("\n\nTESTING TIMEOUT.  SHOULD PRINT \"TIMEDOUT\" in 10 seconds\n\n");


function timerCallback()
{
    system.print("\n\nTIMEDOUT\n\n");
}



system.timeout(10,timerCallback);
