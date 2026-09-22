#include "StringParseHelper.hpp"
#include "logging.hpp"

#include <limits>
#include <string>

namespace BSML {
    std::string RunPaddingParserTests() {
        int checks = 0;
        int failures = 0;
        auto record = [&](std::string_view input, bool passed) {
            ++checks;
            if (!passed) {
                ++failures;
                ERROR("Padding parser regression failed for '{}'", input);
            }
        };
        auto check = [&](std::string_view input, StringParseHelper::Padding expected) {
            auto actual = StringParseHelper(input).tryParsePadding();
            record(input, actual && actual->left == expected.left && actual->right == expected.right
                && actual->top == expected.top && actual->bottom == expected.bottom);
        };

        // CSS shorthand; expected Padding fields are left, right, top, bottom.
        check("2", {2, 2, 2, 2});
        check("1 2", {2, 2, 1, 1});
        check("1 2 3", {2, 2, 1, 3});
        check("1 2 3 4", {4, 2, 1, 3});
        check("  1   2 3 4  ", {4, 2, 1, 3});
        check("+1 -2 0 +4", {4, -2, 1, 0});
        check("\t2\t", {2, 2, 2, 2});
        check(std::to_string(std::numeric_limits<int>::max()), {2147483647, 2147483647, 2147483647, 2147483647});
        const std::string backing = "1 2 3 49";
        check(std::string_view(backing).substr(0, 7), {4, 2, 1, 3});

        for (auto invalid : {"", " ", "\t", "1 2 3 4 5", "abc", "1x", "1.5", "+", "+-1", "1\t2", "2147483648", "-2147483649"}) {
            record(invalid, !StringParseHelper(invalid).tryParsePadding());
        }
        auto report = fmt::format("Padding parser: {}/{} passed", checks - failures, checks);
        INFO("{}", report);
        return report;
    }
}
