#include "BSML/TypeHandlers/ButtonIconHandler.hpp"

namespace BSML {
    static ButtonIconHandler buttonIconHandler;

    ButtonIconHandler::Base::PropMap ButtonIconHandler::get_props() const {
        return {
            {"icon", {"icon"}},
            {"iconSkew", {"icon-skew"}},
            {"showUnderline", {"show-underline"}}
        };
    }

    ButtonIconHandler::Base::SetterMap ButtonIconHandler::get_setters() const {
        return {
            {"icon", [](auto component, auto value){ component->SetIcon(value); }},
            {"iconSkew", [](auto component, auto value){ component->SetSkew(value); }},
            {"showUnderline", [](auto component, auto value){ component->SetUnderlineActive(value); }}
        };
    }
}
