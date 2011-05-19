system.require('std/core/repeatingTimer.em');
system.require('std/core/bind.em');

//if do not already have std and std.core objects
//defined, define them.
if (typeof(std) === "undefined") /** @namespace */ std = {};
if (typeof(std.core) === "undefined") /** @namespace */ std.core = {};


std.core.QueryDistance = system.Class.extend(
   {
       init: function (distance, addfunction, removefunction, presence) {
          this.distance = distance;
          this.addFunc = addfunction;
          this.remFunc = removefunction;
          this.pres = presence;
          this.foundObjects = new Array();
          var func = std.core.bind(this.proxAddFunc, this);
          presence.onProxAdded(func);
          presence.setQueryAngle(.01);
          this.timerLength = 2;
          var pollfunc = std.core.bind(this.distancePoll, this);
          var repTimer = new std.core.RepeatingTimer(2, pollfunc);
          this.timerName = repTimer;
       },
       proxAddFunc: function(presence) {
          var visibleobject = new Object();
  	  visibleobject.presence = presence;
  	  visibleobject.distance = presence.dist(this.pres.getPosition());
  	  for(var i=0; i<this.foundObjects.length; i++) {
  	    if (this.foundObjects[i].presence.toString() == presence.toString()) return;
 	   }
  	  if (visibleobject.distance <= this.distance) {
 	     this.addFunc(presence);
 	   }
  	  this.foundObjects.push(visibleobject);
       },
       distancePoll: function() {
          for (visibleobject in this.foundObjects)
  	  {
 	   //If the object used to be > Distance but is now within it. 
    
   	   if (this.foundObjects[visibleobject].distance > this.distance && 
             this.foundObjects[visibleobject].presence.dist(this.pres.getPosition()) <= this.distance)
  	    {
  	      this.addFunc(this.foundObjects[visibleobject].presence);
    
  	    //If the object used to be <= Distance but is now outside it.
  
  	    } else if (this.foundObjects[visibleobject].distance <= this.distance &&
  	         this.foundObjects[visibleobject].presence.dist(this.pres.getPosition()) > this.distance)
  	    {
  	      this.remFunc(this.foundObjects[visibleobject].presence);  
  	    }
  	    this.foundObjects[visibleobject].distance = this.foundObjects[visibleobject].presence.dist(this.pres.getPosition());
  	  }
       },
       StopDistanceQuery: function() {
          this.timerName.suspend();
       },
       SetDistanceQueryRate: function(time) {
          this.timerName.suspend();
          var repTimer = new std.core.RepeatingTimer(time, this.distancePoll);
          this.timerName = repTimer;
       }
   }
);


