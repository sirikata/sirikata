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
        //indexed by toString of visibles.
        this.foundObjects = {};

        this.isStopped = false;
        
        var func = std.core.bind(this.proxAddFunc, this);
        var removeFunc = std.core.bind(this.proxRemoveFunc,this);
        presence.onProxAdded(func,true);
        presence.onProxRemoved(removeFunc);
        presence.setQueryAngle(.01);
        
        var pollFunc = std.core.bind(this.distancePoll, this);
        var repTimer = new std.core.RepeatingTimer(2, pollFunc);
        this.timerName = repTimer;
    },
    proxAddFunc: function(presence) {
        var visible = new Object();
        visible.presence = presence;
        visible.distance = presence.dist(this.pres.getPosition());
        if (presence.toString() in this.foundObjects)
            return;

        if ((visible.distance <= this.distance) && (!this.isStopped)) {
            this.addFunc(presence);
        }
        this.foundObjects[presence.toString()] = visible;
    },


    proxRemoveFunc: function(presence) {
        if (presence.toString() in this.foundObjects)
        {
            var visible = this.foundObjects[presence.toString()];
            if (this.removeFunc && (visible.distance <= this.distance) && (! this.isStopped))
            {
                //means that presence previously had been within query and
                //that we need to actuall call removeFunc.
                this.removeFunc(presence);                    
            }

            //actually remove visible from set that we were tracking.
            delete this.foundObjects[presence.toString()];
        }
    },

                                                 
                                                 
    distancePoll: function() {
        for (var i in this.foundObjects)
        {
            var visible = this.foundObjects[i];
            var currentDistance = visible.presence.dist(this.pres.getPosition());
            if (this.addFunc && visible.distance > this.distance && 
                currentDistance <= this.distance  && (! this.isStopped)) {
                //If the object used to be > Distance but is now within it.
                
                this.addFunc(visible.presence);
            }

            if (this.removeFunc && visible.distance <= this.distance &&
                visible.presence.dist(this.pres.getPosition()) > this.distance &&
               (!this.isStopped))
            {
                //If the object used to be <= Distance but is now outside it.
                this.removeFunc(visible.presence);  
            }
            visible.distance = visible.presence.dist(this.pres.getPosition());
        }
   },
   StopDistanceQuery: function() {
       this.isStopped = true;
       this.timerName.suspend();
   },
   SetDistanceQueryRate: function(time) {
       this.timerName.suspend();
       var repTimer = new std.core.RepeatingTimer(time, this.distancePoll);
       this.timerName = repTimer;
       this.isStopped = false;
   }
});


