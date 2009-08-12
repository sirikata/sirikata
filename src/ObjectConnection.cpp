/*  cbr
 *  ObjectConnection.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "ObjectConnection.hpp"
#include "Object.hpp"
#include "Message.hpp"
#include "Statistics.hpp"

namespace CBR {

ObjectConnection::ObjectConnection(Object* obj, Trace* trace)
 : mObject(obj),
   mTrace(trace)
{
}

void ObjectConnection::deliver(Message* msg, const Time& t) {
    switch(msg->type()) {
      case MESSAGE_TYPE_PROXIMITY:
          {
              ProximityMessage* prox_msg = dynamic_cast<ProximityMessage*>(msg);
              assert(prox_msg != NULL);

              for(uint32 idx = 0; idx < prox_msg->contents.addition_size(); idx++) {
                  CBR::Protocol::Prox::IObjectAddition addition = prox_msg->contents.addition(idx);
                  Time loc_t( (addition.location().t()-PBJ::Time::null()).toMicroseconds() );
                  MotionVector3f loc_motion(addition.location().position(), addition.location().velocity());
                  TimedMotionVector3f loc(loc_t, loc_motion);
                  mTrace->prox(
                      t,
                      prox_msg->object_header.dest_object(),
                      addition.object(),
                      true,
                      loc
                  );
              }

              for(uint32 idx = 0; idx < prox_msg->contents.removal_size(); idx++) {
                  CBR::Protocol::Prox::IObjectRemoval removal = prox_msg->contents.removal(idx);
                  mTrace->prox(
                      t,
                      prox_msg->object_header.dest_object(),
                      removal.object(),
                      false,
                      TimedMotionVector3f()
                  );
              }

              mObject->proximityMessage(prox_msg);
          }
          break;
      case MESSAGE_TYPE_LOCATION:
          {
              LocationMessage* loc_msg = dynamic_cast<LocationMessage*>(msg);
              assert(loc_msg != NULL);
              mTrace->loc(t, loc_msg->destObject(), loc_msg->sourceObject(), loc_msg->location());
              mObject->locationMessage(loc_msg);
          }
          break;
      case MESSAGE_TYPE_SUBSCRIPTION:
          {
              SubscriptionMessage* subs_msg = dynamic_cast<SubscriptionMessage*>(msg);
              assert(subs_msg != NULL);
              mTrace->subscription(
                  t,
                  subs_msg->object_header.dest_object(),
                  subs_msg->object_header.source_object(),
                  (subs_msg->contents.action() == CBR::Protocol::Subscription::SubscriptionMessage::Subscribe) ? true : false
              );
              mObject->subscriptionMessage(subs_msg);
          }
          break;
      default:
        printf("Warning: Tried to deliver uknown message type through ObjectConnection.\n");
        break;
    }
}

} // namespace CBR
