#pragma once

#include "BSML/Parsing/ParseException.hpp"
#include "StringParseHelper.hpp"
#include "fmt/format.h"
#include <map>

namespace BSML {
    // Accept one case-sensitive name or numeric value, with surrounding whitespace.
    template<class T>
    T ParseEnum(std::string_view input, const std::map<std::string, T>& names, std::string_view property) {
        auto trim = [](std::string_view value) {
            auto first = value.find_first_not_of(" \t\r\n\v\f");
            return first == std::string_view::npos ? std::string_view{} :
                value.substr(first, value.find_last_not_of(" \t\r\n\v\f") - first + 1);
        };
        const auto value = trim(input);
        if (auto number = StringParseHelper(value).tryParseInt()) return static_cast<T>(*number);
        if (auto found = names.find(std::string(value)); found != names.end()) return found->second;
        throw ParseException(fmt::format("Invalid {} '{}'", property, input));
    }
}
