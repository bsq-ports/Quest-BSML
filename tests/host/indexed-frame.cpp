#include "IndexedFrame.hpp"
#include <cassert>
#include <random>

using namespace BSML::detail;

int main() {
    IndexedFrame frame;
    // Every index, including 255, and distinct alpha values must round-trip.
    std::vector<uint8_t> rgba;
    for (int i = 0; i < 256; ++i) {
        rgba.push_back(i);
        rgba.push_back(255 - i);
        rgba.push_back(i / 2);
        rgba.push_back(i % 3 ? 255 : 0);
    }
    assert(frame.Encode(rgba));
    assert(frame.indices.size() == 256);
    assert(frame.indices.back() == 255);
    assert(frame.Decode() == rgba);
    rgba.insert(rgba.end(), {1, 2, 3, 4});
    assert(!frame.Encode(rgba)); // 257th composited color: whole-GIF fallback
    assert(frame.indices.empty());
    rgba = {0, 0, 0, 0, 0, 0, 0, 255};
    assert(frame.Encode(rgba));
    assert(frame.indices[0] != frame.indices[1]);
    assert(frame.Decode() == rgba);
    assert(!frame.Encode({}));
    assert(!frame.Encode(std::span<const uint8_t>(rgba.data(), 3)));

    // Independent random images with up to 256 colors, across many palettes.
    std::mt19937 random(0xB5A1);
    for (int count = 1; count <= 256; ++count) {
        std::vector<std::array<uint8_t, 4>> colors(count);
        for (auto& color : colors) for (auto& channel : color) channel = random();
        rgba.clear();
        for (int p = 0; p < 1000; ++p) {
            auto color = colors[random() % count];
            rgba.insert(rgba.end(), color.begin(), color.end());
        }
        assert(frame.Encode(rgba));
        assert(frame.Decode() == rgba);
    }
    auto layout = IndexedAtlasLayout::Make(256, 256, 100, 4096);
    assert(layout.columns && layout.width * layout.height == 100 * 256 * 256);
    for (int width : {1, 17, 256, 4096}) {
        for (int height : {1, 23, 256, 4096}) {
            for (int count : {1, 3, 17, 100, 1024}) {
                auto grid = IndexedAtlasLayout::Make(width, height, count, 4096);
                if (!grid.columns) {
                    assert((4096 / width) * (4096 / height) < count);
                    continue;
                }
                assert(grid.width <= 4096 && grid.height <= 4096);
                assert(grid.width % width == 0 && grid.height % height == 0);
                assert((grid.width / width) * (grid.height / height) >= count);
            }
        }
    }
    assert(!IndexedAtlasLayout::Make(0, 1, 1, 4096).columns);
    assert(!IndexedAtlasLayout::Make(1, 1, 0, 4096).columns);
    assert(!IndexedAtlasLayout::Make(4097, 1, 1, 4096).columns);
    assert(!IndexedAtlasLayout::Make(16, 16, 4097, 4096).columns);
}
