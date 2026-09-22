// Standalone Android test: runs on Quest without Unity or changing the game.
#include <cstdint>
#include <cstdio>
#include "GifFixtures.hpp"
#include "GifValidation.hpp"
#include "EasyGifReader.h"

int main() {
    int failures = 0;
    for (const auto& fixture : gifFixtures) {
        bool failedDecode = false, matches = true;
        size_t index = 0;
        try {
            if (!BSML::detail::ValidateGif(fixture.bytes)) throw EasyGifReader::Error::INVALID_GIF_FILE;
            auto reader = EasyGifReader::openMemory(fixture.bytes.data(), fixture.bytes.size());
            matches = reader.width() == fixture.width && reader.height() == fixture.height &&
                reader.frameCount() == fixture.hashes.size();
            for (const auto& frame : reader) {
                if (index >= fixture.hashes.size()) { matches = false; break; }
                uint32_t hash = 2166136261u;
                auto bytes = reinterpret_cast<const uint8_t*>(frame.pixels());
                for (int y = frame.height() - 1; y >= 0; --y) {
                    for (int x = 0; x < frame.width() * 4; ++x) {
                        auto row = bytes + y * frame.width() * 4;
                        auto value = x % 4 != 3 && row[x / 4 * 4 + 3] == 0 ? 0 : row[x];
                        hash = (hash ^ value) * 16777619u;
                    }
                }
                if (hash != fixture.hashes[index] || frame.duration().milliseconds() != fixture.delays[index]) {
                    std::printf("  frame %zu hash %u expected %u; delay %d expected %d\n", index,
                        hash, fixture.hashes[index], frame.duration().milliseconds(), fixture.delays[index]);
                    matches = false;
                }
                ++index;
            }
        } catch (...) { failedDecode = true; }
        bool passed = fixture.width ? !failedDecode && matches && index == fixture.hashes.size() : failedDecode;
        std::printf("%s %s\n", passed ? "PASS" : "FAIL", fixture.name);
        if (!passed) ++failures;
    }
    std::printf("%d decoder failures\n", failures);
    return failures ? 1 : 0;
}
