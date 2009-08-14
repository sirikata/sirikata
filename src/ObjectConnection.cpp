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

void ObjectConnection::deliver(const CBR::Protocol::Object::ObjectMessage& msg, const Time& t) {
    assert( msg.dest_object() == mObject->uuid() );

    switch( msg.dest_port() ) {
      case OBJECT_PORT_PROXIMITY:
          {
              assert(msg.source_object() == UUID::null()); // Should originate at space server
              CBR::Protocol::Prox::ProximityResults contents;
              bool parse_success = contents.ParseFromString(msg.payload());
              assert(parse_success);

              for(uint32 idx = 0; idx < contents.addition_size(); idx++) {
                  CBR::Protocol::Prox::IObjectAddition addition = contents.addition(idx);
                  TimedMotionVector3f loc(addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()));
                  mTrace->prox(
                      t,
                      msg.dest_object(),
                      addition.object(),
                      true,
                      loc
                  );
              }

              for(uint32 idx = 0; idx < contents.removal_size(); idx++) {
                  CBR::Protocol::Prox::IObjectRemoval removal = contents.removal(idx);
                  mTrace->prox(
                      t,
                      msg.dest_object(),
                      removal.object(),
                      false,
                      TimedMotionVector3f()
                  );
              }

              mObject->proximityMessage(msg, contents);
          }
          break;
      case OBJECT_PORT_LOCATION:
          {
              CBR::Protocol::Loc::TimedMotionVector contents;
              bool parse_success = contents.ParseFromString(msg.payload());
              assert(parse_success);

              TimedMotionVector3f loc(contents.t(), MotionVector3f(contents.position(), contents.velocity()));

              mTrace->loc(
                  t,
                  msg.dest_object(),
                  msg.source_object(),
                  loc
              );
              mObject->locationMessage(msg.source_object(), loc);
          }
          break;
      case OBJECT_PORT_SUBSCRIPTION:
          {
              CBR::Protocol::Subscription::SubscriptionMessage contents;
              bool parse_success = contents.ParseFromString(msg.payload());
              assert(parse_success);

              mTrace->subscription(
                  t,
                  msg.dest_object(),
                  msg.source_object(),
                  (contents.action() == CBR::Protocol::Subscription::SubscriptionMessage::Subscribe) ? true : false
              );

              mObject->subscriptionMessage(
                  msg.source_object(),
                  (contents.action() == CBR::Protocol::Subscription::SubscriptionMessage::Subscribe) ? true : false
              );
          }
          break;
      default:
        printf("Warning: Tried to deliver uknown message type through ObjectConnection.\n");
        break;
    }
}

} // namespace CBR
