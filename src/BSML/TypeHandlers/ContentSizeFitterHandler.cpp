#include "BSML/TypeHandlers/ContentSizeFitterHandler.hpp"
#include "EnumParseHelper.hpp"

static std::map<std::string, UnityEngine::UI::ContentSizeFitter::FitMode> stringToFitModeMap {
    {"Unconstrained", UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained},
    {"MinSize", UnityEngine::UI::ContentSizeFitter::FitMode::MinSize},
    {"PreferredSize", UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize}
};

namespace BSML {
    static ContentSizeFitterHandler contentSizeFitterHandler{};

    ContentSizeFitterHandler::Base::PropMap ContentSizeFitterHandler::get_props() const {
        return {
            {"horizontalFit", {"horizontal-fit"}},
            {"verticalFit", {"vertical-fit"}}
        };
    }

    ContentSizeFitterHandler::Base::SetterMap ContentSizeFitterHandler::get_setters() const {
        return {
            {"horizontalFit", [](auto component, auto value){ component->set_horizontalFit(ParseEnum(value, stringToFitModeMap, "horizontal-fit")); }},
            {"verticalFit", [](auto component, auto value){ component->set_verticalFit(ParseEnum(value, stringToFitModeMap, "vertical-fit")); }}
        };
    }
}