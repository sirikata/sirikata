

//dataID is the internal 
function requestVolatile(dataID,controllerID)
{
    
}



function advertiseVolatile(initialValue,optionalDescription)
{

    //should we force this to be deterministic?  That way can only do it on calls to set.
    var conditionTrigger = function (varToTest)
    {
        if (varToTest > 20)
            return true;
        return false;
    };

    var eventCallback = function (currentValue)
    {
        system.print();
    }
    
    var returner = new system.eventGenerator(initialValue,optionalDescription,conditionTrigger,eventCallback);

}