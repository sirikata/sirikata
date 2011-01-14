#ifndef _MESHDATA_TO_COLLADA_H
#define _MESHDATA_TO_COLLADA_H

#include "ColladaDocument.hpp"

#include <sirikata/mesh/Meshdata.hpp>

namespace Sirikata {
int meshdataToCollada(const Mesh::Meshdata& meshdata, const std::string& fileName);

}


#endif
