#include "StringParseHelper.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "Helpers/utilities.hpp"
#include "logging.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>

namespace {
    std::string_view TrimParseWhitespace(std::string_view input) {
        auto first = input.find_first_not_of(" \t\r\n\v\f");
        if (first == std::string_view::npos) return {};
        return input.substr(first, input.find_last_not_of(" \t\r\n\v\f") - first + 1);
    }

    template<class Number>
    std::optional<Number> ParseNumber(std::string_view input) {
        input = TrimParseWhitespace(input);
        if (input.empty()) return std::nullopt;

        // from_chars is locale-independent but does not accept a leading plus.
        if (input.front() == '+') {
            input.remove_prefix(1);
            if (input.empty() || input.front() == '-') return std::nullopt;
        }

        Number value{};
        const auto end = input.data() + input.size();
        const auto result = std::from_chars(input.data(), end, value);
        // A valid prefix is not enough: reject trailing text and out-of-range values.
        if (result.ec != std::errc{} || result.ptr != end) return std::nullopt;
        return value;
    }

    template<class T, std::size_t Capacity>
    struct ParsedComponents {
        std::array<T, Capacity> values{};
        std::size_t count = 0;
    };

    template<class T, std::size_t Capacity, class ParseComponent>
    std::optional<ParsedComponents<T, Capacity>> ParseSpaceSeparatedComponents(
        std::string_view input, ParseComponent parseComponent) {
        ParsedComponents<T, Capacity> components;
        while (!input.empty()) {
            const auto end = input.find(' ');
            const auto token = input.substr(0, end);
            input = end == std::string_view::npos ? std::string_view{} : input.substr(end + 1);
            // PC separates components on spaces, not on every whitespace character.
            if (token.empty()) continue;
            if (components.count == Capacity) return std::nullopt;

            auto value = parseComponent(token);
            if (!value) return std::nullopt;
            components.values[components.count++] = *value;
        }
        if (components.count == 0) return std::nullopt;
        return components;
    }

    template<std::size_t Dimensions>
    std::optional<ParsedComponents<float, Dimensions>> ParseVectorComponents(std::string_view input) {
        return ParseSpaceSeparatedComponents<float, Dimensions>(input, [](std::string_view token) {
            return StringParseHelper(token).tryParseFloat();
        });
    }
}

