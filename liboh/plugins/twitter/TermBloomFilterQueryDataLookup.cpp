// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "TermBloomFilterQueryDataLookup.hpp"
#include <json_spirit/json_spirit.h>
#include <sirikata/twitter/TermBloomFilter.hpp>

namespace Sirikata {

TermBloomFilterQueryDataLookup::TermBloomFilterQueryDataLookup(uint32 buckets, uint16 hashes)
 : mBuckets(buckets),
   mHashes(hashes)
{

}

void TermBloomFilterQueryDataLookup::lookup(HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ci) {
    if (!ci->mesh.empty())
        downloadTweetData(ho, ci);
    else
        ho->objectHostConnectIndirect(ho, this, ci);
}

void TermBloomFilterQueryDataLookup::downloadTweetData(HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ocip, uint8 n_retry)
{
    mResourceDownloadTask = Transfer::ResourceDownloadTaskPtr(
        Transfer::ResourceDownloadTask::construct(
            Transfer::URI(ocip->mesh), ho->getObjectHost()->getTransferPool(), 1.0,
            std::tr1::bind(&TermBloomFilterQueryDataLookup::finishedDownload, this, ho, ocip, _1, _2, _3)
        )
    );
    mResourceDownloadTask->start();
}

void TermBloomFilterQueryDataLookup::finishedDownload(
    HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ocip,
    Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request,
    Transfer::DenseDataPtr response)
{
    if (!response) {
        // Failed to download. Leaving it empty will just use an empty filter
        SILOG(term-bloom-filter, error, "Failed to parse tweet data from " << ocip->mesh << ". Leaving extra query data empty.");
        ho->context()->mainStrand->post(std::tr1::bind(&HostedObject::objectHostConnectIndirect, ho.get(), ho, this, ocip));
        return;
    }

    // Parse the tweets
    String tweet_json = response->asString();
    json_spirit::Value tweet_data;
    if (!json_spirit::read(tweet_json, tweet_data)) {
        // Parsing failed, give up and leave this entry empty
        SILOG(term-bloom-filter, error, "Failed to parse tweet data from " << ocip->mesh << ". Leaving extra query data empty.");
        ho->context()->mainStrand->post(std::tr1::bind(&HostedObject::objectHostConnectIndirect, ho.get(), ho, this, ocip));
        return;
    }

    // Scan through, insert into bloom filter
    Twitter::TermBloomFilter bf(mBuckets, mHashes);
    json_spirit::Array& tweet_list = tweet_data.getArray("tweets");
    for(size_t tidx = 0; tidx < tweet_list.size(); tidx++) {
        json_spirit::Array& term_list = tweet_list[tidx].getArray("terms");
        for(size_t term_idx = 0; term_idx < term_list.size(); term_idx++) {
            if (term_list[term_idx].isString()) {
                bf.insert(term_list[term_idx].getString());
            }
            else if (term_list[term_idx].isArray()) { // bigrams/trigrams
                json_spirit::Array& subterm_list = term_list[term_idx].getArray();
                String full_term;
                for(size_t subterm_idx = 0; subterm_idx < subterm_list.size(); subterm_idx++) {
                    if (subterm_idx > 0) full_term += " ";
                    full_term += subterm_list[subterm_idx].getString();
                }
                bf.insert(full_term);
            }
        }
    }

    // "Serialize" by using raw data in string
    bf.serialize(ocip->query_data);

    // And let the OH know it can connect the object now
    ho->context()->mainStrand->post(std::tr1::bind(&HostedObject::objectHostConnectIndirect, ho.get(), ho, this, ocip));
}

} // namespace Sirikata
