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

system.require('moveAndRotateTo.em');

/** 
 * Pursue has one object following another
 */
std.movement.Pursue = std.movement.MoveAndRotateTo.extend(
    {
        init:function(pres,update_cb, destination_visual, speed, callback, callback_distance){
            this._time_quantum=.5;
            this._speed=speed;
            this._pursue_callback=callback;
            this._callback_distance=callback_distance;
            std.movement.MoveAndRotateTo.prototype.init.call(this,pres,undefined,undefined,speed,update_cb,'rotation');
            if (destination_visual) {
                this.follow(destination_visual,speed, callback);
            }
        },
        follow:function(destination_visual, speed, callback, callback_distance) {
           if (speed) {
               this._speed=speed;
           }
           this._destination_visual=destination_visual;
           this._pursue_callback=callback;
           this._callback_distance=callback_distance;
           this.follow_step();
        },
        follow_step:function(){
           if (!this._pres) {
               system.print("Defunct this\n");
               return;
           }else if (!this._destination_visual) {
               system.print("Defunct visual\n");
               return;
           }
           var dest=this._destination_visual.getPosition();
           var pos =this._pres.getPosition();
           var forward=dest.sub(pos);
           var forward_length=forward.length();
           var abort_follow=false;
           system.print("Going from "+pos+" to "+dest+"\nIs "+forward_length+" < "+this._callback_distance);          
           if (forward_length<this._callback_distance) {
               system.print("YES\n");          
               if (this._pursue_callback) {
                   abort_follow=this._pursue_callback.call(this);
                   system.print(" IT IS \n"+abort_follow);          
               }else abort_follow=true;
           }
           if (!abort_follow){
               forward=forward.scale(1./forward_length);
               var goal_orientation=util.Quaternion.fromLookAt(forward,this._pres.getOrientation().yAxis());
               var thus=this;
               this.goToWaypoint(pos.add(forward.scale(this._time_quantum/this._speed)),goal_orientation,true,std.movement.Pursue.prototype.follow_step);
           }
        }
});
