system.require('std/core/repeatingTimer.em');
system.require('std/core/bind.em');

//if do not already have std and std.core objects
//defined, define them.
if (typeof(std) === "undefined") /** @namespace */ std = {};
if (typeof(std.core) === "undefined") /** @namespace */ std.core = {};


std.core.QueryDistance = system.Class.extend({
    init: function (distance, addFunction, removeFunction, presence) {
        this.distance = distance;
        this.addFunc = addFunction;
        this.removeFunc = removeFunction;
        this.pres = presence;
        this.foundObjects = new Array();
        
        var func = std.core.bind(this.proxAddFunc, this);
        presence.onProxAdded(func);
        presence.setQueryAngle(.01);
        
        var pollFunc = std.core.bind(this.distancePoll, this);
        var repTimer = new std.core.RepeatingTimer(2, pollFunc);
        this.timerName = repTimer;
    },
    proxAddFunc: function(presence) {
        var visible = new Object();
        visible.presence = presence;
        visible.distance = presence.dist(this.pres.getPosition());
        for (var i = 0; i < this.foundObjects.length; i++) {
            if (this.foundObjects[i].presence.toString() == presence.toString())
                return;
        }
        if (visible.distance <= this.distance) {
            this.addFunc(presence);
        }
        this.foundObjects.push(visible);
    },
    distancePoll: function() {
        for (var i in this.foundObjects) {
            var visible = this.foundObjects[i];
            var currentDistance = visible.presence.dist(this.pres.getPosition());
            if (this.addFunc && visible.distance > this.distance && 
                currentDistance <= this.distance) {
                //If the object used to be > Distance but is now within it.
                this.addFunc(this.visible.presence);
            }

            if (this.removeFunc && this.visible.distance <= this.distance &&
                visible.presence.dist(this.pres.getPosition()) > this.distance) {
                //If the object used to be <= Distance but is now outside it.
                this.removeFunc(visible.presence);  
            }
            visible.distance = visible.presence.dist(this.pres.getPosition());
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
});


