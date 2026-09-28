#pragma once

#include <string>
#include <string_view>

namespace BSML::detail {
    inline std::string FormatFallbackContent(std::string markup, std::string_view error) {
        std::string escaped;
        for (char c : error) {
            switch (c) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '\'': escaped += "&apos;"; break;
                case '"': escaped += "&quot;"; break;
                default: escaped += c; break;
            }
        }
        for (size_t pos = 0; (pos = markup.find("{0}", pos)) != std::string::npos; pos += escaped.size())
            markup.replace(pos, 3, escaped);
        return markup;
    }
}
