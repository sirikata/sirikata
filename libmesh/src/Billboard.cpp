// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/mesh/Billboard.hpp>

namespace Sirikata {
namespace Mesh {

String Billboard::sType("billboard");

Billboard::Billboard()
 : image(),
   aspectRatio(-1.f),
   facing(FACING_CAMERA)
{
}

Billboard::Billboard(const String& img)
 : image(img),
   aspectRatio(-1.f),
   facing(FACING_CAMERA)
{
}

Billboard::~Billboard() {
}

const String& Billboard::type() const {
    return sType;
}

} // namespace Mesh
} // namespace Sirikata
