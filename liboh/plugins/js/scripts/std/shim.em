/*
  THis is the main shim layer created between Emerson and C++. This is because we want some basic functionality to be in C++ while creating a layer of other essential functionalities directly in Emerson. We save dev time with this and we are not too much bothered about performance. IF performance degrades beyond tolerance, we will implement those functionalities in C++.

*/


system.shim = new Object();

/* Add functions to the prototype for presence */


system.__presence_constructor__.prototype.runSimulation =
function(name)
{
  system.print("\n\nIn side the runSimulation\n\n");
  var pres = this;
  if(!system.shim.graphics)
  {
	  system.print("\n\n Got the new graphics object\n\n");
    system.shim.graphics = new Object();  
  }
  if(!system.shim.graphics[pres])
  {
    system.shim.graphics[pres] = new Object();
    system.print("\n\nBefore _runSimulation\n\n");
    system.shim.graphics[pres][name] = pres._runSimulation(name);
  }
    
	system.print("\n\n YEYEYEY 1\n\n");
  var return_val = system.shim.graphics[pres][name];
	system.print("\n\n YEYEYEY 2\n\n");

	return return_val; 
}

system.__presence_constructor__.prototype.getSimulation = 
function(name)
{
 
  if(!system.shim.graphics || !system.shim.graphics[this]) return undefined;
  return system.shim.graphics[this][name];        
}



