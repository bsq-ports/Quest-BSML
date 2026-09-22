#pragma once

#include "GIF/IndexedFrame.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/Material.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"
#include <memory>

namespace BSML::detail {
    // Main-thread owned. The controller owns the displayed texture; this owner
    // holds the compressed frames and the offscreen expansion resources.
    class IndexedAnimation {
    public:
        static bool Supported();
        static std::unique_ptr<IndexedAnimation> Create(const std::vector<IndexedFrame>& frames,
            IndexedAtlasLayout layout, UnityEngine::Texture2D* output);
        ~IndexedAnimation();
        bool Expand(int frame);

    private:
        safe_ptr<UnityEngine::Texture2D*, true> indices, palettes, output;
        safe_ptr<UnityEngine::RenderTexture*, true> target;
        safe_ptr<UnityEngine::Material*, true> material;
        IndexedAtlasLayout layout;
        int width = 0, height = 0, frameCount = 0, expandedFrame = -1;
    };
}
