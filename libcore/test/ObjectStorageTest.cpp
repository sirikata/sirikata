/*  Sirikata -- Persistence Services
 *  ObjectStorageTest.cpp
 *
 *  Copyright (c) 2008, Stanford University
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
#include <util/Platform.hpp>
#include "ObjectStorageTest.hpp"
using namespace Sirikata;
using namespace Sirikata::Persistence;
/* Utilities for ObjectStorage tests.  No tests are defined here or in
 *  ObjectStorageTest.hpp.
 */

static std::vector<StorageKey> sGeneratedKeys;
static std::vector<StorageValue> sGeneratedValues;

static bool sGeneratedPairs = false;

static void generate_pairs() {
    for(int i = 0; i < OBJECT_STORAGE_GENERATED_PAIRS; i++) {
        String field(rand() % 15 + 1, 'a');
        for(std::size_t j = 0; j < field.size(); j++)
            field[j] = 'a' + (rand() % 25);
        sGeneratedKeys.push_back( StorageKey(UUID::random(), rand() % 1000, field) );
    }

    for(int i = 0; i < OBJECT_STORAGE_GENERATED_PAIRS; i++) {
        int len = rand() % 1000;
        StorageValue value;
        for(int j = 0; j < len; j++)
            value.push_back((char)(rand() % 255));
        sGeneratedValues.push_back(value);
    }
    sGeneratedPairs = true;
}

std::vector<StorageKey>& keys() {
    if (!sGeneratedPairs)
        generate_pairs();
    return sGeneratedKeys;
}

std::vector<StorageValue>& values() {
    if (!sGeneratedPairs)
        generate_pairs();
    return sGeneratedValues;
}
