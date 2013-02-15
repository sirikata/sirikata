// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MESH_ZERNIKE_QUERY_DATA_LOOKUP_HPP_
#define _SIRIKATA_OH_MESH_ZERNIKE_QUERY_DATA_LOOKUP_HPP_

#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

namespace Sirikata {

class ZernikeQueryDataLookup : public QueryDataLookup {
public:
    virtual ~ZernikeQueryDataLookup() {}

    virtual void lookup(HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ci);
    virtual void lookup(HostedObjectPtr ho, const SpaceID& space, const ObjectReference& oref, const String& mesh);
private:
    // To simplify the implementation, these internal methods take both a
    // HostedObject::OHConnectInfoPtr and a SpaceID+ObjectReference+String. We
    // can tell which type of lookup and which callback to invoke based on
    // whether the OHConnectInfoPtr is valid
    void downloadZernikeDescriptor(
        HostedObjectPtr ho,
        HostedObject::OHConnectInfoPtr ocip,
        const SpaceID& space, const ObjectReference& oref, const String& mesh,
        uint8 n_retry=0);
    void metadataDownloaded(
        HostedObjectPtr ho,
        HostedObject::OHConnectInfoPtr ocip,
        const SpaceID& space, const ObjectReference& oref, const String& mesh,
        uint8 retryCount,
        Transfer::MetadataRequestPtr request,
        Transfer::RemoteFileMetadataPtr response);

};

} // namespace Sirikata

#endif //_SIRIKATA_OH_MESH_ZERNIKE_QUERY_DATA_LOOKUP_HPP_
