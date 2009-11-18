/*  cbr
 *  ObjectHostContext.cpp
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

#include "ObjectHostContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>

namespace CBR {

ObjectHostContext::ObjectHostContext(ObjectHostID _id, IOService* ios, IOStrand* strand, Trace* _trace, const Time& epoch, const Duration& simlen)
 : Context("Object Host", ios, strand, _trace, epoch, simlen),
   id(_id),
   objectHost(NULL),
   mFinishedTimer( IOTimer::create(ios) )
{
}

void ObjectHostContext::start() {
    Time t_now = simTime();
    Time t_end = simTime(mSimDuration);
    Duration wait_dur = t_end - t_now;
    mFinishedTimer->wait(
        wait_dur,
        mainStrand->wrap(
            std::tr1::bind(&ObjectHostContext::stopSimulation, this)
        )
    );
}

void ObjectHostContext::stopSimulation() {
    this->stop();
    for(std::vector<Service*>::iterator it = mServices.begin(); it != mServices.end(); it++)
        (*it)->stop();
}

void ObjectHostContext::stop() {
    mFinishedTimer.reset();
}


} // namespace CBR
