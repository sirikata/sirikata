// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "Defs.hpp"

namespace Sirikata {

String ToString(bulletObjTreatment treatment) {
    switch(treatment) {
      case BULLET_OBJECT_TREATMENT_IGNORE: return "ignore"; break;
      case BULLET_OBJECT_TREATMENT_STATIC: return "static"; break;
      case BULLET_OBJECT_TREATMENT_DYNAMIC: return "dynamic"; break;
      case BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC: return "linear_dynamic"; break;
      case BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC: return "vertical_dynamic"; break;
      case BULLET_OBJECT_TREATMENT_CHARACTER: return "character"; break;
      default: return ""; break;
    }
}

String ToString(bulletObjBBox bnds) {
    switch(bnds) {
      case BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT: return "box"; break;
      case BULLET_OBJECT_BOUNDS_PER_TRIANGLE: return "triangles"; break;
      case BULLET_OBJECT_BOUNDS_SPHERE: return "sphere"; break;
      default: return ""; break;
    }
}

} // namespace Sirikata
