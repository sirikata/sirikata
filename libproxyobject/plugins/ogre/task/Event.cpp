/*  Sirikata Kernel -- Task scheduling system
 *  Event.cpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include "Event.hpp"

namespace Sirikata {
namespace Task {

/// Stores the map from primary ID to integer--items are never deleted.
typedef std::map<std::string, int> IDMapType;

typedef std::vector<std::string> IDNameList;

static IDMapType idMap;
static IDNameList idNames;
static int max_id = 0;

/* Simple reader-writer lock, meant to have practically no
 * overhead for reads, since writes are relatively rare.
 */
static const int WRITER = 0x100000;
static AtomicValue<int> readers(0);

int IdPair::Primary::getUniqueId(const std::string &id) {
        int found = -1;
        {
                while (++readers < 0) {
                        --readers;
                        while (readers < 0) {
                        }
                }
                IDMapType::iterator iter = idMap.find(id);
                if (iter != idMap.end()) {
                        found = (*iter).second;
                }
                --readers;
        }
 	if (found == -1) {
 	        readers -= WRITER;
 	        while (readers > -WRITER) {
 	        }
 	        // Can now safely add this to the map.
 	        found = max_id;
 	        assert(found == (int)idNames.size());
		idMap.insert(IDMapType::value_type(id, found));
		idNames.push_back(id);
		max_id++;

		readers += WRITER; // done.
	}
	return found;
}

IdPair::Primary::Primary (const std::string &id)
	: mId(getUniqueId(id)) {
}
IdPair::Primary::Primary (const char *id)
	: mId(getUniqueId(id)) {
}
std::string IdPair::Primary::toString() const {
        while (++readers < 0) {
                --readers;
                while (readers < 0) {
                }
        }
        std::string ret = idNames[mId];
        --readers;
        return ret;
}
void Event::operator() (EventHistory){
    //FIXME should this delete the event?
}

}
}
