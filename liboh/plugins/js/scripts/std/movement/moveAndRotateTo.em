/*  Sirikata
 *  moveAndRotateTo.em
 *
 *  Copyright (c) 2011, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

system.require('moveAndRotate.em');
/** MoveAndRotate gives you intuitive control over moving to a position and
 *  attaining a final rotation. The most common use for this is when you want to 
 *  send an object to a particular location with a particular facing when it gets there.
 *  A callback is presented when the object arrives at the destination so further movement 
 *  can be queued or an action can take place
 */
std.movement.MoveAndRotateTo = std.movement.MoveAndRotate.extend(
    {
        init:function(pres,update_cb, destination, destination_facing, speed, callback){
            this._speed=speed;
            std.movement.MoveAndRotate.prototype.init.call(this,pres,update_cb,'rotation');
            if (destination) {
                this.goTo(destination,destination_facing,speed, callback);
            }
        },
        goTo:function(destination,destination_facing, speed, callback) {
           if (speed) {
               this._speed=speed;
           }
           var pos =this._pres.getPosition();
           var forward=destination.sub(pos).normal();
           destination_facing=destination_facing||this._pres.getOrientation();
           var goal_orientation=util.Quaternion.fromLookAt(forward,destination_facing.yAxis());
           var thus=this;
           var destination_delta=destination.sub(pos);
           var destination_length=destination_delta.length();
           var turning_distance=.25;
           if(destination_length>2.5*turning_distance) {
               destination_delta=destination_delta.scale(.125);
           }else {
               destination_delta.scale(turning_distance/destination_length);
           }
           this.goToWaypoint(destination_delta.add(pos),goal_orientation,true, function(){
                 thus.goToWaypoint(destination.add(destination_delta.scale(-1.0)),goal_orientation,false,function (){
                   thus.goToWaypoint(destination,(destination_facing||this._pres.orientation()),true,function () {
                                         thus._pres.setVelocity(util.Vec3(0,0,0));
                                         thus._pres.setOrientationVel(util.Quaternion(0,0,0,0));
                                         callback();
                                     });
               });
           });
        },
        goToWaypoint:function(destination,destination_facing,doTurning,callback) {
            this._callback=callback;
            this._destination=destination;
            this._destination_facing=destination_facing;
            var pres=this._pres;
            var pos=pres.getPosition();
            var deltapos=this._destination.sub(pos);
            var distance=deltapos.length();
            var time = distance/this._speed;
            var inverseOrientation=pres.getOrientation().inverse();
            var rotation=inverseOrientation.mul(this._destination_facing);
            pres.setOrientationVel(rotation.scale(1.0/time));
            //this.move(deltapos.scale(this._speed/distance));
            if (doTurning) {
                this.move(<0,0,-1>,this._speed,true);
            }else {
                this.move(inverseOrientation.mul(deltapos),this._speed/distance,true);
            }
            //system.print("At "+pos.x+" "+pos.y+" "+pos.z+" going to "+destination.x+" "+destination.y+" "+destination.z+"\n");
            system.timeout(time,function() {
                           pres.setOrientation(destination_facing);
                           if (!doTurning)
                               pres.setPosition(destination);                           
                           callback();
                       });
        }
    }
);