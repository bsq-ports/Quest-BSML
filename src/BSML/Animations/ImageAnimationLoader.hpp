#pragma once

#include "BSML/Animations/AnimationLoader.hpp"
#include "IndexedAnimation.hpp"

namespace BSML::detail {
    using ImageAnimationCallback = std::function<void(UnityEngine::Texture2D*,
        ArrayW<UnityEngine::Rect>, ArrayW<float>, std::unique_ptr<IndexedAnimation>)>;

    // The public atlas API retains its RGBA atlas contract. SetImage can also
    // consume a shared, dynamically expanded texture through this internal API.
    void ProcessImageAnimation(AnimationLoader::AnimationType type, ArrayW<uint8_t> data,
        ImageAnimationCallback onProcessed, std::function<void()> onError);
}
