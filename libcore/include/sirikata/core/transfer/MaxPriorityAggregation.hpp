/*  Sirikata Transfer -- Content Transfer management system
 *  MaxPriorityAggregation.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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
/*  Created on: Jan 22th, 2010 */

#ifndef SIRIKATA_MaxPriorityAggregation_HPP__
#define SIRIKATA_MaxPriorityAggregation_HPP__

#include "TransferPool.hpp"

namespace Sirikata {
namespace Transfer {

/*
 * Just take the highest priority. Stupid aggregation.
 */
class MaxPriorityAggregation : public PriorityAggregationAlgorithm {

public:

    virtual Priority aggregate(
        const std::vector<Priority>& l) const {
        Priority max = (*l.begin());
        for (std::vector<Priority>::const_iterator it = ++(l.begin()); it != l.end(); it++) {
            if ((*it) > max) max = *it;
        }
        return max;
    }

    //Return an aggregated priority given the list of priorities
    virtual Priority aggregate(
        const std::map<std::string, TransferRequestPtr > & l) const {

        //don't feel like making compare func for max_element so just manual laziness
        std::map<std::string, TransferRequestPtr>::const_iterator findMax = l.begin();
        Priority max = findMax->second->getPriority();
        findMax++;
        while(findMax != l.end()) {
            if(findMax->second->getPriority() > max) {
                max = findMax->second->getPriority();
            }
            findMax++;
        }

        return max;
    }

    virtual Priority aggregate(
        const std::vector<TransferRequestPtr>& l) const {
        Priority max = (*l.begin())->getPriority();
        for(std::vector<TransferRequestPtr>::const_iterator it = ++(l.begin()); it != l.end(); it++) {
            if ((*it)->getPriority() > max) max = (*it)->getPriority();
        }
        return max;
    }
};

}
}

#endif
