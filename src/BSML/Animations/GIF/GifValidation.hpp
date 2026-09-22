#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace BSML::detail {
    // Validate the container before giflib allocates raster buffers. LZW decoding
    // and compositing remain giflib/EasyGifReader's responsibility.
    inline bool ValidateGif(std::span<const uint8_t> bytes) {
        constexpr uint64_t maxDecodedBytes = 128ULL * 1024 * 1024;
        if (bytes.size() < 13 || bytes.size() > 64 * 1024 * 1024 ||
            (std::memcmp(bytes.data(), "GIF87a", 6) && std::memcmp(bytes.data(), "GIF89a", 6))) return false;
        auto word = [&](size_t pos) { return unsigned(bytes[pos]) | unsigned(bytes[pos + 1]) << 8; };
        auto width = word(6), height = word(8);
        if (!width || !height || width > 4096 || height > 4096) return false;
        size_t pos = 13;
        auto skip = [&](size_t size) {
            if (size > bytes.size() - pos) return false;
            pos += size;
            return true;
        };
        auto blocks = [&]() {
            while (pos < bytes.size()) {
                auto size = bytes[pos++];
                if (!size) return true;
                if (!skip(size)) return false;
            }
            return false;
        };
        bool globalPalette = bytes[10] & 0x80;
        if (globalPalette && !skip(3u << ((bytes[10] & 7) + 1))) return false;
        unsigned frames = 0;
        while (pos < bytes.size()) {
            switch (bytes[pos++]) {
                case 0x3b: return frames != 0;
                case 0x21:
                    if (!skip(1) || !blocks()) return false;
                    break;
                case 0x2c: {
                    if (bytes.size() - pos < 9) return false;
                    auto left = word(pos), top = word(pos + 2);
                    auto w = word(pos + 4), h = word(pos + 6);
                    auto flags = bytes[pos + 8];
                    pos += 9;
                    if (!w || !h || left + w > width || top + h > height ||
                        ++frames > 1024 || uint64_t(width) * height * frames * 4 > maxDecodedBytes) return false;
                    if (flags & 0x80) {
                        if (!skip(3u << ((flags & 7) + 1))) return false;
                    } else if (!globalPalette) return false;
                    if (pos == bytes.size() || bytes[pos] < 2 || bytes[pos] > 8) return false;
                    ++pos;
                    if (!blocks()) return false;
                    break;
                }
                default: return false;
            }
        }
        return false;
    }
}
