/*  Sirikata libspace -- Known Service Ports
 *  KnownServices.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef SIRIKATA_KNOWN_SERVICES_HPP_
#define SIRIKATA_KNOWN_SERVICES_HPP_
namespace Sirikata {
namespace Services{
enum Ports{
    REGISTRATION=1,
    LOC=2,
    GEOM=3, // Proximity service: Also known as PROX
    ROUTER=4,
    PERSISTENCE=5,
    PHYSICS=6,
    TIMESYNC=7,
    SUBSCRIPTION=9,
    BROADCAST=10,
    SCRIPTING=11,               //on this channel, we get messages to execute
                                //scripts.  (For instance, typing
                                //system.print("\n\n") into terminal will send
                                //a message with the system.print text to the JSObjectScript
    COMMUNICATION=12,           //allows JSObjectScripts to send messages to
                                //each other.
    LISTEN_FOR_SCRIPT_BEGIN=13, //a hosted object listens on this channel for
                                //any begin scriptable events.  if it receives
                                //any, will instantiate a JSObjectScript
                                //attached to the HostedObject.  Currently, ogre
                                //mouse events send this signal to the
                                //HostedObject.
    CREATE_ENTITY=14,           //The HostedObject listens to this port for
                                //messages to spawn a new entity
    OBJECT_CONNECTIONS=16383
};
}
}
#endif
