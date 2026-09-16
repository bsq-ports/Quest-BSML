#include "BSML/TypeHandlers/TextHandler.hpp"
#include "EnumParseHelper.hpp"
#include "Helpers/utilities.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "TMPro/TextOverflowModes.hpp"
#include "TMPro/FontStyles.hpp"

static std::map<std::string, TMPro::TextAlignmentOptions> stringToTextAlignmentOptions {
    {"TopLeft", TMPro::TextAlignmentOptions::TopLeft},
    {"Top", TMPro::TextAlignmentOptions::Top},
    {"TopRight", TMPro::TextAlignmentOptions::TopRight},
    {"TopJustified", TMPro::TextAlignmentOptions::TopJustified},
    {"TopFlush", TMPro::TextAlignmentOptions::TopFlush},
    {"TopGeoAligned", TMPro::TextAlignmentOptions::TopGeoAligned},
    {"Left", TMPro::TextAlignmentOptions::Left},
    {"Center", TMPro::TextAlignmentOptions::Center},
    {"Right", TMPro::TextAlignmentOptions::Right},
    {"Justified", TMPro::TextAlignmentOptions::Justified},
    {"Flush", TMPro::TextAlignmentOptions::Flush},
    {"CenterGeoAligned", TMPro::TextAlignmentOptions::CenterGeoAligned},
    {"BottomLeft", TMPro::TextAlignmentOptions::BottomLeft},
    {"Bottom", TMPro::TextAlignmentOptions::Bottom},
    {"BottomRight", TMPro::TextAlignmentOptions::BottomRight},
    {"BottomJustified", TMPro::TextAlignmentOptions::BottomJustified},
    {"BottomFlush", TMPro::TextAlignmentOptions::BottomFlush},
    {"BottomGeoAligned", TMPro::TextAlignmentOptions::BottomGeoAligned},
    {"BaselineLeft", TMPro::TextAlignmentOptions::BaselineLeft},
    {"Baseline", TMPro::TextAlignmentOptions::Baseline},
    {"BaselineRight", TMPro::TextAlignmentOptions::BaselineRight},
    {"BaselineJustified", TMPro::TextAlignmentOptions::BaselineJustified},
    {"BaselineFlush", TMPro::TextAlignmentOptions::BaselineFlush},
    {"BaselineGeoAligned", TMPro::TextAlignmentOptions::BaselineGeoAligned},
    {"MidlineLeft", TMPro::TextAlignmentOptions::MidlineLeft},
    {"Midline", TMPro::TextAlignmentOptions::Midline},
    {"MidlineRight", TMPro::TextAlignmentOptions::MidlineRight},
    {"MidlineJustified", TMPro::TextAlignmentOptions::MidlineJustified},
    {"MidlineFlush", TMPro::TextAlignmentOptions::MidlineFlush},
    {"MidlineGeoAligned", TMPro::TextAlignmentOptions::MidlineGeoAligned},
    {"CaplineLeft", TMPro::TextAlignmentOptions::CaplineLeft},
    {"Capline", TMPro::TextAlignmentOptions::Capline},
    {"CaplineRight", TMPro::TextAlignmentOptions::CaplineRight},
    {"CaplineJustified", TMPro::TextAlignmentOptions::CaplineJustified},
    {"CaplineFlush", TMPro::TextAlignmentOptions::CaplineFlush},
    {"CaplineGeoAligned", TMPro::TextAlignmentOptions::CaplineGeoAligned}
};

static std::map<std::string, TMPro::TextOverflowModes> stringToOverflowModes {
    {"Overflow", TMPro::TextOverflowModes::Overflow},
    {"Ellipsis", TMPro::TextOverflowModes::Ellipsis},
    {"Masking", TMPro::TextOverflowModes::Masking},
    {"Truncate", TMPro::TextOverflowModes::Truncate},
    {"ScrollRect", TMPro::TextOverflowModes::ScrollRect},
    {"Page", TMPro::TextOverflowModes::Page},
    {"Linked", TMPro::TextOverflowModes::Linked}
};

