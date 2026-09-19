#pragma once

#include "BSML/Parsing/ParseException.hpp"
#include "StringParseHelper.hpp"
#include "fmt/format.h"
#include <map>

namespace BSML {
    // Match Enum.Parse: case-sensitive names, numeric values, and comma-separated names.
    template<class T>
    T ParseEnum(std::string_view input, const std::map<std::string, T>& names, std::string_view property) {
        auto trim = [](std::string_view value) {
            auto first = value.find_first_not_of(" \t\r\n\v\f");
            return first == std::string_view::npos ? std::string_view{} :
                value.substr(first, value.find_last_not_of(" \t\r\n\v\f") - first + 1);
        };
        auto remaining = trim(input);
        if (auto number = StringParseHelper(remaining).tryParseInt()) return static_cast<T>(*number);
        int value = 0;
        for (;;) {
            auto comma = remaining.find(',');
            auto name = trim(remaining.substr(0, comma));
            auto found = names.find(std::string(name));
            if (found == names.end()) throw ParseException(fmt::format("Invalid {} '{}'", property, input));
            value |= static_cast<int>(found->second);
            if (comma == std::string_view::npos) return static_cast<T>(value);
            remaining.remove_prefix(comma + 1);
        }
    }
}
