#include "BSML/TypeHandlers/ScrollIndicatorHandler.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "Helpers/utilities.hpp"

UnityEngine::UI::Image* GetHandleImage(BSML::ScrollIndicator* indicator) {
    return indicator->get_Handle()->GetComponent<UnityEngine::UI::Image*>();
}

void SetHandleColor(BSML::ScrollIndicator* indicator, const StringParseHelper& htmlColor) {
    GetHandleImage(indicator)->set_color(static_cast<UnityEngine::Color>(htmlColor));
}

namespace BSML { 
    static ScrollIndicatorHandler scrollIndicatorHandler{};

    ScrollIndicatorHandler::Base::PropMap ScrollIndicatorHandler::get_props() const {
        return {
            { "handleColor", {"handle-color"} },
            { "handleImage", {"handle-image"} }
        };
    }

    ScrollIndicatorHandler::Base::SetterMap ScrollIndicatorHandler::get_setters() const {
        return {
            { "handleColor", SetHandleColor },
            { "handleImage", [](auto component, auto value){ Utilities::SetImage(GetHandleImage(component), value); }}
        };
    }
}