// splits this view into a vector of views into the different parts
std::vector<std::string_view> StringParseHelper::split(char split) const {
    std::vector<std::string_view> parts;
    std::size_t start = 0, end;
    while((end = this->find(split, start)) != std::string::npos) {
        auto part = this->substr(start, end - start);
        start = end + 1;
        // if empty, skip
        if (part.empty()) continue;
        // if only whitespace, skip this part
        if (std::find_if(part.begin(), part.end(), [](auto c){ return !std::isspace(c); }) == part.end()) continue;

        parts.emplace_back(part);
    }

    parts.emplace_back(this->substr(start));
    return parts;
}
/// makes a string thats lowercase
std::string StringParseHelper::toLower() const {
    std::string ret{data(), size()};
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

std::string StringParseHelper::toUpper() const {
    std::string ret{data(), size()};
    std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
    return ret;
}

std::optional<bool> StringParseHelper::tryParseBool() const {
    auto lower = toLower();
    if (lower == "true")
        return true;
    if (lower == "false")
        return false;
    return std::nullopt;
}

std::optional<int> StringParseHelper::tryParseInt() const {
    return ParseNumber<int>(*this);
}
std::optional<float> StringParseHelper::tryParseFloat() const {
    auto value = tryParseDouble();
    if (!value) return std::nullopt;
    // Check the rounded result so decimal representations of FLT_MAX remain valid.
    auto result = static_cast<float>(*value);
    if (std::isfinite(*value) && std::isinf(result)) return std::nullopt;
    return result;
}

std::optional<double> StringParseHelper::tryParseDouble() const {
    auto input = TrimParseWhitespace(*this);
    // Keep .NET's special-value spellings; reject hex numbers and NaN payloads.
    if (input == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (input == "Infinity") return std::numeric_limits<double>::infinity();
    if (input == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (input.find_first_not_of("0123456789.eE+-") != std::string_view::npos) return std::nullopt;
    return ParseNumber<double>(input);
}

UnityEngine::Vector3 StringParseHelper::parseVector3(float defaultZ) const {
    auto result = tryParseVector3(defaultZ);
    if (!result) throw BSML::ParseException(fmt::format("Could not parse Vector3 from '{}': expected one to three numbers", *this));
    return *result;
}
std::optional<UnityEngine::Color> StringParseHelper::tryParseColor() const {
    return BSML::Utilities::ParseHTMLColorOpt(*this);
}
std::optional<UnityEngine::Color32> StringParseHelper::tryParseColor32() const {
    return BSML::Utilities::ParseHTMLColor32Opt(*this);
}
std::optional<UnityEngine::Vector2> StringParseHelper::tryParseVector2() const {
    auto parsed = ParseVectorComponents<2>(*this);
    if (!parsed) return std::nullopt;
    const auto& components = parsed->values;
    const auto count = parsed->count;
    return UnityEngine::Vector2{components[0], count == 1 ? components[0] : components[1]};
}
std::optional<UnityEngine::Vector3> StringParseHelper::tryParseVector3(float defaultZ) const {
    auto parsed = ParseVectorComponents<3>(*this);
    if (!parsed) return std::nullopt;
    const auto& components = parsed->values;
    const auto count = parsed->count;
    // A scalar fills every axis; defaultZ applies only to two-component input.
    if (count == 1) return UnityEngine::Vector3{components[0], components[0], components[0]};
    return UnityEngine::Vector3{components[0], components[1], count == 2 ? defaultZ : components[2]};
}
std::optional<UnityEngine::Vector4> StringParseHelper::tryParseVector4() const {
    auto parsed = ParseVectorComponents<4>(*this);
    if (!parsed) return std::nullopt;
    const auto& components = parsed->values;
    const auto count = parsed->count;
    auto x = components[0];
    auto y = count > 1 ? components[1] : x;
    return UnityEngine::Vector4{x, y, count > 2 ? components[2] : x, count > 3 ? components[3] : y};
}
std::optional<StringParseHelper::Padding> StringParseHelper::tryParsePadding() const {
    auto parsed = ParseSpaceSeparatedComponents<int, 4>(*this, ParseNumber<int>);
    if (!parsed) return std::nullopt;
    const auto& values = parsed->values;
    const auto count = parsed->count;

    // CSS shorthand: all; vertical horizontal; top horizontal bottom;
    // or top right bottom left. This also matches PC's actual output.
    int top = values[0];
    int right = count > 1 ? values[1] : top;
    int bottom = count > 2 ? values[2] : top;
    int left = count > 3 ? values[3] : right;
    return Padding{left, right, top, bottom};
}

const MethodInfo* minfo_from_name_in_parents(Il2CppClass* klass, const char* name, int argc) {
    if (!klass) return nullptr;
    auto minfo = i2c::functions::class_get_method_from_name(klass, name, argc);
    if (minfo) return minfo;
    return i2c::functions::class_get_method_from_name(klass->parent, name, argc);
}
const MethodInfo* StringParseHelper::asMethodInfo(System::Object* host, int argCount) const {
    return minfo_from_name_in_parents(host->klass, data(), argCount);
}
const MethodInfo* StringParseHelper::asSetter(System::Object* host) const {
    auto name = "set_" + this->operator std::string();
    return i2c::functions::class_get_method_from_name(host->klass, name.c_str(), 1);
}
const MethodInfo* StringParseHelper::asGetter(System::Object* host) const {
    auto name = "get_" + this->operator std::string();
    return i2c::functions::class_get_method_from_name(host->klass, name.c_str(), 0);
}
FieldInfo* StringParseHelper::asFieldInfo(System::Object* host) const {
    return i2c::functions::class_get_field_from_name(host->klass, data());
}
StringParseHelper::operator StringW() const {
    return StringW(*this);
}
StringParseHelper::operator std::string() const {
    return {data(), size()};
}

StringParseHelper::operator bool() const {
    auto result = StringParseHelper(TrimParseWhitespace(*this)).tryParseBool();
    if (!result) throw BSML::ParseException(fmt::format("Could not parse bool from '{}'", *this));
    return *result;
}
StringParseHelper::operator int() const {
    auto result = tryParseInt();
    if (!result) throw BSML::ParseException(fmt::format("Could not parse integer from '{}'", *this));
    return *result;
}
StringParseHelper::operator float() const {
    auto result = tryParseFloat();
    if (!result) throw BSML::ParseException(fmt::format("Could not parse float from '{}'", *this));
    return *result;
}
StringParseHelper::operator double() const {
    auto result = tryParseDouble();
    if (!result) throw BSML::ParseException(fmt::format("Could not parse double from '{}'", *this));
    return *result;
}
StringParseHelper::operator UnityEngine::Color() const {
    auto result = tryParseColor();
    if (!result) throw BSML::ParseException(fmt::format("Invalid color '{}'", *this));
    return *result;
}
StringParseHelper::operator UnityEngine::Color32() const {
    auto result = tryParseColor32();
    if (!result) throw BSML::ParseException(fmt::format("Invalid color '{}'", *this));
    return *result;
}
StringParseHelper::operator UnityEngine::Vector2() const {
    auto result = tryParseVector2();
    if (!result) throw BSML::ParseException(fmt::format("Could not parse Vector2 from '{}': expected one or two numbers", *this));
    return *result;
}
StringParseHelper::operator UnityEngine::Vector3() const { return parseVector3(); }
StringParseHelper::operator UnityEngine::Vector4() const {
    auto result = tryParseVector4();
    if (!result) throw BSML::ParseException(fmt::format("Could not parse Vector4 from '{}': expected one to four numbers", *this));
    return *result;
}
