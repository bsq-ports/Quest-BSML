#include "StringParseHelper.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "Helpers/utilities.hpp"
#include "logging.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <concepts>
#include <limits>

namespace {
    std::string_view TrimParseWhitespace(std::string_view input) {
        auto first = input.find_first_not_of(" \t\r\n\v\f");
        if (first == std::string_view::npos) return {};
        return input.substr(first, input.find_last_not_of(" \t\r\n\v\f") - first + 1);
    }

    // Shared "unwrap or throw" body for the implicit-conversion operators below —
    // `message` is only invoked (and only builds the fmt::format string) on the
    // failure path, so this doesn't cost anything extra on a successful parse.
    template<typename T>
    T ParseOrThrow(std::optional<T> result, std::invocable auto message) {
        if (!result) throw BSML::ParseException(message());
        return *result;
    }

    template<std::size_t N>
    std::optional<std::pair<std::array<float, N>, std::size_t>> ParseVectorComponents(std::string_view input) {
        std::array<float, N> values{};
        std::size_t count = 0;
        while (!input.empty()) {
            auto end = input.find(' ');
            auto token = input.substr(0, end);
            input = end == std::string_view::npos ? std::string_view{} : input.substr(end + 1);
            if (token.empty()) continue; // PC splits on spaces and removes empty entries.
            if (count == N) return std::nullopt;
            auto value = StringParseHelper(TrimParseWhitespace(token)).tryParseFloat();
            if (!value) return std::nullopt;
            values[count++] = *value;
        }
        if (count == 0) return std::nullopt;
        return std::pair{values, count};
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
    auto input = TrimParseWhitespace(*this);
    if (input.empty()) return std::nullopt;
    if (input.front() == '+') {
        input.remove_prefix(1);
        if (input.empty() || input.front() == '-') return std::nullopt;
    }
    int value;
    auto result = std::from_chars(input.data(), input.data() + input.size(), value);
    if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) return std::nullopt;
    return value;
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
    // Invariant .NET numeric syntax; do not accept strtod's hex numbers or NaN payloads.
    if (input == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (input == "Infinity") return std::numeric_limits<double>::infinity();
    if (input == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (input.empty()) return std::nullopt;
    if (input.front() == '+') {
        input.remove_prefix(1);
        if (input.empty() || input.front() == '-') return std::nullopt;
    }
    if (input.find_first_not_of("0123456789.eE+-") != std::string_view::npos) return std::nullopt;
    double value;
    auto result = std::from_chars(input.data(), input.data() + input.size(), value, std::chars_format::general);
    if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) return std::nullopt;
    return value;
}

UnityEngine::Vector3 StringParseHelper::parseVector3(float defaultZ) const {
    return ParseOrThrow(tryParseVector3(defaultZ), [this]{ return fmt::format("Could not parse Vector3 from '{}': expected one to three numbers", *this); });
}
std::optional<UnityEngine::Color> StringParseHelper::tryParseColor() const {
    return BSML::Utilities::ParseHTMLColorOpt(*this);
}
std::optional<UnityEngine::Color32> StringParseHelper::tryParseColor32() const {
    return BSML::Utilities::ParseHTMLColor32Opt(*this);
}
std::optional<UnityEngine::Vector2> StringParseHelper::tryParseVector2() const {
    auto values = ParseVectorComponents<2>(*this);
    if (!values) return std::nullopt;
    auto& [components, count] = *values;
    return UnityEngine::Vector2{components[0], count == 1 ? components[0] : components[1]};
}
std::optional<UnityEngine::Vector3> StringParseHelper::tryParseVector3(float defaultZ) const {
    auto values = ParseVectorComponents<3>(*this);
    if (!values) return std::nullopt;
    auto& [components, count] = *values;
    if (count == 1) return UnityEngine::Vector3{components[0], components[0], components[0]};
    return UnityEngine::Vector3{components[0], components[1], count == 2 ? defaultZ : components[2]};
}
std::optional<UnityEngine::Vector4> StringParseHelper::tryParseVector4() const {
    auto values = ParseVectorComponents<4>(*this);
    if (!values) return std::nullopt;
    auto& [components, count] = *values;
    auto x = components[0];
    auto y = count > 1 ? components[1] : x;
    return UnityEngine::Vector4{x, y, count > 2 ? components[2] : x, count > 3 ? components[3] : y};
}
std::optional<StringParseHelper::Padding> StringParseHelper::tryParsePadding() const {
    std::array<int, 4> values{};
    std::size_t count = 0;
    std::string_view input = *this;
    while (!input.empty()) {
        auto end = input.find(' ');
        auto token = input.substr(0, end);
        input = end == std::string_view::npos ? std::string_view{} : input.substr(end + 1);
        if (token.empty()) continue;
        if (count == values.size()) return std::nullopt;

        // Match Int parsing on PC, including a leading plus and surrounding whitespace.
        auto first = token.find_first_not_of("\t\r\n\v\f");
        if (first == std::string_view::npos) return std::nullopt;
        token = token.substr(first, token.find_last_not_of("\t\r\n\v\f") - first + 1);
        if (token.front() == '+') {
            token.remove_prefix(1);
            if (token.empty() || token.front() == '-') return std::nullopt;
        }
        auto result = std::from_chars(token.data(), token.data() + token.size(), values[count]);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) return std::nullopt;
        ++count;
    }
    if (count == 0) return std::nullopt;

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
    return ParseOrThrow(StringParseHelper(TrimParseWhitespace(*this)).tryParseBool(), [this]{ return fmt::format("Could not parse bool from '{}'", *this); });
}
StringParseHelper::operator int() const {
    return ParseOrThrow(tryParseInt(), [this]{ return fmt::format("Could not parse integer from '{}'", *this); });
}
StringParseHelper::operator float() const {
    return ParseOrThrow(StringParseHelper(TrimParseWhitespace(*this)).tryParseFloat(), [this]{ return fmt::format("Could not parse float from '{}'", *this); });
}
StringParseHelper::operator double() const {
    return ParseOrThrow(tryParseDouble(), [this]{ return fmt::format("Could not parse double from '{}'", *this); });
}
StringParseHelper::operator UnityEngine::Color() const {
    return ParseOrThrow(tryParseColor(), [this]{ return fmt::format("Invalid color '{}'", *this); });
}
StringParseHelper::operator UnityEngine::Color32() const {
    return ParseOrThrow(tryParseColor32(), [this]{ return fmt::format("Invalid color '{}'", *this); });
}
StringParseHelper::operator UnityEngine::Vector2() const {
    return ParseOrThrow(tryParseVector2(), [this]{ return fmt::format("Could not parse Vector2 from '{}': expected one or two numbers", *this); });
}
StringParseHelper::operator UnityEngine::Vector3() const { return parseVector3(); }
StringParseHelper::operator UnityEngine::Vector4() const {
    return ParseOrThrow(tryParseVector4(), [this]{ return fmt::format("Could not parse Vector4 from '{}': expected one to four numbers", *this); });
}
