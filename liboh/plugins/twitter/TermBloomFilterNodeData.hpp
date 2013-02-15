// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_TWITTER_TERM_BLOOM_FILTER_NODE_DATA_HPP_
#define _SIRIKATA_OH_TWITTER_TERM_BLOOM_FILTER_NODE_DATA_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/twitter/TermBloomFilterNodeData.hpp>

#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"

namespace Sirikata {

// Same as other versions *except* for where it gets options for bloom parameters
/** Maintains bounding sphere, maximum sized object, and bloom filter for search
 *  terms.
 */
template<typename SimulationTraits>
class TermBloomFilterNodeData : public TermBloomFilterNodeDataBase<SimulationTraits, TermBloomFilterNodeData<SimulationTraits> > {
public:
    typedef TermBloomFilterNodeData NodeData; // For convenience/consistency
    typedef TermBloomFilterNodeDataBase<SimulationTraits, TermBloomFilterNodeData<SimulationTraits> > ThisBase;

    typedef typename SimulationTraits::TimeType Time;
    typedef Prox::LocationServiceCache<SimulationTraits> LocationServiceCacheType;
    typedef typename LocationServiceCacheType::Iterator LocCacheIterator;

    TermBloomFilterNodeData()
     : ThisBase(
         GetOptionValue<uint32>(OPT_TWITTER_BLOOM_BUCKETS),
         GetOptionValue<uint16>(OPT_TWITTER_BLOOM_HASHES)
     )
    {
    }

    TermBloomFilterNodeData(LocationServiceCacheType* loc, const LocCacheIterator& obj_id, const Time& t)
     : ThisBase(
         GetOptionValue<uint32>(OPT_TWITTER_BLOOM_BUCKETS),
         GetOptionValue<uint16>(OPT_TWITTER_BLOOM_HASHES),
         loc, obj_id, t
     )
    {
    }
};


} // namespace Sirikata

#endif //_SIRIKATA_OH_TWITTER_TERM_BLOOM_FILTER_NODE_DATA_HPP_
