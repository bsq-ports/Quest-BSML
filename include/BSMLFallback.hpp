#pragma once

#include <string>
#include <string_view>

namespace BSML::detail {
    inline constexpr auto DefaultFallbackContent = R"(<bg>
        <vertical child-control-height='false' child-control-width='true' child-align='UpperCenter' pref-width='110' pad-left='3' pad-right='3'>
            <horizontal bg='panel-top' pad-left='10' pad-right='10' horizontal-fit='PreferredSize' vertical-fit='PreferredSize'>
                <text text='Invalid BSML' font-size='10'/>
            </horizontal>
        </vertical>
        <text-page text='{0}' rich-text='false' anchor-min-x='0.1' anchor-max-x='0.9' anchor-max-y='0.8'/>
    </bg>)";

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
