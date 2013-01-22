// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SPACE_TWITTER_OPTIONS_HPP_
#define _SIRIKATA_SPACE_TWITTER_OPTIONS_HPP_

#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>

// TWAGGMGR instead of AGGMGR avoids naming conflicts with regular mesh
// aggregate manager
#define OPT_TWAGGMGR_LOCAL_PATH        "twaggmgr.local-path"
#define OPT_TWAGGMGR_LOCAL_URL_PREFIX  "twaggmgr.local-url-prefix"
#define OPT_TWAGGMGR_GEN_THREADS       "twaggmgr.gen-threads"
#define OPT_TWAGGMGR_UPLOAD_THREADS    "twaggmgr.upload-threads"


// Parameters for twitter bloom filters
#define OPT_TWITTER_BLOOM_BUCKETS      "twitter.bloom.buckets"
#define OPT_TWITTER_BLOOM_HASHES       "twitter.bloom.hashes"

#endif //_SIRIKATA_SPACE_TWITTER_OPTIONS_HPP_
