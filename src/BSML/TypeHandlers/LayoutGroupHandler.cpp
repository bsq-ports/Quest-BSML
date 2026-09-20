#include "BSML/TypeHandlers/LayoutGroupHandler.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "EnumParseHelper.hpp"

static std::map<std::string, UnityEngine::TextAnchor> stringToTextAnchorMap {
    {"UpperLeft", UnityEngine::TextAnchor::UpperLeft},
    {"UpperCenter", UnityEngine::TextAnchor::UpperCenter},
    {"UpperRight", UnityEngine::TextAnchor::UpperRight},
    {"MiddleLeft", UnityEngine::TextAnchor::MiddleLeft},
    {"MiddleCenter", UnityEngine::TextAnchor::MiddleCenter},
    {"MiddleRight", UnityEngine::TextAnchor::MiddleRight},
    {"LowerLeft", UnityEngine::TextAnchor::LowerLeft},
    {"LowerCenter", UnityEngine::TextAnchor::LowerCenter},
    {"LowerRight", UnityEngine::TextAnchor::LowerRight}
};

namespace BSML {
    static LayoutGroupHandler layoutGroupHandler{};

    LayoutGroupHandler::Base::PropMap LayoutGroupHandler::get_props() const {
        return {
            {"padTop", {"pad-top"}},
            {"padBottom", {"pad-bottom"}},
            {"padLeft", {"pad-left"}},
            {"padRight", {"pad-right"}},
            {"pad", {"padding", "pad"}},
            {"childAlign",  {"child-alignment", "child-align"}}
        };
    }

    LayoutGroupHandler::Base::SetterMap LayoutGroupHandler::get_setters() const {
        return {

        };
    }

    void LayoutGroupHandler::HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleType(componentType, parserParams);
        auto layoutGroup = i2c::try_cast<UnityEngine::UI::LayoutGroup*>(componentType.component);
        if (!layoutGroup) {
            ERROR("LayoutGroupHandler::HandleType given a component that is not a LayoutGroup");
            return;
        }
        auto& data = componentType.data;

        auto padItr = data.find("pad");
        if (padItr != data.end()) {
            auto pad = StringParseHelper(padItr->second).tryParsePadding();
            if (pad) {
                layoutGroup->set_padding(UnityEngine::RectOffset::New_ctor(pad->left, pad->right, pad->top, pad->bottom));
            } else {
                throw ParseException(fmt::format("Invalid padding '{}': expected one to four integers", padItr->second));
            }
        }

        auto padTopItr = data.find("padTop");
        auto padBottomItr = data.find("padBottom");
        auto padLeftItr = data.find("padLeft");
        auto padRightItr = data.find("padRight");
        auto originalPadding = layoutGroup->get_padding();

        layoutGroup->set_padding(UnityEngine::RectOffset::New_ctor(
            padLeftItr == data.end()     ? originalPadding->get_left()   : StringParseHelper(padLeftItr->second), 
            padRightItr == data.end()    ? originalPadding->get_right()  : StringParseHelper(padRightItr->second),
            padTopItr == data.end()      ? originalPadding->get_top()    : StringParseHelper(padTopItr->second), 
            padBottomItr == data.end()   ? originalPadding->get_bottom() : StringParseHelper(padBottomItr->second) 
        ));

        auto childAlignItr = data.find("childAlign");
        if (childAlignItr != data.end()) {
            layoutGroup->set_childAlignment(ParseEnum(childAlignItr->second, stringToTextAnchorMap, "child-alignment"));
        }
    }
}
