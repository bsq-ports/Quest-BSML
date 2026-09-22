#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace BSML::detail {
    struct IndexedFrame {
        std::vector<uint8_t> indices;
        std::array<uint8_t, 256 * 4> palette{};

        // Index the composited RGBA image, not the GIF's current local palette.
        // Alpha is part of the key: opaque black and transparent black differ.
        bool Encode(std::span<const uint8_t> rgba) {
            indices.clear();
            palette.fill(0);
            if (rgba.empty() || rgba.size() % 4) return false;
            std::unordered_map<uint32_t, uint8_t> colors;
            colors.reserve(256);
            indices.reserve(rgba.size() / 4);
            for (size_t p = 0; p < rgba.size(); p += 4) {
                uint32_t key = uint32_t(rgba[p]) | uint32_t(rgba[p + 1]) << 8 |
                    uint32_t(rgba[p + 2]) << 16 | uint32_t(rgba[p + 3]) << 24;
                auto it = colors.find(key);
                if (it == colors.end()) {
                    if (colors.size() == 256) {
                        indices.clear();
                        return false;
                    }
                    auto index = static_cast<uint8_t>(colors.size());
                    it = colors.emplace(key, index).first;
                    std::copy_n(rgba.data() + p, 4, palette.data() + size_t(index) * 4);
                }
                indices.push_back(it->second);
            }
            return true;
        }

        std::vector<uint8_t> Decode() const {
            std::vector<uint8_t> rgba(indices.size() * 4);
            for (size_t p = 0; p < indices.size(); ++p)
                std::copy_n(palette.data() + size_t(indices[p]) * 4, 4, rgba.data() + p * 4);
            return rgba;
        }
    };

    struct IndexedAtlasLayout {
        int width = 0, height = 0, columns = 0;
        static IndexedAtlasLayout Make(int frameWidth, int frameHeight, int frames, int limit) {
            if (frameWidth <= 0 || frameHeight <= 0 || frames <= 0 || frames > limit ||
                frameWidth > limit || frameHeight > limit || limit < 256) return {};
            // Minimize padded area; no scaling or interpolation of indices.
            IndexedAtlasLayout best;
            int64_t bestArea = INT64_MAX;
            for (int columns = 1; columns <= std::min(frames, limit / frameWidth); ++columns) {
                int rows = (frames + columns - 1) / columns;
                if (rows > limit / frameHeight) continue;
                int width = columns * frameWidth, height = rows * frameHeight;
                int64_t area = int64_t(width) * height;
                if (area < bestArea || (area == bestArea && std::max(width, height) < std::max(best.width, best.height))) {
                    best = {width, height, columns};
                    bestArea = area;
                }
            }
            return best;
        }
    };
}
