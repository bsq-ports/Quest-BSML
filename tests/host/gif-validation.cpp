#include "GifValidation.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

std::vector<uint8_t> Read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    assert(file);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv) {
    assert(argc == 2);
    using BSML::detail::ValidateGif;
    std::string root = argv[1];
    for (auto name : {"a", "b", "single", "alpha"}) {
        auto bytes = Read(root + "/" + name + ".gif");
        assert(ValidateGif(bytes));
        for (size_t length = 0; length < bytes.size(); ++length)
            assert(!ValidateGif({bytes.data(), length}));
        auto invalid = bytes;
        invalid[0] = 0; assert(!ValidateGif(invalid));
        invalid = bytes; invalid[6] = invalid[7] = 0; assert(!ValidateGif(invalid));
        invalid = bytes; invalid[6] = invalid[7] = 255; assert(!ValidateGif(invalid));
    }
    for (auto name : {"bad", "truncated", "empty"})
        assert(!ValidateGif(Read(root + "/" + name + ".gif")));
    // This fixture must reach giflib, whose LZW failure is tested on Quest.
    assert(ValidateGif(Read(root + "/lzw.gif")));
    auto bytes = Read(root + "/single.gif");
    size_t descriptor = 13 + (3u << ((bytes[10] & 7) + 1));
    assert(bytes[descriptor] == 0x2c);
    bytes[descriptor + 1] = 255; // Frame rectangle lies outside the screen.
    assert(!ValidateGif(bytes));
    // A small compressed GIF with excessive composited frame memory must be rejected.
    bytes = Read(root + "/b.gif");
    bytes[6] = bytes[8] = 0; bytes[7] = bytes[9] = 16; // 4096 x 4096
    assert(!ValidateGif(bytes));
}