TMPro::FontStyles SetStyle(TMPro::FontStyles existing, TMPro::FontStyles modify, bool flag) {
    if (flag)
        return TMPro::FontStyles(existing.value__ | modify.value__);
    else
        return TMPro::FontStyles(existing.value__ & ~modify.value__);

}

namespace BSML {
    static TextHandler textHandler{};

    TextHandler::Base::PropMap TextHandler::get_props() const {
        return {
            {"enableAutoSizing", {"enable-auto-sizing"}},
            {"text", {"text", "label"}},
            {"fontSize", {"font-size"}},
            {"fontSizeMin", {"font-size-min"}},
            {"fontSizeMax", {"font-size-max"}},
            {"fontColor", {"font-color", "color"}},
            {"faceColor", {"face-color"}},
            {"outlineColor", {"outline-color"}},
            {"outlineWidth", {"outline-width"}},
            {"richText", {"rich-text"}},
            {"fontAlign", {"font-align", "align"}},
            {"overflowMode", {"overflow-mode"}},
            {"wordWrapping", {"word-wrapping"}},
            {"bold", {"bold"}},
            {"italics", {"italics"}},
            {"underlined", {"underlined"}},
            {"strikethrough", {"strikethrough"}},
            {"allUppercase", {"all-uppercase"}},
            {"lineSpacing", {"line-spacing"}},
            {"lineSpacingAdjustment", {"line-spacing-adjustment"}},
            {"paragraphSpacing", {"paragraph-spacing"}}
        };
    }

    TextHandler::Base::SetterMap TextHandler::get_setters() const {
        return {
            {"enableAutoSizing", [](auto component, auto value){ component->set_enableAutoSizing(value); }},
            {"text",            [](auto component, auto value){ component->set_text(value); }},
            {"fontSize",       [](auto component, auto value){ component->set_fontSize(value); }},
            {"fontSizeMin",    [](auto component, auto value){ component->set_fontSizeMin(value); }},
            {"fontSizeMax",    [](auto component, auto value){ component->set_fontSizeMax(value); }},
            {"fontColor",      [](auto component, auto value){ component->set_color(value); }},
            {"faceColor",      [](auto component, auto value){ component->set_faceColor(value); }},
            {"outlineColor",   [](auto component, auto value){ component->set_outlineColor(value); }},
            {"outlineWidth",   [](auto component, auto value){ component->set_outlineWidth(value); }},
            {"richText",       [](auto component, auto value){ component->set_richText(value); }},
            {"fontAlign",      [](auto component, auto value){ component->set_alignment(ParseEnum(value, stringToTextAlignmentOptions, "text alignment")); }},
            {"overflowMode",   [](auto component, auto value){ component->set_overflowMode(ParseEnum(value, stringToOverflowModes, "overflow mode")); }},
            {"wordWrapping",   [](auto component, auto value){ component->set_enableWordWrapping(value); }},
            {"bold",            [](auto component, auto value){ component->set_fontStyle(SetStyle(component->get_fontStyle(), TMPro::FontStyles::Bold, value)); }},
            {"italics",         [](auto component, auto value){ component->set_fontStyle(SetStyle(component->get_fontStyle(), TMPro::FontStyles::Italic, value)); }},
            {"underlined",      [](auto component, auto value){ component->set_fontStyle(SetStyle(component->get_fontStyle(), TMPro::FontStyles::Underline, value)); }},
            {"strikethrough",   [](auto component, auto value){ component->set_fontStyle(SetStyle(component->get_fontStyle(), TMPro::FontStyles::Strikethrough, value)); }},
            {"allUppercase",   [](auto component, auto value){ component->set_fontStyle(SetStyle(component->get_fontStyle(), TMPro::FontStyles::UpperCase, value)); }},
            {"lineSpacing",    [](auto component, auto value){ component->set_lineSpacing(value); }},
            {"lineSpacingAdjustment", [](auto component, auto value){ component->set_lineSpacingAdjustment(value); }},
            {"paragraphSpacing", [](auto component, auto value){ component->set_paragraphSpacing(value); }}
        };
    }
}
