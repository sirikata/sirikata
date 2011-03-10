#include <sirikata/proxyobject/OverlayPosition.hpp>

namespace Sirikata {
OverlayPosition::OverlayPosition(const RelativePosition &relPosition, short offsetLeft, short offsetTop) {
    usingRelative = true;
    data.rel.position = relPosition;
    data.rel.x = offsetLeft;
    data.rel.y = offsetTop;
}

OverlayPosition::OverlayPosition(short absoluteLeft, short absoluteTop) {
    usingRelative = false;
    data.abs.left = absoluteLeft;
    data.abs.top = absoluteTop;
}

OverlayPosition::OverlayPosition() {
    usingRelative = false;
    data.abs.left = 0;
    data.abs.top = 0;
}
}
