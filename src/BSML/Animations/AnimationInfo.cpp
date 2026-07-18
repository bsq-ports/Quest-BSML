#include "BSML/Animations/AnimationInfo.hpp"

namespace BSML {
    AnimationInfo::~AnimationInfo() {
        while(!frames.empty()) frames.pop();
    }

    FrameInfo::FrameInfo(int width, int height, int bpp)
    :
        width(width),
        height(height),
        colors(ArrayW<uint8_t>(width * height * bpp)),
        delay(0) {}
}
