// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MESH_TERM_BLOOM_FILTER_QUERY_DATA_LOOKUP_HPP_
#define _SIRIKATA_OH_MESH_TERM_BLOOM_FILTER_DATA_LOOKUP_HPP_

#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

namespace Sirikata {

class TermBloomFilterQueryDataLookup : public QueryDataLookup {
public:
    TermBloomFilterQueryDataLookup(uint32 buckets, uint16 hashes);
    virtual ~TermBloomFilterQueryDataLookup() {}

    virtual void lookup(HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ci);
    virtual void lookup(HostedObjectPtr ho, const SpaceID& space, const ObjectReference& oref, const String& mesh);
private:
    // To simplify the implementation, these internal methods take both a
    // HostedObject::OHConnectInfoPtr and a SpaceID+ObjectReference+String. We
    // can tell which type of lookup and which callback to invoke based on
    // whether the OHConnectInfoPtr is valid
    void downloadTweetData(
        HostedObjectPtr ho,
        HostedObject::OHConnectInfoPtr ocip,
        const SpaceID& space, const ObjectReference& oref, const String& mesh,
        uint8 n_retry=0);

    void finishedDownload(
        HostedObjectPtr ho,
        HostedObject::OHConnectInfoPtr ocip,
        const SpaceID& space, const ObjectReference& oref, const String& mesh,
        Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request,
        Transfer::DenseDataPtr response);

    const uint32 mBuckets;
    const uint16 mHashes;
    Transfer::ResourceDownloadTaskPtr mResourceDownloadTask;
};

} // namespace Sirikata

#endif //_SIRIKATA_OH_MESH_ZERNIKE_QUERY_DATA_LOOKUP_HPP_
